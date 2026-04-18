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
    }

    private void Start()
    {
        networkClient = new WebSocketClient();
        networkClient.OnTelemetryReceived += HandleTelemetryReceived;

        if (audioManager != null)
        {
            audioManager.SetNetworkClient(networkClient);
            audioManager.OnAudioBlobReady += HandleAudioBlobReady;
        }

        if (connectOnStart)
        {
            Connect();
        }
    }

    private void OnDestroy()
    {
        if (audioManager != null)
        {
            audioManager.OnAudioBlobReady -= HandleAudioBlobReady;
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
            return;
        }

        networkClient.Connect(serverUrl);
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
        }

        audioManager = manager;

        if (audioManager != null)
        {
            audioManager.SetNetworkClient(networkClient);
            audioManager.OnAudioBlobReady += HandleAudioBlobReady;
        }
    }

    private void HandleTelemetryReceived(TelemetryData data)
    {
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
}
