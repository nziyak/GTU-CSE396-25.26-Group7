/// File:    UIManager.cs
/// Brief:   HUD Display for Temperature, Smoke and Victim Status in MOD-05 Unity Digital Twin

using UnityEngine;
using UnityEngine.UI;

public static class UIManagerConstants
{
    public const string SMOKE_DETECTED_TEXT = "SMOKE DETECTED";  // Warning label shown on HUD
    public const string SMOKE_CLEAR_TEXT    = "SMOKE CLEAR";     // Normal label shown on HUD
    public const string STUCK_DETECTED_TEXT = "ROBOT STUCK";
    public const string STUCK_CLEAR_TEXT    = "MOTION NORMAL";
    public const string TEMPERATURE_UNIT    = "°C";
}

/// UIManager — Updates the HUD overlay with live telemetry for MOD-05 Unity Digital Twin.
/// Work with INetworkClient.OnTelemetryReceived - Nuri Ziya made it
/// and forward the relevant fields to UpdateTemperature, UpdateSmokeStatus, UpdateVictimStatus.
/// Alternatively, pass the full packet to UpdateHUD() to refresh all fields at once.
public class UIManager : MonoBehaviour
{
    [Header("HUD Labels")]
    [SerializeField] private Text temperatureText;
    [SerializeField] private Text smokeStatusText;
    [SerializeField] private Text stuckStatusText;
    [SerializeField] private Text victimStatusText;
    [SerializeField] private Text pttStateText;

    [Header("HUD Colors")]
    [SerializeField] private Color normalTextColor = Color.white;
    [SerializeField] private Color smokeWarningColor = Color.red;

    [Header("PTT Labels")]
    [SerializeField] private string idlePttText = string.Empty;
    [SerializeField] private string activePttText = "Recording...";

    private void Awake()
    {
        AutoBindMissingLabels();
    }

    /// Updates the temperature label on the HUD.
    /// Displays value in Celsius appended with the degree symbol (e.g., "37.2 °C").
    /// <param name="temperature">Current temperature in Celsius (from TelemetryData)</param>
    public void UpdateTemperature(float temperature)
    {
        if (temperatureText == null)
        {
            return;
        }

        temperatureText.text = $"{temperature:0.0} {UIManagerConstants.TEMPERATURE_UNIT}";
    }

    /// Updates the smoke status indicator on the HUD.
    /// Shows SMOKE_DETECTED_TEXT (red) when true, SMOKE_CLEAR_TEXT (white) when false.
    /// <param name="smokeDetected">True if smoke threshold is exceeded (from TelemetryData)</param>
    public void UpdateSmokeStatus(bool smokeDetected)
    {
        if (smokeStatusText == null)
        {
            return;
        }

        smokeStatusText.text = smokeDetected
            ? UIManagerConstants.SMOKE_DETECTED_TEXT
            : UIManagerConstants.SMOKE_CLEAR_TEXT;
        smokeStatusText.color = smokeDetected ? smokeWarningColor : normalTextColor;
    }

    /// Updates the stuck-status indicator on the HUD.
    /// Shows a warning when the robot reports a physical stuck condition.
    /// <param name="isStuck">True if robot motion is blocked despite motor activity</param>
    public void UpdateStuckStatus(bool isStuck)
    {
        if (stuckStatusText == null)
        {
            return;
        }

        stuckStatusText.text = isStuck
            ? UIManagerConstants.STUCK_DETECTED_TEXT
            : UIManagerConstants.STUCK_CLEAR_TEXT;
        stuckStatusText.color = isStuck ? smokeWarningColor : normalTextColor;
    }

    /// Updates the victim status label on the HUD.
    /// Displays the enum name as a string ("TRAPPED", "LYING", "STANDING", "NONE") -> I explained in MapManager
    /// <param name="status">AI-classified victim status (from TelemetryData.victimStatus)</param>
    public void UpdateVictimStatus(VictimStatus status)
    {
        if (victimStatusText == null)
        {
            return;
        }

        victimStatusText.text = status.ToString().ToUpperInvariant();
        victimStatusText.color = ResolveVictimStatusColor(status);
    }

    /// Convenience method — refreshes all HUD fields from a single TelemetryData packet.
    /// Internal flow:
    ///   1. UpdateTemperature(data.temperature)
    ///   2. UpdateSmokeStatus(data.smokeDetected)
    ///   3. UpdateStuckStatus(data.isStuck)
    ///   4. UpdateVictimStatus(data.victimStatus)
    /// <param name="data">Full telemetry packet received from INetworkClient.OnTelemetryReceived</param>
    public void UpdateHUD(TelemetryData data)
    {
        UpdateTemperature(data.temperature);
        UpdateSmokeStatus(data.smokeDetected);
        UpdateStuckStatus(data.isStuck);
        UpdateVictimStatus(data.victimStatus);
    }

    /// Updates the PTT recording state indicator on the HUD.
    /// Subscribe to AudioManager.OnCaptureStateChanged and forward the state here.
    /// Displays a "Recording..." label while in Recording or Encoding state.
    /// <param name="state">Current AudioCaptureState (from AudioManager.OnCaptureStateChanged)</param>
    public void UpdatePTTState(AudioCaptureState state)
    {
        if (pttStateText == null)
        {
            return;
        }

        bool isActive = state == AudioCaptureState.Recording || state == AudioCaptureState.Encoding;
        pttStateText.text = isActive ? activePttText : idlePttText;
        pttStateText.color = isActive ? smokeWarningColor : normalTextColor;
    }

    /// Resolves the HUD highlight colour for a given VictimStatus.
    /// TRAPPED  → Color.red
    /// LYING    → Color.yellow
    /// STANDING → Color.green
    /// NONE     → Color.white
    /// <param name="status">Victim status to resolve</param>
    /// <returns>Unity Color matching the priority of the status</returns>
    private Color ResolveVictimStatusColor(VictimStatus status)
    {
        switch (status)
        {
            case VictimStatus.TRAPPED:
                return Color.red;
            case VictimStatus.LYING:
                return Color.yellow;
            case VictimStatus.STANDING:
                return Color.green;
            default:
                return normalTextColor;
        }
    }

    private void AutoBindMissingLabels()
    {
        if (temperatureText == null)
        {
            temperatureText = FindTextByName("TemperatureText", "Temperature");
        }

        if (smokeStatusText == null)
        {
            smokeStatusText = FindTextByName("SmokeStatusText", "SmokeStatus");
        }

        if (stuckStatusText == null)
        {
            stuckStatusText = FindTextByName("StuckStatusText", "RobotStatusText", "StuckStatus");
        }

        if (victimStatusText == null)
        {
            victimStatusText = FindTextByName("VictimStatusText", "VictimStatus");
        }

        if (pttStateText == null)
        {
            pttStateText = FindTextByName("PTTStateText", "PushToTalkText", "PTTStatus");
        }
    }

    private Text FindTextByName(params string[] names)
    {
        Text[] labels = FindObjectsOfType<Text>(true);
        for (int i = 0; i < labels.Length; i++)
        {
            for (int j = 0; j < names.Length; j++)
            {
                if (labels[i].name == names[j])
                {
                    return labels[i];
                }
            }
        }

        return null;
    }
}
