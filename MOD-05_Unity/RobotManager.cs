/// <summary>
/// File:    RobotManager.cs
/// Brief:   Central event-driven coordinator for MOD-05 Unity Digital Twin.
/// </summary>

using System;
using System.Threading;
using UnityEngine;

public class RobotManager : MonoBehaviour
{
    [Header("Connection")]
    [SerializeField] private string serverUrl = "ws://localhost:5001";
    [SerializeField] private string mockTelemetryFile = "telemetry.json";
    [SerializeField] private bool connectOnStart = true;

    [Header("Demo / Presentation Mode")]
    [SerializeField] private bool useMockFileData = false;
    [SerializeField] private float mockUpdateIntervalSeconds = 1f;

    [Header("Managers")]
    [SerializeField] private MapManager mapManager;
    [SerializeField] private UIManager uiManager;
    [SerializeField] private MapManager_AcousticBeam acousticBeamManager;

    [Header("Acoustic Beam")]
    [SerializeField] private float acousticCooldownSeconds = 3f;

    /// <summary>Raised after telemetry JSON is deserialized on Unity's main thread.</summary>
    public event Action<TelemetryData> OnTelemetryUpdated;

    /// <summary>Raised only when a victim classification is present.</summary>
    public event Action<TelemetryData> OnVictimDetected;

    /// <summary>Raised when acoustic direction data should update visual/audio subsystems.</summary>
    public event Action<float> OnAcousticAngleUpdated;

    /// <summary>Raised with compressed JPEG bytes received from MOD-04 video_frame events.</summary>
    public event Action<byte[]> OnVideoFrameReceived;

    /// <summary>Raised with the latest optional STM32 diagnostic values.</summary>
    public event Action<UartDebugData> OnUartDebugUpdated;
    public event Action<SpeechCommandParsedData> OnSpeechCommandParsed;

    public event Action<NetworkConnectionState> OnConnectionStateChanged;
    public event Action<float> OnLatencyUpdated;
    public event Action<float> OnPttAmplitudeUpdated;

    private INetworkClient networkClient;
    private AudioManager audioManager;
    private SynchronizationContext mainThreadContext;
    private float lastAcousticHitTime = -999f;
    private float latestLatencyMs;

    private void Awake()
    {
        mainThreadContext = SynchronizationContext.Current;
        audioManager = new AudioManager();
        AutoBindSceneReferences();
    }

    public void AutoBindSceneReferences()
    {
        if (mapManager == null)
        {
            mapManager = FindObjectOfType<MapManager>();
        }

        if (uiManager == null)
        {
            uiManager = FindObjectOfType<UIManager>();
        }

        if (acousticBeamManager == null)
        {
            acousticBeamManager = FindObjectOfType<MapManager_AcousticBeam>();
        }

        if (mapManager == null)
        {
            Debug.LogError("RobotManager: MapManager reference could not be auto-bound.");
        }

        if (uiManager == null)
        {
            Debug.LogError("RobotManager: UIManager reference could not be auto-bound.");
        }
    }

    private void Start()
    {
        ConfigureNetworkClient();
        SubscribeLocalSystems();

        if (audioManager != null)
        {
            audioManager.SetNetworkClient(networkClient);
            audioManager.OnAudioBlobReady += HandleAudioBlobReady;
            audioManager.OnCaptureStateChanged += HandleAudioCaptureStateChanged;
        }

        if (connectOnStart)
        {
            Connect();
        }
    }

    private void OnDestroy()
    {
        UnsubscribeLocalSystems();

        if (audioManager != null)
        {
            audioManager.OnAudioBlobReady -= HandleAudioBlobReady;
            audioManager.OnCaptureStateChanged -= HandleAudioCaptureStateChanged;
        }

        if (networkClient != null)
        {
            networkClient.OnTelemetryJsonReceived -= HandleTelemetryJsonReceived;
            networkClient.OnVideoFrameReceived -= HandleVideoFrameReceived;
            networkClient.OnUartDebugJsonReceived -= HandleUartDebugJsonReceived;
            networkClient.OnSpeechCommandParsedJsonReceived -= HandleSpeechCommandParsedJsonReceived;
            networkClient.OnConnectionStateChanged -= HandleConnectionStateChanged;
            networkClient.OnLatencyUpdated -= HandleLatencyUpdated;
            networkClient.Disconnect();
        }
    }

    public void Connect()
    {
        ConfigureNetworkClient();
        string endpoint = useMockFileData ? mockTelemetryFile : serverUrl;
        networkClient.Connect(endpoint);
    }

    public void Disconnect()
    {
        if (networkClient == null)
        {
            return;
        }

        networkClient.Disconnect();
    }

    public void SetUseMockFileData(bool enabled)
    {
        if (useMockFileData == enabled)
        {
            return;
        }

        useMockFileData = enabled;

        if (connectOnStart || (networkClient != null && networkClient.IsConnected))
        {
            Connect();
        }
    }

    public void SendOperatorCommand(string command)
    {
        if (networkClient == null)
        {
            return;
        }

        networkClient.SendOperatorCommand(command);
    }

    public void StartPushToTalk()
    {
        if (audioManager == null)
        {
            return;
        }

        audioManager.StartRecording();
    }

    public void StopPushToTalk()
    {
        if (audioManager == null)
        {
            return;
        }

        audioManager.StopAndEncode();
        OnPttAmplitudeUpdated?.Invoke(0f);
    }

    public void BindAudioManager(AudioManager manager)
    {
        if (audioManager != null)
        {
            audioManager.OnAudioBlobReady -= HandleAudioBlobReady;
            audioManager.OnCaptureStateChanged -= HandleAudioCaptureStateChanged;
        }

        audioManager = manager;

        if (audioManager != null)
        {
            audioManager.SetNetworkClient(networkClient);
            audioManager.OnAudioBlobReady += HandleAudioBlobReady;
            audioManager.OnCaptureStateChanged += HandleAudioCaptureStateChanged;
        }
    }

    private void ConfigureNetworkClient()
    {
        Type expectedType = useMockFileData ? typeof(FileNetworkClient) : typeof(WebSocketClient);
        if (networkClient != null && networkClient.GetType() == expectedType)
        {
            return;
        }

        if (networkClient != null)
        {
            networkClient.OnTelemetryJsonReceived -= HandleTelemetryJsonReceived;
            networkClient.OnVideoFrameReceived -= HandleVideoFrameReceived;
            networkClient.OnUartDebugJsonReceived -= HandleUartDebugJsonReceived;
            networkClient.OnSpeechCommandParsedJsonReceived -= HandleSpeechCommandParsedJsonReceived;
            networkClient.OnConnectionStateChanged -= HandleConnectionStateChanged;
            networkClient.OnLatencyUpdated -= HandleLatencyUpdated;
            networkClient.Disconnect();
        }

        networkClient = useMockFileData
            ? new FileNetworkClient(mockUpdateIntervalSeconds)
            : new WebSocketClient();

        networkClient.OnTelemetryJsonReceived += HandleTelemetryJsonReceived;
        networkClient.OnVideoFrameReceived += HandleVideoFrameReceived;
        networkClient.OnUartDebugJsonReceived += HandleUartDebugJsonReceived;
        networkClient.OnSpeechCommandParsedJsonReceived += HandleSpeechCommandParsedJsonReceived;
        networkClient.OnConnectionStateChanged += HandleConnectionStateChanged;
        networkClient.OnLatencyUpdated += HandleLatencyUpdated;

        if (audioManager != null)
        {
            audioManager.SetNetworkClient(networkClient);
        }
    }

    private void SubscribeLocalSystems()
    {
        OnTelemetryUpdated += UpdateMap;
        OnTelemetryUpdated += UpdateHUD;
        OnAcousticAngleUpdated += UpdateAcousticBeam;
        OnVideoFrameReceived += UpdateVideoFrame;
        OnUartDebugUpdated += UpdateUartDebug;
        OnSpeechCommandParsed += UpdateSpeechCommandParsed;
        OnConnectionStateChanged += UpdateConnectionState;
        OnLatencyUpdated += UpdateLatency;
        OnPttAmplitudeUpdated += UpdatePttAmplitude;
    }

    private void UnsubscribeLocalSystems()
    {
        OnTelemetryUpdated -= UpdateMap;
        OnTelemetryUpdated -= UpdateHUD;
        OnAcousticAngleUpdated -= UpdateAcousticBeam;
        OnVideoFrameReceived -= UpdateVideoFrame;
        OnUartDebugUpdated -= UpdateUartDebug;
        OnSpeechCommandParsed -= UpdateSpeechCommandParsed;
        OnConnectionStateChanged -= UpdateConnectionState;
        OnLatencyUpdated -= UpdateLatency;
        OnPttAmplitudeUpdated -= UpdatePttAmplitude;
    }

    private void Update()
    {
        if (audioManager == null || audioManager.GetCaptureState() != AudioCaptureState.Recording)
        {
            return;
        }

        OnPttAmplitudeUpdated?.Invoke(audioManager.PollAmplitudeLevel());
    }

    private void HandleTelemetryJsonReceived(string telemetryJson)
    {
        if (mainThreadContext != null && SynchronizationContext.Current != mainThreadContext)
        {
            mainThreadContext.Post(_ => ProcessTelemetryJson(telemetryJson), null);
            return;
        }

        ProcessTelemetryJson(telemetryJson);
    }

    private void ProcessTelemetryJson(string telemetryJson)
    {
        if (string.IsNullOrWhiteSpace(telemetryJson))
        {
            return;
        }

        TelemetryData data;
        try
        {
            data = DeserializeTelemetry(telemetryJson);
        }
        catch (Exception ex)
        {
            Debug.LogWarning($"RobotManager: telemetry parse failed: {ex.Message}");
            return;
        }

        OnTelemetryUpdated?.Invoke(data);

        if (data.victimStatus != VictimStatus.None)
        {
            OnVictimDetected?.Invoke(data);
        }

        if (data.acousticHit)
        {
            lastAcousticHitTime = Time.time;
            OnAcousticAngleUpdated?.Invoke(data.acousticAngle);
        }
        else if (Time.time - lastAcousticHitTime >= acousticCooldownSeconds)
        {
            HideAcousticBeam();
        }
    }

    private void HandleVideoFrameReceived(byte[] jpegBytes)
    {
        if (mainThreadContext != null && SynchronizationContext.Current != mainThreadContext)
        {
            mainThreadContext.Post(_ => OnVideoFrameReceived?.Invoke(jpegBytes), null);
            return;
        }

        OnVideoFrameReceived?.Invoke(jpegBytes);
    }

    private void HandleUartDebugJsonReceived(string uartDebugJson)
    {
        if (mainThreadContext != null && SynchronizationContext.Current != mainThreadContext)
        {
            mainThreadContext.Post(_ => HandleUartDebugJsonReceived(uartDebugJson), null);
            return;
        }

        if (string.IsNullOrWhiteSpace(uartDebugJson))
        {
            return;
        }

        try
        {
            OnUartDebugUpdated?.Invoke(JsonUtility.FromJson<UartDebugData>(uartDebugJson));
        }
        catch (Exception ex)
        {
            Debug.LogWarning($"RobotManager: uart_debug parse failed: {ex.Message}");
        }
    }

    private void HandleConnectionStateChanged(NetworkConnectionState state)
    {
        if (mainThreadContext != null && SynchronizationContext.Current != mainThreadContext)
        {
            mainThreadContext.Post(_ => OnConnectionStateChanged?.Invoke(state), null);
            return;
        }

        OnConnectionStateChanged?.Invoke(state);
    }

    private void HandleSpeechCommandParsedJsonReceived(string speechCommandJson)
    {
        if (mainThreadContext != null && SynchronizationContext.Current != mainThreadContext)
        {
            mainThreadContext.Post(_ => HandleSpeechCommandParsedJsonReceived(speechCommandJson), null);
            return;
        }

        if (string.IsNullOrWhiteSpace(speechCommandJson))
        {
            return;
        }

        try
        {
            OnSpeechCommandParsed?.Invoke(JsonUtility.FromJson<SpeechCommandParsedData>(speechCommandJson));
        }
        catch (Exception ex)
        {
            Debug.LogWarning($"RobotManager: speech_command_parsed parse failed: {ex.Message}");
        }
    }

    private void HandleLatencyUpdated(float latencyMs)
    {
        latestLatencyMs = latencyMs;
        if (mainThreadContext != null && SynchronizationContext.Current != mainThreadContext)
        {
            mainThreadContext.Post(_ => OnLatencyUpdated?.Invoke(latencyMs), null);
            return;
        }

        OnLatencyUpdated?.Invoke(latencyMs);
    }

    private void UpdateMap(TelemetryData data)
    {
        if (mapManager != null)
        {
            mapManager.UpdateRobotPosition(data.posX, data.posY);
            mapManager.PlacePin(data.posX, data.posY, data.victimStatus);
        }

    }

    private void UpdateHUD(TelemetryData data)
    {
        if (uiManager != null)
        {
            uiManager.UpdateHUD(data);
        }
    }

    private void UpdateAcousticBeam(float acousticAngle)
    {
        if (acousticBeamManager == null)
        {
            return;
        }

        acousticBeamManager.UpdateAcousticBeamAngle(acousticAngle);
        acousticBeamManager.ShowAcousticBeam();
    }

    private void HideAcousticBeam()
    {
        if (acousticBeamManager != null)
        {
            acousticBeamManager.HideAcousticBeam();
        }
    }

    private void UpdateVideoFrame(byte[] jpegBytes)
    {
        if (uiManager != null)
        {
            uiManager.UpdateVideoFrame(jpegBytes);
        }
    }

    private void UpdateUartDebug(UartDebugData data)
    {
        if (uiManager != null)
        {
            uiManager.UpdateUartDebug(data);
        }
    }

    private void UpdateConnectionState(NetworkConnectionState state)
    {
        if (uiManager != null)
        {
            uiManager.UpdateConnectionState(state);
        }
    }

    private void UpdateSpeechCommandParsed(SpeechCommandParsedData data)
    {
        if (uiManager != null)
        {
            uiManager.UpdateSpeechCommandParsed(data);
        }
    }

    private void UpdateLatency(float latencyMs)
    {
        if (uiManager != null)
        {
            uiManager.UpdateLatency(latencyMs);
        }
    }

    private void UpdatePttAmplitude(float amplitude)
    {
        if (uiManager != null)
        {
            uiManager.UpdatePttAmplitude(amplitude);
        }
    }

    public void PlayMockReplay()
    {
        if (networkClient is FileNetworkClient fileClient)
        {
            fileClient.Play();
        }
    }

    public void PauseMockReplay()
    {
        if (networkClient is FileNetworkClient fileClient)
        {
            fileClient.Pause();
        }
    }

    public void StopMockReplay()
    {
        if (networkClient is FileNetworkClient fileClient)
        {
            fileClient.Stop();
        }
    }

    private void HandleAudioBlobReady(byte[] wavData)
    {
        if (networkClient == null)
        {
            return;
        }

        networkClient.SendAudioBlob(wavData);
    }

    private void HandleAudioCaptureStateChanged(AudioCaptureState state)
    {
        if (uiManager != null)
        {
            uiManager.UpdatePTTState(state);
        }
    }

    private TelemetryData DeserializeTelemetry(string telemetryJson)
    {
        TelemetryWirePayload payload = JsonUtility.FromJson<TelemetryWirePayload>(telemetryJson);
        TelemetryData data = new TelemetryData
        {
            posX = payload.HasUnderscoreCoordinates ? payload.pos_x : payload.posX,
            posY = payload.HasUnderscoreCoordinates ? payload.pos_y : payload.posY,
            temperature = payload.HasShortTemperature ? payload.temp : payload.temperature,
            smokeDetected = payload.smokeDetected || payload.smoke_detected || payload.smoke,
            isStuck = payload.isStuck || payload.is_stuck,
            priorityLevel = payload.HasPriority ? payload.priority_level : payload.priorityLevel,
            acousticHit = payload.acousticHit || payload.acoustic_hit,
            acousticAngle = Mathf.Abs(payload.acoustic_angle) > Mathf.Epsilon
                ? payload.acoustic_angle
                : payload.acousticAngle,
            networkLatencyMs = latestLatencyMs
        };

        string victimStatusText = ExtractJsonString(telemetryJson, "victimStatus");
        if (string.IsNullOrEmpty(victimStatusText))
        {
            victimStatusText = ExtractJsonString(telemetryJson, "victim_status");
        }

        if (!string.IsNullOrEmpty(victimStatusText))
        {
            data.victimStatus = ParseVictimStatus(victimStatusText);
        }
        else
        {
            data.victimStatus = payload.victimStatus;
        }

        return data;
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

        int valueEnd = json.IndexOf('"', valueStart + 1);
        if (valueEnd <= valueStart)
        {
            return string.Empty;
        }

        return json.Substring(valueStart + 1, valueEnd - valueStart - 1);
    }

    private static VictimStatus ParseVictimStatus(string rawStatus)
    {
        if (Enum.TryParse(rawStatus, true, out VictimStatus parsedStatus))
        {
            return parsedStatus;
        }

        return VictimStatus.None;
    }

    [Serializable]
    private struct TelemetryWirePayload
    {
        public float posX;
        public float posY;
        public float temperature;
        public bool smokeDetected;
        public VictimStatus victimStatus;
        public bool isStuck;
        public int priorityLevel;
        public bool acousticHit;
        public float acousticAngle;

        public float pos_x;
        public float pos_y;
        public float temp;
        public bool smoke;
        public bool smoke_detected;
        public bool is_stuck;
        public int priority_level;
        public bool acoustic_hit;
        public float acoustic_angle;

        public bool HasUnderscoreCoordinates => Mathf.Abs(pos_x) > Mathf.Epsilon || Mathf.Abs(pos_y) > Mathf.Epsilon;
        public bool HasShortTemperature => Mathf.Abs(temp) > Mathf.Epsilon;
        public bool HasPriority => priority_level != 0;
    }
}
