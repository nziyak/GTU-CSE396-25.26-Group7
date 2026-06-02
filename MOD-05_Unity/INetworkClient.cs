/// <summary>
/// File: INetworkClient.cs
/// Brief: Network abstraction for real WebSocket and mock-file telemetry sources.
/// </summary>

using System;

public interface INetworkClient
{
    /// <summary>
    /// Fired with raw telemetry JSON. RobotManager owns deserialization and event fan-out.
    /// </summary>
    event Action<string> OnTelemetryJsonReceived;

    /// <summary>
    /// Fired with compressed JPEG frame bytes received from MOD-04 video_frame events.
    /// </summary>
    event Action<byte[]> OnVideoFrameReceived;

    /// <summary>
    /// Fired with raw diagnostic JSON received from MOD-04 uart_debug events.
    /// </summary>
    event Action<string> OnUartDebugJsonReceived;

    /// <summary>
    /// Fired with raw parsed-STT JSON received from MOD-04 speech_command_parsed events.
    /// </summary>
    event Action<string> OnSpeechCommandParsedJsonReceived;

    /// <summary>
    /// Fired whenever the concrete client enters Connecting, Connected, or Disconnected.
    /// </summary>
    event Action<NetworkConnectionState> OnConnectionStateChanged;

    /// <summary>
    /// Fired when a ping/pong latency sample is available.
    /// </summary>
    event Action<float> OnLatencyUpdated;

    /// <summary>
    /// True while the concrete client is actively connected or replaying data.
    /// </summary>
    bool IsConnected { get; }

    /// <summary>
    /// Connects to the selected telemetry source. Real clients use a URL, mock clients use a file name/path.
    /// </summary>
    void Connect(string endpoint);

    /// <summary>
    /// Disconnects gracefully from the selected telemetry source.
    /// </summary>
    void Disconnect();

    /// <summary>
    /// Sends a manual override command to the robot.
    /// </summary>
    void SendOperatorCommand(string command);

    /// <summary>
    /// Sends the recorded microphone audio blob for Edge STT processing.
    /// </summary>
    void SendAudioBlob(byte[] wavData);
}
