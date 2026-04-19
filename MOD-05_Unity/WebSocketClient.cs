/// <summary>
/// File:    WebSocketClient.cs
/// Brief:   Minimal Socket.IO-compatible INetworkClient implementation for Unity
/// Author:  Dicle Çoban
/// Date:    2026-04-18
/// Version: 0.1
///
/// Notes:
/// - Connects to a Flask-SocketIO backend over Engine.IO v4 WebSocket transport.
/// - Receives "telemetry_update" events and forwards them as TelemetryData.
/// - Sends operator commands and audio payloads through Socket.IO event packets.
/// </summary>

using System;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;

public class WebSocketClient : INetworkClient
{
    private const string TelemetryEventName = "telemetry_update";
    private const string OperatorCommandEventName = "operator_command";
    private const string AudioEventName = "audio_received";

    private readonly SynchronizationContext mainThreadContext;
    private readonly SemaphoreSlim sendSemaphore = new SemaphoreSlim(1, 1);

    private ClientWebSocket socket;
    private CancellationTokenSource cancellationTokenSource;
    private Task receiveLoopTask;
    private volatile bool isConnected;

    public event Action<TelemetryData> OnTelemetryReceived;

    public WebSocketClient()
    {
        mainThreadContext = SynchronizationContext.Current ?? new SynchronizationContext();
    }

    /// <summary>
    /// Connects to the Raspberry Pi WebSocket server using Socket.IO's websocket transport.
    /// Example accepted inputs:
    /// - ws://192.168.1.10:5000
    /// - http://192.168.1.10:5000
    /// - ws://192.168.1.10:5000/socket.io/?EIO=4&transport=websocket
    /// </summary>
    public async void Connect(string ipAddress)
    {
        await ConnectAsync(ipAddress);
    }

    public async void Disconnect()
    {
        await DisconnectAsync();
    }

    /// <summary>
    /// Sends a manual override packet to MOD-04. The backend integration docs expect
    /// an object shape compatible with {"override": true, "cmd": "FORWARD"}.
    /// </summary>
    public async void SendOperatorCommand(string command)
    {
        if (string.IsNullOrWhiteSpace(command))
        {
            return;
        }

        string payloadJson = "{\"override\":true,\"cmd\":\"" + EscapeJson(command.Trim()) + "\"}";
        await SendSocketIoEventAsync(OperatorCommandEventName, payloadJson);
    }

    /// <summary>
    /// Sends WAV data as a base64 payload. This keeps the client simple while preserving
    /// the audio bytes faithfully for backend-side decode.
    /// </summary>
    public async void SendAudioBlob(byte[] wavData)
    {
        if (wavData == null || wavData.Length == 0)
        {
            return;
        }

        string base64Audio = Convert.ToBase64String(wavData);
        string payloadJson =
            "{\"encoding\":\"base64-wav\",\"byteCount\":" + wavData.Length + ",\"data\":\"" + base64Audio + "\"}";

        await SendSocketIoEventAsync(AudioEventName, payloadJson);
    }

    private async Task ConnectAsync(string address)
    {
        await DisconnectAsync();

        cancellationTokenSource = new CancellationTokenSource();
        socket = new ClientWebSocket();

        Uri socketUri = BuildSocketIoUri(address);
        try
        {
            await socket.ConnectAsync(socketUri, cancellationTokenSource.Token);
            receiveLoopTask = ReceiveLoopAsync(cancellationTokenSource.Token);
        }
        catch (Exception ex)
        {
            Debug.LogError($"WebSocketClient connection failed: {ex.Message}");
            isConnected = false;
        }
    }

    private async Task DisconnectAsync()
    {
        isConnected = false;

        CancellationTokenSource previousCts = cancellationTokenSource;
        cancellationTokenSource = null;

        if (previousCts != null)
        {
            previousCts.Cancel();
        }

        if (socket != null)
        {
            try
            {
                if (socket.State == WebSocketState.Open || socket.State == WebSocketState.CloseReceived)
                {
                    await socket.CloseAsync(WebSocketCloseStatus.NormalClosure, "Client disconnect", CancellationToken.None);
                }
            }
            catch (Exception ex)
            {
                Debug.LogWarning($"WebSocketClient close warning: {ex.Message}");
            }
            finally
            {
                socket.Dispose();
                socket = null;
            }
        }
    }

    private async Task ReceiveLoopAsync(CancellationToken cancellationToken)
    {
        ArraySegment<byte> receiveBuffer = new ArraySegment<byte>(new byte[8192]);
        StringBuilder textBuilder = new StringBuilder();

        try
        {
            while (!cancellationToken.IsCancellationRequested && socket != null && socket.State == WebSocketState.Open)
            {
                WebSocketReceiveResult result = await socket.ReceiveAsync(receiveBuffer, cancellationToken);
                if (result.MessageType == WebSocketMessageType.Close)
                {
                    isConnected = false;
                    break;
                }

                if (result.MessageType != WebSocketMessageType.Text)
                {
                    continue;
                }

                textBuilder.Append(Encoding.UTF8.GetString(receiveBuffer.Array, 0, result.Count));
                if (!result.EndOfMessage)
                {
                    continue;
                }

                string packet = textBuilder.ToString();
                textBuilder.Length = 0;
                await HandleIncomingPacketAsync(packet, cancellationToken);
            }
        }
        catch (OperationCanceledException)
        {
            // Expected during shutdown.
        }
        catch (Exception ex)
        {
            isConnected = false;
            Debug.LogError($"WebSocketClient receive loop failed: {ex.Message}");
        }
    }

    private async Task HandleIncomingPacketAsync(string packet, CancellationToken cancellationToken)
    {
        if (string.IsNullOrEmpty(packet))
        {
            return;
        }

        if (packet[0] == '0')
        {
            await SendRawTextAsync("40", cancellationToken);
            return;
        }

        if (packet == "40" || packet.StartsWith("40{", StringComparison.Ordinal))
        {
            isConnected = true;
            Debug.Log("WebSocketClient Socket.IO namespace connected.");
            return;
        }

        if (packet[0] == '2')
        {
            await SendRawTextAsync("3", cancellationToken);
            return;
        }

        if (packet[0] == '3')
        {
            return;
        }

        if (packet.StartsWith("42", StringComparison.Ordinal))
        {
            HandleSocketIoEvent(packet.Substring(2));
        }
    }

    private void HandleSocketIoEvent(string eventPayload)
    {
        if (string.IsNullOrEmpty(eventPayload) || eventPayload[0] != '[')
        {
            return;
        }

        int eventNameStart = eventPayload.IndexOf('"');
        if (eventNameStart < 0)
        {
            return;
        }

        int eventNameEnd = eventPayload.IndexOf('"', eventNameStart + 1);
        if (eventNameEnd <= eventNameStart)
        {
            return;
        }

        string eventName = eventPayload.Substring(eventNameStart + 1, eventNameEnd - eventNameStart - 1);
        if (!string.Equals(eventName, TelemetryEventName, StringComparison.Ordinal))
        {
            return;
        }

        int separatorIndex = eventPayload.IndexOf(',', eventNameEnd + 1);
        if (separatorIndex < 0)
        {
            return;
        }

        string telemetryJson = eventPayload.Substring(separatorIndex + 1).Trim();
        if (telemetryJson.EndsWith("]", StringComparison.Ordinal))
        {
            telemetryJson = telemetryJson.Substring(0, telemetryJson.Length - 1);
        }

        TelemetryData telemetry = ParseTelemetryData(telemetryJson);
        mainThreadContext.Post(_ => OnTelemetryReceived?.Invoke(telemetry), null);
    }

    private TelemetryData ParseTelemetryData(string telemetryJson)
    {
        TelemetryWirePayload payload = JsonUtility.FromJson<TelemetryWirePayload>(telemetryJson);

        TelemetryData data = new TelemetryData
        {
            posX = payload.HasUnderscoreCoordinates ? payload.pos_x : payload.posX,
            posY = payload.HasUnderscoreCoordinates ? payload.pos_y : payload.posY,
            temperature = payload.HasShortTemperature ? payload.temp : payload.temperature,
            smokeDetected = payload.HasShortSmoke ? payload.smoke : payload.smokeDetected || payload.smoke_detected,
            priorityLevel = payload.HasShortPriority ? payload.priority : payload.priorityLevel
        };

        if (!string.IsNullOrEmpty(payload.victim_status))
        {
            data.victimStatus = ParseVictimStatus(payload.victim_status);
        }
        else
        {
            data.victimStatus = payload.victimStatus;
        }

        if (payload.acousticHit || payload.acoustic_hit)
        {
            data.acousticHit = true;
        }

        data.acousticAngle = Mathf.Abs(payload.acoustic_angle) > Mathf.Epsilon
            ? payload.acoustic_angle
            : payload.acousticAngle;

        return data;
    }

    private static VictimStatus ParseVictimStatus(string rawStatus)
    {
        if (Enum.TryParse(rawStatus, true, out VictimStatus parsedStatus))
        {
            return parsedStatus;
        }

        return VictimStatus.NONE;
    }

    private async Task SendSocketIoEventAsync(string eventName, string payloadJson)
    {
        if (!isConnected || socket == null || socket.State != WebSocketState.Open)
        {
            Debug.LogWarning($"WebSocketClient is not connected. Dropped event '{eventName}'.");
            return;
        }

        string packet = "42[\"" + eventName + "\"," + payloadJson + "]";
        await SendRawTextAsync(packet, cancellationTokenSource != null ? cancellationTokenSource.Token : CancellationToken.None);
    }

    private async Task SendRawTextAsync(string message, CancellationToken cancellationToken)
    {
        if (socket == null || socket.State != WebSocketState.Open)
        {
            return;
        }

        byte[] messageBytes = Encoding.UTF8.GetBytes(message);
        ArraySegment<byte> messageSegment = new ArraySegment<byte>(messageBytes);

        await sendSemaphore.WaitAsync(cancellationToken);
        try
        {
            await socket.SendAsync(messageSegment, WebSocketMessageType.Text, true, cancellationToken);
        }
        finally
        {
            sendSemaphore.Release();
        }
    }

    private static Uri BuildSocketIoUri(string address)
    {
        if (string.IsNullOrWhiteSpace(address))
        {
            throw new ArgumentException("Socket address cannot be empty.", nameof(address));
        }

        string normalizedAddress = address.Trim();
        if (normalizedAddress.StartsWith("http://", StringComparison.OrdinalIgnoreCase))
        {
            normalizedAddress = "ws://" + normalizedAddress.Substring("http://".Length);
        }
        else if (normalizedAddress.StartsWith("https://", StringComparison.OrdinalIgnoreCase))
        {
            normalizedAddress = "wss://" + normalizedAddress.Substring("https://".Length);
        }

        if (normalizedAddress.IndexOf("/socket.io/", StringComparison.OrdinalIgnoreCase) < 0)
        {
            normalizedAddress = normalizedAddress.TrimEnd('/') + "/socket.io/?EIO=4&transport=websocket";
        }
        else
        {
            if (normalizedAddress.IndexOf("EIO=", StringComparison.OrdinalIgnoreCase) < 0)
            {
                normalizedAddress += normalizedAddress.IndexOf('?', StringComparison.Ordinal) >= 0 ? "&EIO=4" : "?EIO=4";
            }

            if (normalizedAddress.IndexOf("transport=", StringComparison.OrdinalIgnoreCase) < 0)
            {
                normalizedAddress += normalizedAddress.IndexOf('?', StringComparison.Ordinal) >= 0 ? "&transport=websocket" : "?transport=websocket";
            }
        }

        return new Uri(normalizedAddress);
    }

    private static string EscapeJson(string value)
    {
        return value
            .Replace("\\", "\\\\")
            .Replace("\"", "\\\"");
    }

    [Serializable]
    private struct TelemetryWirePayload
    {
        public float posX;
        public float posY;
        public float temperature;
        public bool smokeDetected;
        public VictimStatus victimStatus;
        public int priorityLevel;
        public bool acousticHit;
        public float acousticAngle;

        public float pos_x;
        public float pos_y;
        public float temp;
        public bool smoke;
        public string victim_status;
        public int priority;
        public bool acoustic_hit;
        public float acoustic_angle;
        public bool smoke_detected;

        public bool HasUnderscoreCoordinates => Mathf.Abs(pos_x) > Mathf.Epsilon || Mathf.Abs(pos_y) > Mathf.Epsilon;
        public bool HasShortTemperature => Mathf.Abs(temp) > Mathf.Epsilon;
        public bool HasShortSmoke => smoke || smoke_detected;
        public bool HasShortPriority => priority != 0;
    }
}
