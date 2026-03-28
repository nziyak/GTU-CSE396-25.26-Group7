/// <summary>
/// File: IAudioManager.cs
/// Brief: Push-to-Talk Audio Capture - public interface
/// Author: Evrim [Soyisim] [Öğrenci No]
/// Date: 2026-03-28
/// Version: 0.2
///
/// Changelog:
/// v0.1 (2026-03-28) - Initial draft: StartRecording, StopAndEncode, OnAudioBlobReady
/// v0.2 (2026-03-28) - Aligned with Ziya's DataContracts.cs and INetworkClient.cs
/// </summary>

using System;

/* -- Constants ------------------------------------------- */

/// <summary>
/// Constants for microphone recording duration and sample rates.
/// </summary>
public static class AudioManagerConstants
{
    public const int AUDIO_SAMPLE_RATE_HZ = 16000; 
    public const int AUDIO_MAX_RECORD_SECS = 10;   
    public const int AUDIO_CHANNELS = 1;      
}

/* -- Data Types ------------------------------------------ */

/// <summary>
/// Current state of the Push-to-Talk pipeline.
/// </summary>
public enum AudioCaptureState
{
    Idle = 0,      
    Recording = 1, 
    Encoding = 2,  
    Sending = 3    
}

/* -- Public Interface ------------------------------------ */

/// <summary>
/// IAudioManager — Push-to-Talk audio capture contract for MOD-05 Unity Digital Twin.
/// </summary>
public interface IAudioManager
{
    /// <summary>
    /// Fired after encoding, before SendAudioBlob. UIManager subscribes for HUD feedback.
    /// </summary>
    event Action<byte[]> OnAudioBlobReady;

    /// <summary>
    /// Fired on every state transition: Idle -> Recording -> Encoding -> Sending -> Idle.
    /// </summary>
    event Action<AudioCaptureState> OnCaptureStateChanged;

    /// <summary>
    /// Injects the NetworkClient. Must be called before any recording starts.
    /// </summary>
    /// <param name="client">INetworkClient instance</param>
    void SetNetworkClient(INetworkClient client);

    /// <summary>
    /// Starts microphone capture. Ignored if already Recording.
    /// </summary>
    void StartRecording();

    /// <summary>
    /// Stops capture, encodes to .wav, and prepares the blob for network transmission.
    /// </summary>
    void StopAndEncode();

    /// <summary>
    /// Returns the current pipeline state.
    /// </summary>
    /// <returns>AudioCaptureState (Idle / Recording / Encoding / Sending)</returns>
    AudioCaptureState GetCaptureState();
}