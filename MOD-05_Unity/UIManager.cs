/// File:    UIManager.cs
/// Brief:   HUD Display for Temperature, Smoke and Victim Status in MOD-05 Unity Digital Twin

using System.Collections;
using UnityEngine;
using UnityEngine.UI;

public static class UIManagerConstants
{
    public const string SMOKE_DETECTED_TEXT = "SMOKE DETECTED";  // Warning label shown on HUD
    public const string SMOKE_CLEAR_TEXT    = "SMOKE CLEAR";     // Normal label shown on HUD
    public const string ROBOT_STUCK_TEXT    = "ROBOT STUCK";     // MOD-01 IMU stuck warning
    public const string ROBOT_OK_TEXT       = "ROBOT MOBILITY OK";
    public const string TEMPERATURE_UNIT    = "°C";
}

/// UIManager — Updates the HUD overlay with live telemetry for MOD-05 Unity Digital Twin.
/// Consumes RobotManager.OnTelemetryUpdated through UpdateHUD().
/// Alternatively, pass the full packet to UpdateHUD() to refresh all fields at once.
public class UIManager : MonoBehaviour
{
    [Header("HUD Labels")]
    [SerializeField] private Text temperatureText;
    [SerializeField] private Text smokeStatusText;
    [SerializeField] private Text victimStatusText;
    [SerializeField] private Text stuckStatusText;
    [SerializeField] private Text connectionStatusText;
    [SerializeField] private Text latencyText;
    [SerializeField] private Text pttStateText;
    [SerializeField] private RawImage videoFrameImage;

    [Header("Feedback Widgets")]
    [SerializeField] private Slider pttAmplitudeSlider;
    [SerializeField] private Slider temperatureGauge;
    [SerializeField] private RectTransform pttAmplitudeScaleTarget;
    [SerializeField] private Graphic temperatureGlowPanel;
    [SerializeField] private Graphic stuckAlarmPanel;
    [SerializeField] private Graphic smokeAlarmPanel;

    [Header("HUD Colors")]
    [SerializeField] private Color normalTextColor = Color.white;
    [SerializeField] private Color coldTemperatureColor = new Color(0.05f, 0.55f, 1f, 0.35f);
    [SerializeField] private Color hotTemperatureColor = new Color(1f, 0.08f, 0.02f, 0.85f);
    [SerializeField] private Color smokeWarningColor = Color.red;
    [SerializeField] private Color stuckWarningColor = new Color(1f, 0.55f, 0f);
    [SerializeField] private Color connectedColor = Color.green;
    [SerializeField] private Color connectingColor = Color.yellow;
    [SerializeField] private Color disconnectedColor = Color.red;

    [Header("PTT Labels")]
    [SerializeField] private string idlePttText = string.Empty;
    [SerializeField] private string activePttText = "Recording...";

    [Header("Hazard Thresholds")]
    [SerializeField] private float minGaugeTemperature = 0f;
    [SerializeField] private float criticalTemperature = 50f;

    private Texture2D videoTexture;
    private Coroutine stuckBlinkCoroutine;
    private Coroutine smokeBlinkCoroutine;

    public void AutoBindDashboardElements()
    {
        temperatureText = temperatureText != null ? temperatureText : FindText("TemperatureText");
        smokeStatusText = smokeStatusText != null ? smokeStatusText : FindText("SmokeStatusText");
        victimStatusText = victimStatusText != null ? victimStatusText : FindText("VictimStatusText");
        stuckStatusText = stuckStatusText != null ? stuckStatusText : FindText("StuckStatusText");
        connectionStatusText = connectionStatusText != null ? connectionStatusText : FindText("ConnectionStatusText");
        latencyText = latencyText != null ? latencyText : FindText("LatencyText");
        pttStateText = pttStateText != null ? pttStateText : FindText("PTTStateText");

        videoFrameImage = videoFrameImage != null ? videoFrameImage : FindComponent<RawImage>("VideoFrameRawImage");
        pttAmplitudeSlider = pttAmplitudeSlider != null ? pttAmplitudeSlider : FindComponent<Slider>("PTTAmplitudeSlider");
        temperatureGauge = temperatureGauge != null ? temperatureGauge : FindComponent<Slider>("TemperatureGauge");
        pttAmplitudeScaleTarget = pttAmplitudeScaleTarget != null
            ? pttAmplitudeScaleTarget
            : FindTransform<RectTransform>("PTTAmplitudeScaleTarget");

        temperatureGlowPanel = temperatureGlowPanel != null ? temperatureGlowPanel : FindComponent<Graphic>("TemperatureGlowPanel");
        stuckAlarmPanel = stuckAlarmPanel != null ? stuckAlarmPanel : FindComponent<Graphic>("StuckAlarmPanel");
        smokeAlarmPanel = smokeAlarmPanel != null ? smokeAlarmPanel : FindComponent<Graphic>("SmokeAlarmPanel");

        if (temperatureText == null)
        {
            Debug.LogError("UIManager: TemperatureText could not be auto-bound.");
        }

        if (videoFrameImage == null)
        {
            Debug.LogError("UIManager: VideoFrameRawImage could not be auto-bound.");
        }
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
        UpdateThermalGauge(temperature);
    }

    /// Updates the smoke status indicator on the HUD.
    /// Shows SMOKE_DETECTED_TEXT (red) when true, SMOKE_CLEAR_TEXT (white) when false.
    /// <param name="smokeDetected">True if smoke threshold is exceeded (from TelemetryData)</param>
    public void UpdateSmokeStatus(bool smokeDetected)
    {
        SetBlinkingAlarm(smokeDetected, smokeAlarmPanel, ref smokeBlinkCoroutine, smokeWarningColor);

        if (smokeStatusText == null)
        {
            return;
        }

        smokeStatusText.text = smokeDetected
            ? UIManagerConstants.SMOKE_DETECTED_TEXT
            : UIManagerConstants.SMOKE_CLEAR_TEXT;
        smokeStatusText.color = smokeDetected ? smokeWarningColor : normalTextColor;
    }

    /// Updates the victim status label on the HUD.
    /// Displays the enum name as a string ("TRAPPED", "LYING", "STANDING", "NONE").
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

    /// <summary>Updates the MOD-01 stuck alert shown to the operator.</summary>
    public void UpdateStuckStatus(bool isStuck)
    {
        SetBlinkingAlarm(isStuck, stuckAlarmPanel, ref stuckBlinkCoroutine, stuckWarningColor);

        if (stuckStatusText == null)
        {
            return;
        }

        stuckStatusText.text = isStuck
            ? UIManagerConstants.ROBOT_STUCK_TEXT
            : UIManagerConstants.ROBOT_OK_TEXT;
        stuckStatusText.color = isStuck ? stuckWarningColor : normalTextColor;
    }

    /// Convenience method — refreshes all HUD fields from a single TelemetryData packet.
    /// Internal flow:
    ///   1. UpdateTemperature(data.temperature)
    ///   2. UpdateSmokeStatus(data.smokeDetected)
    ///   3. UpdateVictimStatus(data.victimStatus)
    /// <param name="data">Full telemetry packet distributed by RobotManager.OnTelemetryUpdated</param>
    public void UpdateHUD(TelemetryData data)
    {
        UpdateTemperature(data.temperature);
        UpdateSmokeStatus(data.smokeDetected);
        UpdateVictimStatus(data.victimStatus);
        UpdateStuckStatus(data.isStuck);
    }

    /// <summary>
    /// Renders compressed JPEG frames from MOD-04 on the main thread without creating a new texture per frame.
    /// </summary>
    public void UpdateVideoFrame(byte[] jpegBytes)
    {
        if (videoFrameImage == null || jpegBytes == null || jpegBytes.Length == 0)
        {
            return;
        }

        if (videoTexture == null)
        {
            videoTexture = new Texture2D(2, 2, TextureFormat.RGB24, false);
            videoFrameImage.texture = videoTexture;
        }

        if (!videoTexture.LoadImage(jpegBytes, false))
        {
            Debug.LogWarning("UIManager: Failed to decode MOD-04 JPEG frame.");
        }
    }

    public void UpdateConnectionState(NetworkConnectionState state)
    {
        if (connectionStatusText == null)
        {
            return;
        }

        connectionStatusText.text = state.ToString().ToUpperInvariant();
        switch (state)
        {
            case NetworkConnectionState.Connected:
                connectionStatusText.color = connectedColor;
                break;
            case NetworkConnectionState.Connecting:
                connectionStatusText.color = connectingColor;
                break;
            default:
                connectionStatusText.color = disconnectedColor;
                break;
        }
    }

    public void UpdateLatency(float latencyMs)
    {
        if (latencyText != null)
        {
            latencyText.text = $"{latencyMs:0} ms";
        }
    }

    public void UpdatePttAmplitude(float amplitude)
    {
        float normalized = Mathf.Clamp01(amplitude);
        if (pttAmplitudeSlider != null)
        {
            pttAmplitudeSlider.value = normalized;
        }

        if (pttAmplitudeScaleTarget != null)
        {
            float scale = Mathf.Lerp(0.85f, 1.2f, normalized);
            pttAmplitudeScaleTarget.localScale = new Vector3(scale, scale, 1f);
        }
    }

    public void UpdateThermalGauge(float temperature)
    {
        float normalized = Mathf.InverseLerp(minGaugeTemperature, criticalTemperature, temperature);
        Color gaugeColor = Color.Lerp(coldTemperatureColor, hotTemperatureColor, normalized);

        if (temperatureGauge != null)
        {
            temperatureGauge.value = normalized;
            Image fillImage = temperatureGauge.fillRect != null
                ? temperatureGauge.fillRect.GetComponent<Image>()
                : null;

            if (fillImage != null)
            {
                fillImage.color = gaugeColor;
            }
        }

        if (temperatureGlowPanel != null)
        {
            float glowAlpha = temperature >= criticalTemperature ? 0.85f : Mathf.Lerp(0.1f, 0.45f, normalized);
            gaugeColor.a = glowAlpha;
            temperatureGlowPanel.color = gaugeColor;
        }
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
    /// Trapped  -> Color.red
    /// Lying    -> Color.yellow
    /// Standing -> Color.green
    /// None     -> Color.white
    /// <param name="status">Victim status to resolve</param>
    /// <returns>Unity Color matching the priority of the status</returns>
    private Color ResolveVictimStatusColor(VictimStatus status)
    {
        switch (status)
        {
            case VictimStatus.Trapped:
                return Color.red;
            case VictimStatus.Lying:
                return Color.yellow;
            case VictimStatus.Standing:
                return Color.green;
            default:
                return normalTextColor;
        }
    }

    private void SetBlinkingAlarm(bool active, Graphic panel, ref Coroutine coroutine, Color color)
    {
        if (panel == null)
        {
            return;
        }

        if (active)
        {
            if (coroutine == null)
            {
                coroutine = StartCoroutine(BlinkAlarm(panel, color));
            }

            return;
        }

        if (coroutine != null)
        {
            StopCoroutine(coroutine);
            coroutine = null;
        }

        Color clear = color;
        clear.a = 0f;
        panel.color = clear;
    }

    private IEnumerator BlinkAlarm(Graphic panel, Color color)
    {
        Color visible = color;
        Color hidden = color;
        visible.a = 0.8f;
        hidden.a = 0.15f;

        while (true)
        {
            panel.color = visible;
            yield return new WaitForSeconds(0.35f);
            panel.color = hidden;
            yield return new WaitForSeconds(0.35f);
        }
    }

    private static Text FindText(string objectName)
    {
        return FindComponent<Text>(objectName);
    }

    private static T FindComponent<T>(string objectName) where T : Component
    {
        GameObject target = GameObject.Find(objectName);
        return target != null ? target.GetComponent<T>() : null;
    }

    private static T FindTransform<T>(string objectName) where T : Component
    {
        GameObject target = GameObject.Find(objectName);
        return target != null ? target.GetComponent<T>() : null;
    }
}
