/// <summary>
/// File: INetworkClient.cs
/// Brief: WebSocket Network Interface for Unity to communicate with Raspberry Pi 5
/// Author: Ziya 210104004027
/// Date: 2026-03-27
/// Version: 0.1
/// 
/// Changelog:
/// v0.1 - Initial draft, defined connection and audio streaming events.
/// </summary>

using System;

public interface INetworkClient
{
    /// <summary>
    /// Event triggered when a new telemetry JSON is received and parsed.
    /// </summary>
    event Action<TelemetryData> OnTelemetryReceived;

    /// <summary>
    /// Connects to the Raspberry Pi WebSocket server.
    /// </summary>
    /// <param name="ipAddress">The IP address of the Pi 5 (e.g., "ws://192.168.1.10:5000")</param>
    void Connect(string ipAddress);

    /// <summary>
    /// Disconnects gracefully from the server.
    /// </summary>
    void Disconnect();

    /// <summary>
    /// Sends a manual override command to the robot.
    /// </summary>
    /// <param name="command">String command (e.g., "FORWARD", "STOP")</param>
    void SendOperatorCommand(string command);

    /// <summary>
    /// Sends the recorded microphone audio blob for Edge STT processing.
    /// </summary>
    /// <param name="wavData">Byte array of the .wav file</param>
    void SendAudioBlob(byte[] wavData);
}