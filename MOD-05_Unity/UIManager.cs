/// File:    UIManager.cs
/// Brief:   HUD Display for Temperature, Smoke and Victim Status in MOD-05 Unity Digital Twin

using UnityEngine;
using UnityEngine.UI;

public static class UIManagerConstants
{
    public const string SMOKE_DETECTED_TEXT    = "SMOKE DETECTED";  // Warning label shown on HUD
    public const string SMOKE_CLEAR_TEXT       = "SMOKE CLEAR";     // Normal label shown on HUD
    public const string TEMPERATURE_UNIT       = "°C";              
}


/// UIManager — Updates the HUD overlay with live telemetry for MOD-05 Unity Digital Twin.
/// Work with INetworkClient.OnTelemetryReceived - Nuri Ziya made it
/// and forward the relevant fields to UpdateTemperature, UpdateSmokeStatus, UpdateVictimStatus.
/// Alternatively, pass the full packet to UpdateHUD() to refresh all fields at once.
public class UIManager : MonoBehaviour
{

    /// Updates the temperature label on the HUD.
    /// Displays value in Celsius appended with the degree symbol (e.g., "37.2 °C").
    /// <param name="temperature">Current temperature in Celsius (from TelemetryData)</param>
    public void UpdateTemperature(float temperature) { }

    /// Updates the smoke status indicator on the HUD.
    /// Shows SMOKE_DETECTED_TEXT (red) when true, SMOKE_CLEAR_TEXT (white) when false.
    /// <param name="smokeDetected">True if smoke threshold is exceeded (from TelemetryData)</param>
    public void UpdateSmokeStatus(bool smokeDetected) { }

    /// Updates the victim status label on the HUD.
    /// Displays the enum name as a string ("TRAPPED", "LYING", "STANDING", "NONE") -> I explained in MapManager
    /// <param name="status">AI-classified victim status (from TelemetryData.victimStatus)</param>
    public void UpdateVictimStatus(VictimStatus status) { }

    /// Convenience method — refreshes all HUD fields from a single TelemetryData packet.
    /// Internal flow:
    ///   1. UpdateTemperature(data.temperature)
    ///   2. UpdateSmokeStatus(data.smokeDetected)
    ///   3. UpdateVictimStatus(data.victimStatus)
    /// <param name="data">Full telemetry packet received from INetworkClient.OnTelemetryReceived</param>
    public void UpdateHUD(TelemetryData data) { }

    /// Updates the PTT recording state indicator on the HUD.
    /// Subscribe to AudioManager.OnCaptureStateChanged and forward the state here.
    /// Displays a "Recording..." label while in Recording or Encoding state.
    /// <param name="state">Current AudioCaptureState (from AudioManager.OnCaptureStateChanged)</param>
    public void UpdatePTTState(AudioCaptureState state) { }

    /// Resolves the HUD highlight colour for a given VictimStatus.
    /// TRAPPED  → Color.red
    /// LYING    → Color.yellow
    /// STANDING → Color.green
    /// NONE     → Color.white
    /// <param name="status">Victim status to resolve</param>
    /// <returns>Unity Color matching the priority of the status</returns>
    private Color ResolveVictimStatusColor(VictimStatus status) { return Color.white; }
}
