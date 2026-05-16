/// <summary>
/// File:    RobotManager.cs
/// Brief:   Scene-level coordinator for MOD-05 Unity Digital Twin
/// Author:  Dicle Coban
/// Date:    2026-04-18
/// Version: 0.1
///
/// Notes:
/// - Owns the INetworkClient lifecycle.
/// - Forwards telemetry to MapManager, UIManager, and acoustic beam visualization.
/// - Bridges AudioManager output to the network layer.
/// </summary>

using UnityEngine;

public class RobotManager : MonoBehaviour
{
    [Header("Connection")]
    [SerializeField] private string serverUrl = "ws://192.168.1.10:5000";
    [SerializeField] private bool connectOnStart = true;

    [Header("Mocking (Demo Only)")]
    [SerializeField] private bool useMockFileData = false;
    [SerializeField] private string mockFileName = "mock_telemetry.json";
    [SerializeField] private float mockUpdateInterval = 1.0f;

    [Header("Managers")]
    [SerializeField] private MapManager mapManager;
    [SerializeField] private UIManager uiManager;
    [SerializeField] private MapManager_AcousticBeam acousticBeamManager;

    [Header("Acoustic Beam")]
    [SerializeField] private AcousticBeamStyle beamStyle = AcousticBeamStyle.DirectionArrow;

    private INetworkClient networkClient;
    private AudioManager audioManager;

    private void Awake()
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

        if (audioManager == null)
        {
            audioManager = new AudioManager();
        }
    }

    private void Start()
    {
        if (useMockFileData)
        {
            Debug.Log("RobotManager: Using mock file data mode.");
            networkClient = new FileNetworkClient(mockUpdateInterval);

            // Check for potential interference and automatically disable it
            MockTelemetryTester tester = FindObjectOfType<MockTelemetryTester>();
            if (tester != null && tester.enabled)
            {
                Debug.LogWarning("RobotManager: Automatically disabling MockTelemetryTester to prevent interference with file-based mock data.");
                tester.enabled = false;
            }
        }
        else
        {
            Debug.Log("RobotManager: Using real WebSocket client mode.");
            networkClient = new WebSocketClient();
        }

        networkClient.OnTelemetryReceived += HandleTelemetryReceived;

        if (audioManager != null)
        {
            audioManager.OnAudioBlobReady += HandleAudioBlobReady;
            audioManager.OnCaptureStateChanged += HandleAudioCaptureStateChanged;
        }

        if (connectOnStart || useMockFileData)
        {
            Connect();
        }
    }

    private void OnDestroy()
    {
        if (audioManager != null)
        {
            audioManager.OnAudioBlobReady -= HandleAudioBlobReady;
            audioManager.OnCaptureStateChanged -= HandleAudioCaptureStateChanged;
        }

        if (networkClient != null)
        {
            networkClient.OnTelemetryReceived -= HandleTelemetryReceived;
            networkClient.Disconnect();
        }
    }

    public void Connect()
    {
        if (networkClient == null)
        {
            Debug.LogError("RobotManager: networkClient is null in Connect().");
            return;
        }

        if (useMockFileData)
        {
            Debug.Log($"RobotManager: Connecting to mock file '{mockFileName}'...");
            networkClient.Connect(mockFileName);
        }
        else
        {
            Debug.Log($"RobotManager: Connecting to server URL '{serverUrl}'...");
            networkClient.Connect(serverUrl);
        }
    }

    public void Disconnect()
    {
        if (networkClient == null)
        {
            return;
        }

        networkClient.Disconnect();
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
            audioManager.OnAudioBlobReady += HandleAudioBlobReady;
            audioManager.OnCaptureStateChanged += HandleAudioCaptureStateChanged;
        }
    }

    private void HandleTelemetryReceived(TelemetryData data)
    {
        Debug.Log($"RobotManager: Received telemetry. Pos: ({data.posX}, {data.posY}), Status: {data.victimStatus}");
        
        if (mapManager != null)
        {
            mapManager.UpdateRobotPosition(data.posX, data.posY);
            mapManager.PlacePin(data.posX, data.posY, data.victimStatus);
        }

        if (uiManager != null)
        {
            uiManager.UpdateHUD(data);
        }

        if (acousticBeamManager != null)
        {
            if (data.acousticHit)
            {
                AcousticBeamData beamData = new AcousticBeamData
                {
                    bearingDeg = data.acousticAngle,
                    hitDetected = true,
                    posX = data.posX,
                    posY = data.posY,
                    timestampMs = (uint)(Time.time * 1000f)
                };

                acousticBeamManager.ShowAcousticBeam(beamData, beamStyle);
            }
            else
            {
                acousticBeamManager.HideAcousticBeam();
            }
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
        if (uiManager == null)
        {
            return;
        }

        uiManager.UpdatePTTState(state);
    }
}
