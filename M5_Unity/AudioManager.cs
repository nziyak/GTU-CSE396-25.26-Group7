/// <summary>
/// File:    AudioManager.cs
/// Brief:   Push-to-Talk Audio Capture - public interface
/// Author:  Evrim Doğa Solmaz 230104004042
/// Date:    2026-03-28
/// Version: 0.2
///
/// Changelog:
/// v0.1 (2026-03-28) - Initial draft: StartRecording, StopAndEncode, OnAudioBlobReady
/// v0.2 (2026-03-28) - Aligned with Ziya's DataContracts.cs and INetworkClient.cs
/// </summary>

using System;
using UnityEngine;
// DataContracts.cs   // TelemetryData, VictimStatus — defined by Ziya
// INetworkClient.cs  // SendAudioBlob()             — defined by Ziya

// -- Constants ------------------------------------------------------------

public static class AudioManagerConstants
{
    public const int AUDIO_SAMPLE_RATE_HZ  = 16000; // Required by Vosk/Whisper.cpp on Pi 5
    public const int AUDIO_MAX_RECORD_SECS = 10;    // Safety cap — auto-stops runaway recording
    public const int AUDIO_CHANNELS        = 1;     // Mono — sufficient and expected by STT
}

// -- Data Types -----------------------------------------------------------

/// <summary>
/// Current state of the Push-to-Talk recording pipeline.
/// </summary>
public enum AudioCaptureState
{
    Idle      = 0,  // No recording in progress
    Recording = 1,  // Actively capturing microphone input
    Encoding  = 2,  // Converting AudioClip to .wav byte[]
    Sending   = 3   // Blob handed off to INetworkClient.SendAudioBlob()
}

// -- Callback Type --------------------------------------------------------

/// <summary>
/// Delegate fired when .wav encoding is complete and blob is ready to send.
/// </summary>
/// <param name="wavData">Encoded .wav byte array</param>
public delegate void AudioBlobReadyCallback(byte[] wavData);

// -- Public Class ---------------------------------------------------------

/// <summary>
/// AudioManager — Push-to-Talk audio capture for MOD-05 Unity Digital Twin.
/// Attach to AudioManager GameObject. Wire PTT button events to
/// StartRecording() (OnPointerDown) and StopAndEncode() (OnPointerUp).
/// </summary>
public class AudioManager : MonoBehaviour
{
    /// <summary>
    /// Fired after .wav encoding is complete, just before SendAudioBlob() is called.
    /// UIManager subscribes to show a "Sending..." indicator on the HUD.
    /// </summary>
    public event Action<byte[]> OnAudioBlobReady;

    /// <summary>
    /// Fired on every state transition: Idle → Recording → Encoding → Sending → Idle.
    /// UIManager subscribes to mirror PTT state on the HUD.
    /// </summary>
    public event Action<AudioCaptureState> OnCaptureStateChanged;

    /// <summary>
    /// Injects Ziya's INetworkClient implementation.
    /// Must be called before any recording. Typically called from RobotManager.Start().
    /// </summary>
    /// <param name="client">Ziya's INetworkClient instance (WebSocketClient)</param>
    public void SetNetworkClient(INetworkClient client) { /* STUB */ }

    /// <summary>
    /// Starts microphone capture. Bind to PTT button's OnPointerDown event.
    /// Ignored if already in Recording state. Sets state → Recording.
    /// </summary>
    public void StartRecording() { /* STUB */ }

    /// <summary>
    /// Stops capture, encodes AudioClip to .wav, sends via INetworkClient.SendAudioBlob().
    /// Bind to PTT button's OnPointerUp event. Ignored safely if not in Recording state.
    /// Internal flow:
    ///   1. Microphone.End()                     → AudioClip
    ///   2. State → Encoding
    ///   3. EncodeToWav(clip)                     → byte[]
    ///   4. OnAudioBlobReady?.Invoke(wavData)
    ///   5. State → Sending
    ///   6. _networkClient.SendAudioBlob(wavData) ← Ziya's INetworkClient
    ///   7. State → Idle
    /// </summary>
    public void StopAndEncode() { /* STUB */ }

    /// <summary>
    /// Returns the current capture state.
    /// UIManager can poll this or subscribe to OnCaptureStateChanged instead.
    /// </summary>
    /// <returns>Current AudioCaptureState (Idle / Recording / Encoding / Sending)</returns>
    public AudioCaptureState GetCaptureState() { return AudioCaptureState.Idle; /* STUB */ }

    // -- Private Helpers --------------------------------------------------

    /// <summary>
    /// Encodes a Unity AudioClip into a standard PCM .wav byte array.
    /// Output: 16-bit PCM, mono, 16000 Hz — required by Vosk/Whisper.cpp on Pi 5.
    /// Writes RIFF/fmt/data chunk headers manually before the raw PCM samples.
    /// </summary>
    /// <param name="clip">AudioClip from Microphone.Start()</param>
    /// <returns>Complete .wav byte array for INetworkClient.SendAudioBlob()</returns>
    private byte[] EncodeToWav(AudioClip clip) { return null; /* STUB */ }
}