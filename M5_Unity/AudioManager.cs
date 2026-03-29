/// <summary>
/// File: AudioManager.cs
/// Brief: Push-to-Talk Audio Capture - public interface
/// Author: Evrim Doğa Solmaz 230104004042
/// Date: 2026-03-29
/// Version: 0.2
/// </summary>

using System;

/* -- Constants & Data Types ------------------------------ */
public static class AudioManagerConstants {
    public const int AUDIO_SAMPLE_RATE_HZ = 16000; 
    public const int AUDIO_MAX_RECORD_SECS = 10;   
}

public enum AudioCaptureState { Idle=0, Recording=1, Encoding=2, Sending=3 }

/* -- Contract Class -------------------------------------- */
public class AudioManager 
{
    public event Action<byte[]> OnAudioBlobReady;
    public event Action<AudioCaptureState> OnCaptureStateChanged;

    public void SetNetworkClient(INetworkClient client) { 
        throw new NotImplementedException(); 
    }

    public void StartRecording() { 
        throw new NotImplementedException(); 
    }

    public void StopAndEncode() { 
        throw new NotImplementedException(); 
    }

    public AudioCaptureState GetCaptureState() { 
        throw new NotImplementedException(); 
    }

    /* -- Private Internal Logic (Visible for Architecture) -- */
    
    /// <summary>
    /// Internal helper to encode raw microphone data to PCM .wav format.
    /// This demonstrates the internal audio pipeline separation.
    /// </summary>
    private byte[] EncodeToWav(object clip) { 
        throw new NotImplementedException(); 
    }
}