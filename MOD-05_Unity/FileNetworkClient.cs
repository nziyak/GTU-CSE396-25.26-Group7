/// <summary>
/// File:    FileNetworkClient.cs
/// Brief:   Lightweight mock INetworkClient that replays telemetry from a JSON file.
/// </summary>

using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;

public class FileNetworkClient : INetworkClient
{
    private readonly float updateIntervalSeconds;
    private readonly SynchronizationContext mainThreadContext;
    private readonly ManualResetEventSlim playGate = new ManualResetEventSlim(true);

    private CancellationTokenSource cancellationTokenSource;
    private TelemetryReplayFrame[] replayFrames;

    public event Action<string> OnTelemetryJsonReceived;
    public event Action<byte[]> OnVideoFrameReceived;
    public event Action<string> OnUartDebugJsonReceived;
    public event Action<string> OnSpeechCommandParsedJsonReceived;
    public event Action<NetworkConnectionState> OnConnectionStateChanged;
    public event Action<float> OnLatencyUpdated;
    public bool IsConnected => cancellationTokenSource != null && !cancellationTokenSource.IsCancellationRequested;

    public FileNetworkClient(float updateIntervalSeconds)
    {
        this.updateIntervalSeconds = Mathf.Max(0.1f, updateIntervalSeconds);
        mainThreadContext = SynchronizationContext.Current ?? new SynchronizationContext();
    }

    public async void Connect(string fileName)
    {
        Disconnect();

        cancellationTokenSource = new CancellationTokenSource();
        string filePath = ResolveFilePath(fileName);
        PublishConnectionState(NetworkConnectionState.Connecting);

        if (!File.Exists(filePath))
        {
            Debug.LogWarning($"FileNetworkClient: Mock telemetry file not found at '{filePath}'.");
            PublishConnectionState(NetworkConnectionState.Disconnected);
            return;
        }

        PublishConnectionState(NetworkConnectionState.Connected);
        mainThreadContext.Post(_ => OnLatencyUpdated?.Invoke(0f), null);
        await ReplayFileAsync(filePath, cancellationTokenSource.Token);
    }

    public void Disconnect()
    {
        if (cancellationTokenSource == null)
        {
            return;
        }

        cancellationTokenSource.Cancel();
        cancellationTokenSource.Dispose();
        cancellationTokenSource = null;
        PublishConnectionState(NetworkConnectionState.Disconnected);
    }

    public void Play()
    {
        playGate.Set();
    }

    public void Pause()
    {
        playGate.Reset();
    }

    public void Stop()
    {
        Disconnect();
    }

    public void SendOperatorCommand(string command)
    {
        Debug.Log($"FileNetworkClient: Mock command sent: {command}");
    }

    public void SendAudioBlob(byte[] wavData)
    {
        int byteCount = wavData != null ? wavData.Length : 0;
        Debug.Log($"FileNetworkClient: Mock audio blob sent: {byteCount} bytes");
    }

    private async Task ReplayFileAsync(string filePath, CancellationToken cancellationToken)
    {
        try
        {
            string fileJson = File.ReadAllText(filePath);
            replayFrames = ParseReplayFrames(fileJson);
        }
        catch (Exception ex)
        {
            Debug.LogWarning($"FileNetworkClient: Failed to read mock telemetry: {ex.Message}");
            return;
        }

        if (replayFrames == null || replayFrames.Length == 0)
        {
            return;
        }

        int frameIndex = 0;
        while (!cancellationToken.IsCancellationRequested)
        {
            for (; frameIndex < replayFrames.Length; frameIndex++)
            {
                try
                {
                    playGate.Wait(cancellationToken);
                }
                catch (OperationCanceledException)
                {
                    return;
                }

                TelemetryReplayFrame frame = replayFrames[frameIndex];
                mainThreadContext.Post(_ => OnTelemetryJsonReceived?.Invoke(frame.json), null);

                int delayMs = ResolveDelayMs(frameIndex);
                try
                {
                    await Task.Delay(delayMs, cancellationToken);
                }
                catch (OperationCanceledException)
                {
                    return;
                }
            }

            frameIndex = 0;
        }
    }

    private int ResolveDelayMs(int frameIndex)
    {
        if (replayFrames == null || frameIndex >= replayFrames.Length - 1)
        {
            return Mathf.RoundToInt(updateIntervalSeconds * 1000f);
        }

        int delta = replayFrames[frameIndex + 1].timestampMs - replayFrames[frameIndex].timestampMs;
        return Mathf.Clamp(delta, 1, 60000);
    }

    private static TelemetryReplayFrame[] ParseReplayFrames(string json)
    {
        string trimmed = json.Trim();
        if (!trimmed.StartsWith("[", StringComparison.Ordinal))
        {
            return new[] { new TelemetryReplayFrame { timestampMs = 0, json = trimmed } };
        }

        string[] objectJsons = SplitTopLevelObjects(trimmed);
        TelemetryReplayFrame[] frames = new TelemetryReplayFrame[objectJsons.Length];
        for (int i = 0; i < objectJsons.Length; i++)
        {
            frames[i] = new TelemetryReplayFrame
            {
                timestampMs = ExtractJsonInt(objectJsons[i], "timestamp_ms"),
                json = objectJsons[i]
            };
        }

        return frames;
    }

    private static string[] SplitTopLevelObjects(string arrayJson)
    {
        System.Collections.Generic.List<string> objects = new System.Collections.Generic.List<string>();
        int depth = 0;
        int start = -1;
        bool inString = false;
        bool escaped = false;

        for (int i = 0; i < arrayJson.Length; i++)
        {
            char c = arrayJson[i];
            if (escaped)
            {
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
                inString = !inString;
                continue;
            }

            if (inString)
            {
                continue;
            }

            if (c == '{')
            {
                if (depth == 0)
                {
                    start = i;
                }

                depth++;
            }
            else if (c == '}')
            {
                depth--;
                if (depth == 0 && start >= 0)
                {
                    objects.Add(arrayJson.Substring(start, i - start + 1));
                    start = -1;
                }
            }
        }

        return objects.ToArray();
    }

    private static int ExtractJsonInt(string json, string key)
    {
        string quotedKey = "\"" + key + "\"";
        int keyIndex = json.IndexOf(quotedKey, StringComparison.Ordinal);
        if (keyIndex < 0)
        {
            return 0;
        }

        int colonIndex = json.IndexOf(':', keyIndex + quotedKey.Length);
        if (colonIndex < 0)
        {
            return 0;
        }

        int endIndex = colonIndex + 1;
        while (endIndex < json.Length && (char.IsWhiteSpace(json[endIndex]) || json[endIndex] == '-'))
        {
            endIndex++;
        }

        int startIndex = colonIndex + 1;
        while (startIndex < json.Length && char.IsWhiteSpace(json[startIndex]))
        {
            startIndex++;
        }

        while (endIndex < json.Length && char.IsDigit(json[endIndex]))
        {
            endIndex++;
        }

        if (int.TryParse(json.Substring(startIndex, endIndex - startIndex), out int parsed))
        {
            return parsed;
        }

        return 0;
    }

    private void PublishConnectionState(NetworkConnectionState state)
    {
        mainThreadContext.Post(_ => OnConnectionStateChanged?.Invoke(state), null);
    }

    private static string ResolveFilePath(string fileName)
    {
        if (Path.IsPathRooted(fileName))
        {
            return fileName;
        }

        string streamingAssetsPath = Path.Combine(Application.streamingAssetsPath, fileName);
        if (File.Exists(streamingAssetsPath))
        {
            return streamingAssetsPath;
        }

        return Path.Combine(Application.dataPath, fileName);
    }

    private struct TelemetryReplayFrame
    {
        public int timestampMs;
        public string json;
    }
}
