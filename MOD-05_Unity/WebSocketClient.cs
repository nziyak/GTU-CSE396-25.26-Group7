/// <summary>
/// File:    WebSocketClient.cs
/// Brief:   Minimal Socket.IO-compatible INetworkClient implementation for Unity
/// Author:  Dicle Çoban
/// Date:    2026-04-18
/// Version: 0.1
///
/// Notes:
/// - Connects to a Flask-SocketIO backend over Engine.IO v4 WebSocket transport.
/// - Receives "telemetry_update" events and forwards raw JSON to RobotManager.
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
    private const string VideoFrameEventName = "video_frame";
    private const string UnityPingEventName = "unity_ping";
    private const string UnityPongEventName = "unity_pong";
    private const string OperatorCommandEventName = "operator_command";
    private const string AudioEventName = "audio_received";
    private const int PingIntervalMs = 3000;
    private const int MaxReconnectDelayMs = 16000;

    private readonly SynchronizationContext mainThreadContext;
    private readonly SemaphoreSlim sendSemaphore = new SemaphoreSlim(1, 1);

    private ClientWebSocket socket;
    private CancellationTokenSource cancellationTokenSource;
    private Task receiveLoopTask;
    private Task pingLoopTask;
    private volatile bool isConnected;
    private volatile bool manualDisconnect;
    private string lastEndpoint;
    private int reconnectAttempt;
    private long lastPingTicks;
    private NetworkConnectionState connectionState = NetworkConnectionState.Disconnected;

    public event Action<string> OnTelemetryJsonReceived;
    public event Action<byte[]> OnVideoFrameReceived;
    public event Action<NetworkConnectionState> OnConnectionStateChanged;
    public event Action<float> OnLatencyUpdated;
    public bool IsConnected => isConnected;

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
        manualDisconnect = false;
        lastEndpoint = ipAddress;
        await ConnectAsync(ipAddress);
    }

    public async void Disconnect()
    {
        manualDisconnect = true;
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
        await DisconnectAsync(false);

        cancellationTokenSource = new CancellationTokenSource();
        socket = new ClientWebSocket();
        SetConnectionState(NetworkConnectionState.Connecting);

        Uri socketUri = BuildSocketIoUri(address);
        try
        {
            await socket.ConnectAsync(socketUri, cancellationTokenSource.Token);
            receiveLoopTask = ReceiveLoopAsync(cancellationTokenSource.Token);
            pingLoopTask = PingLoopAsync(cancellationTokenSource.Token);
        }
        catch (Exception ex)
        {
            Debug.LogError($"WebSocketClient connection failed: {ex.Message}");
            isConnected = false;
            SetConnectionState(NetworkConnectionState.Disconnected);
            ScheduleReconnect();
        }
    }

    private async Task DisconnectAsync(bool publishDisconnected = true)
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

        if (publishDisconnected)
        {
            SetConnectionState(NetworkConnectionState.Disconnected);
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
                    SetConnectionState(NetworkConnectionState.Disconnected);
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
            SetConnectionState(NetworkConnectionState.Disconnected);
            Debug.LogError($"WebSocketClient receive loop failed: {ex.Message}");
        }

        if (!manualDisconnect)
        {
            ScheduleReconnect();
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
            reconnectAttempt = 0;
            SetConnectionState(NetworkConnectionState.Connected);
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
        int separatorIndex = eventPayload.IndexOf(',', eventNameEnd + 1);
        if (separatorIndex < 0)
        {
            return;
        }

        string payloadJson = eventPayload.Substring(separatorIndex + 1).Trim();
        if (payloadJson.EndsWith("]", StringComparison.Ordinal))
        {
            payloadJson = payloadJson.Substring(0, payloadJson.Length - 1);
        }

        if (string.Equals(eventName, TelemetryEventName, StringComparison.Ordinal))
        {
            mainThreadContext.Post(_ => OnTelemetryJsonReceived?.Invoke(payloadJson), null);
            return;
        }

        if (string.Equals(eventName, VideoFrameEventName, StringComparison.Ordinal))
        {
            byte[] jpegBytes = DecodeVideoFramePayload(payloadJson);
            if (jpegBytes != null && jpegBytes.Length > 0)
            {
                mainThreadContext.Post(_ => OnVideoFrameReceived?.Invoke(jpegBytes), null);
            }
            return;
        }

        if (string.Equals(eventName, UnityPongEventName, StringComparison.Ordinal))
        {
            PublishLatencySample();
        }
    }

    private async Task PingLoopAsync(CancellationToken cancellationToken)
    {
        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                await Task.Delay(PingIntervalMs, cancellationToken);
                if (isConnected && socket != null && socket.State == WebSocketState.Open)
                {
                    lastPingTicks = DateTime.UtcNow.Ticks;
                    string payloadJson = "{\"clientTicks\":" + lastPingTicks + "}";
                    await SendSocketIoEventAsync(UnityPingEventName, payloadJson);
                }
            }
            catch (OperationCanceledException)
            {
                return;
            }
        }
    }

    private async void ScheduleReconnect()
    {
        if (manualDisconnect || string.IsNullOrEmpty(lastEndpoint))
        {
            return;
        }

        int delayMs = Mathf.Min(MaxReconnectDelayMs, 2000 * (1 << Mathf.Min(reconnectAttempt, 3)));
        reconnectAttempt++;

        try
        {
            await Task.Delay(delayMs);
            if (!manualDisconnect)
            {
                await ConnectAsync(lastEndpoint);
            }
        }
        catch (Exception ex)
        {
            Debug.LogWarning($"WebSocketClient reconnect failed: {ex.Message}");
        }
    }

    private void PublishLatencySample()
    {
        if (lastPingTicks <= 0)
        {
            return;
        }

        float latencyMs = (float)TimeSpan.FromTicks(DateTime.UtcNow.Ticks - lastPingTicks).TotalMilliseconds;
        mainThreadContext.Post(_ => OnLatencyUpdated?.Invoke(latencyMs), null);
    }

    private void SetConnectionState(NetworkConnectionState newState)
    {
        if (connectionState == newState)
        {
            return;
        }

        connectionState = newState;
        mainThreadContext.Post(_ => OnConnectionStateChanged?.Invoke(newState), null);
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

    private static byte[] DecodeVideoFramePayload(string payloadJson)
    {
        string dataUrl = ExtractJsonString(payloadJson, "image");
        if (string.IsNullOrEmpty(dataUrl))
        {
            return null;
        }

        const string base64Marker = "base64,";
        int base64Start = dataUrl.IndexOf(base64Marker, StringComparison.OrdinalIgnoreCase);
        string base64 = base64Start >= 0
            ? dataUrl.Substring(base64Start + base64Marker.Length)
            : dataUrl;

        try
        {
            return Convert.FromBase64String(base64);
        }
        catch (FormatException ex)
        {
            Debug.LogWarning($"WebSocketClient video_frame decode failed: {ex.Message}");
            return null;
        }
    }

    private static string ExtractJsonString(string json, string key)
    {
        string quotedKey = "\"" + key + "\"";
        int keyIndex = json.IndexOf(quotedKey, StringComparison.Ordinal);
        if (keyIndex < 0)
        {
            return string.Empty;
        }

        int colonIndex = json.IndexOf(':', keyIndex + quotedKey.Length);
        if (colonIndex < 0)
        {
            return string.Empty;
        }

        int valueStart = json.IndexOf('"', colonIndex + 1);
        if (valueStart < 0)
        {
            return string.Empty;
        }

        StringBuilder valueBuilder = new StringBuilder();
        bool escaped = false;
        for (int i = valueStart + 1; i < json.Length; i++)
        {
            char c = json[i];
            if (escaped)
            {
                valueBuilder.Append(c);
                escaped = false;
                continue;
            }

            if (c == '\\')
            {
                escaped = true;
                continue;
            }

            if (c == '"')
            {
                return valueBuilder.ToString();
            }

            valueBuilder.Append(c);
        }

        return string.Empty;
    }
}
