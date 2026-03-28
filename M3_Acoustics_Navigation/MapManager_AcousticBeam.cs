/// <summary>
/// File:    MapManager_AcousticBeam.cs
/// Brief:   MOD-03 Unity MapManager — Acoustic Beam Visualization Interface
/// Author:  Dicle Çoban [Öğrenci No Yaz]
/// Date:    2026-03-28
/// Version: 0.1
///
/// Changelog:
/// v0.1 (2026-03-28) - Initial draft: ShowAcousticBeam, HideAcousticBeam,
///                     UpdateAcousticBeamAngle stubs defined.
///
/// Consumed by: Unity RobotManager / UIManager (Ziya's INetworkClient layer)
/// Depends on:  DataContracts.cs (TelemetryData, VictimStatus) 
///              INetworkClient.cs (OnTelemetryReceived)          
///              MapManager.cs (PlacePin, UpdateRobotPosition)   
/// </summary>

using UnityEngine;

// -- Constants ---------------------------------------------------------------

public static class AcousticBeamConstants
{
    public const float BEAM_ARROW_LENGTH    = 1.5f;  // Visual arrow length in Unity world units
    public const float BEAM_SWEEP_DURATION  = 0.8f;  // Radar sweep animation duration (seconds)
}

// -- Data Types --------------------------------------------------------------

/// <summary>
/// Acoustic beam display style on the Unity map.
/// </summary>
public enum AcousticBeamStyle
{
    DirectionArrow = 0,  // Single directional arrow toward sound source
    RadarSweep     = 1   // Animated radar-style arc sweep
}

/// <summary>
/// Data required to render one acoustic beam event on the Unity map.
/// Populated from TelemetryData received via INetworkClient.OnTelemetryReceived.
/// </summary>
[System.Serializable]
public class AcousticBeamData
{
    /// <summary> Bearing angle from STM32 (-180.0 to +180.0 degrees) </summary>
    public float bearingDeg;

    /// <summary> True if A_Hit=1 was present in the telemetry packet </summary>
    public bool hitDetected;

    /// <summary> Robot's X position on the 2D grid (from TelemetryData.posX) </summary>
    public float posX;

    /// <summary> Robot's Y position on the 2D grid (from TelemetryData.posY) </summary>
    public float posY;

    /// <summary> Timestamp of detection event (ms since boot) </summary>
    public uint timestampMs;
}

// -- Public Interface --------------------------------------------------------

/// <summary>
/// MapManager_AcousticBeam — Visualizes MOD-03 acoustic bearing data on the Unity map.
/// Attach alongside MapManager.cs on the MapManager GameObject.
/// Subscribe to INetworkClient.OnTelemetryReceived (Ziya) and call
/// ShowAcousticBeam() when telemetry contains A_Hit = true.
/// </summary>
public class MapManager_AcousticBeam : MonoBehaviour
{
    /// <summary>
    /// Render the acoustic bearing as a visual indicator on the Unity map.
    /// Called by RobotManager when telemetry contains A_Hit = true.
    /// Pin color follows VictimStatus convention from DataContracts.cs:
    ///   TRAPPED  → Red  |  LYING → Yellow  |  STANDING → Green
    /// </summary>
    /// <param name="data">Acoustic beam data from the telemetry packet</param>
    /// <param name="style">Visual style to use (DirectionArrow or RadarSweep)</param>
    public void ShowAcousticBeam(AcousticBeamData data, AcousticBeamStyle style) { /* TODO: [Dicle] */ }

    /// <summary>
    /// Clear and hide the acoustic beam indicator from the map.
    /// Called when FSM exits ACOUSTIC_HOMING state.
    /// </summary>
    public void HideAcousticBeam() { /* TODO: [Dicle] */ }

    /// <summary>
    /// Update the beam direction without creating a new indicator.
    /// Used when bearing is refined by multiple consecutive readings.
    /// </summary>
    /// <param name="newBearingDeg">Updated bearing angle in degrees (-180.0 to +180.0)</param>
    public void UpdateAcousticBeamAngle(float newBearingDeg) { /* TODO: [Dicle] */ }
}
