/// <summary>
/// File: AudioManager.cs
/// Brief: Push-to-Talk Audio Capture - public interface
/// Author: Evrim Doğa Solmaz 230104004042
/// Date: 2026-03-29
/// Version: 0.2
/// </summary>

using System;
using System.IO;
using UnityEngine;

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

    private INetworkClient networkClient;
    private AudioCaptureState currentState = AudioCaptureState.Idle;
    private AudioClip recordingClip;
    private string microphoneDevice;

    public void SetNetworkClient(INetworkClient client) { 
        networkClient = client;
    }

    public void StartRecording() { 
        if (currentState != AudioCaptureState.Idle)
        {
            Debug.LogWarning("AudioManager: Cannot start recording — current state is " + currentState);
            return;
        }

        // Pick the default microphone (null = default device in Unity)
        microphoneDevice = null;
        if (Microphone.devices.Length > 0)
        {
            microphoneDevice = Microphone.devices[0];
        }
        else
        {
            Debug.LogError("AudioManager: No microphone device found.");
            return;
        }

        recordingClip = Microphone.Start(
            microphoneDevice,
            false, // loop = false, single recording
            AudioManagerConstants.AUDIO_MAX_RECORD_SECS,
            AudioManagerConstants.AUDIO_SAMPLE_RATE_HZ
        );

        SetState(AudioCaptureState.Recording);
        Debug.Log("AudioManager: Recording started.");
    }

    public void StopAndEncode() { 
        if (currentState != AudioCaptureState.Recording)
        {
            Debug.LogWarning("AudioManager: Cannot stop — not recording. Current state: " + currentState);
            return;
        }

        // Determine how many samples were actually recorded
        int recordedSamples = Microphone.GetPosition(microphoneDevice);
        Microphone.End(microphoneDevice);

        if (recordedSamples <= 0 || recordingClip == null)
        {
            Debug.LogWarning("AudioManager: No audio data captured.");
            SetState(AudioCaptureState.Idle);
            return;
        }

        SetState(AudioCaptureState.Encoding);
        Debug.Log($"AudioManager: Encoding {recordedSamples} samples...");

        // Extract only the recorded portion of the clip
        float[] samples = new float[recordedSamples * recordingClip.channels];
        recordingClip.GetData(samples, 0);

        byte[] wavData = EncodeToWav(samples, recordingClip.channels, recordingClip.frequency);

        SetState(AudioCaptureState.Sending);
        OnAudioBlobReady?.Invoke(wavData);

        // Send directly to network if client is available
        if (networkClient != null)
        {
            networkClient.SendAudioBlob(wavData);
            Debug.Log($"AudioManager: Sent {wavData.Length} bytes to network.");
        }

        SetState(AudioCaptureState.Idle);
    }

    public AudioCaptureState GetCaptureState() { 
        return currentState;
    }

    /* -- Private Internal Logic (Visible for Architecture) -- */

    /// <summary>
    /// Sets the current capture state and fires the state-changed event.
    /// </summary>
    private void SetState(AudioCaptureState newState)
    {
        currentState = newState;
        OnCaptureStateChanged?.Invoke(currentState);
    }

    /// <summary>
    /// Encodes raw PCM float samples into a standard 16-bit PCM .wav byte array.
    /// WAV format: RIFF header (44 bytes) + raw PCM data.
    /// </summary>
    /// <param name="samples">Float sample array from AudioClip.GetData</param>
    /// <param name="channels">Number of audio channels (1 = mono)</param>
    /// <param name="sampleRate">Sample rate in Hz</param>
    /// <returns>Complete .wav file as byte array</returns>
    private byte[] EncodeToWav(float[] samples, int channels, int sampleRate)
    {
        int bitsPerSample = 16;
        int byteRate = sampleRate * channels * (bitsPerSample / 8);
        int blockAlign = channels * (bitsPerSample / 8);
        int dataSize = samples.Length * (bitsPerSample / 8);

        using (MemoryStream stream = new MemoryStream(44 + dataSize))
        using (BinaryWriter writer = new BinaryWriter(stream))
        {
            // RIFF header
            writer.Write(System.Text.Encoding.ASCII.GetBytes("RIFF"));
            writer.Write(36 + dataSize);                     // ChunkSize
            writer.Write(System.Text.Encoding.ASCII.GetBytes("WAVE"));

            // fmt sub-chunk
            writer.Write(System.Text.Encoding.ASCII.GetBytes("fmt "));
            writer.Write(16);                                // Subchunk1Size (PCM)
            writer.Write((short)1);                          // AudioFormat (1 = PCM)
            writer.Write((short)channels);                   // NumChannels
            writer.Write(sampleRate);                        // SampleRate
            writer.Write(byteRate);                          // ByteRate
            writer.Write((short)blockAlign);                 // BlockAlign
            writer.Write((short)bitsPerSample);              // BitsPerSample

            // data sub-chunk
            writer.Write(System.Text.Encoding.ASCII.GetBytes("data"));
            writer.Write(dataSize);                          // Subchunk2Size

            // Convert float samples [-1.0, 1.0] to 16-bit signed integers
            for (int i = 0; i < samples.Length; i++)
            {
                float clamped = Mathf.Clamp(samples[i], -1f, 1f);
                short pcm16 = (short)(clamped * short.MaxValue);
                writer.Write(pcm16);
            }

            return stream.ToArray();
        }
    }

    /// <summary>
    /// Overload kept for backward compatibility with the original signature.
    /// Internal helper to encode raw microphone data to PCM .wav format.
    /// This demonstrates the internal audio pipeline separation.
    /// </summary>
    private byte[] EncodeToWav(object clip) { 
        // Legacy stub — real encoding uses the typed overload above.
        Debug.LogWarning("AudioManager: EncodeToWav(object) called — use the typed overload instead.");
        return new byte[0];
    }
}