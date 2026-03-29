/// File:    MapManager.cs
/// Brief:   2D Map Pin Placement and Robot Position Tracking for MOD-05 Unity Digital Twin
/// Version: 0.2 (Ceng_Integration)
/// [R12] Docstrings updated to reflect new TelemetryData field names (docx §2A)

using UnityEngine;

public static class MapManagerConstants
{
    public const float MAP_PIN_HEIGHT      = 0.1f; // Z-offset so pins render above the map layer
    public const int   PIN_PRIORITY_RED    = 1;    // TRAPPED  — highest priority
    public const int   PIN_PRIORITY_YELLOW = 2;    // LYING    — medium priority
    public const int   PIN_PRIORITY_GREEN  = 3;    // STANDING — low priority
}

/// [R12] Work with INetworkClient.OnTelemetryReceived - Nuri Ziya made it
/// and forward data.pos_x / data.pos_y / data.victim_status / data.priority to the relevant methods.
/// (Original: posX/posY/victimStatus/priorityLevel → updated per docx §2A JSON keys)
public class MapManager : MonoBehaviour
{
    /// 2D Map color-coded. 
    /// Pin colour is determined by VictimStatus:
    ///   TRAPPED  → Red    (priority 1)
    ///   LYING    → Yellow (priority 2)
    ///   STANDING → Green  (priority 3)
    ///   NONE     → no pin placed
    /// <param name="posX">Robot's X position on the 2D grid (from TelemetryData)</param>
    /// <param name="posY">Robot's Y position on the 2D grid (from TelemetryData)</param>
    /// <param name="status">AI-classified victim status (from TelemetryData.victimStatus)</param>
    public void PlacePin(float posX, float posY, VictimStatus status) { }

    /// Moves the robot marker to the latest X-Y coordinates received from telemetry.
    /// Called every time a new TelemetryData packet arrives.
    
    /// <param name="posX">Robot's current X position on the 2D grid</param>
    /// <param name="posY">Robot's current Y position on the 2D grid</param>
    public void UpdateRobotPosition(float posX, float posY) { }


    /// Removes all pins currently placed on the map.
    /// Useful for mission reset or new run start.
    public void ClearAllPins() { }

    /// Resolves the correct pin prefab (Red/Yellow/Green) based on VictimStatus.
    /// Returns null for VictimStatus.NONE — caller must guard against null.
    /// <param name="status">Victim status to resolve</param>
    /// <returns>Matching pin GameObject prefab, or null if status is NONE</returns>
    private GameObject ResolvePinPrefab(VictimStatus status) { return null; }

    /// Converts raw grid coordinates to Unity world-space Vector3 position.
    /// Applies MAP_PIN_HEIGHT as Z-offset to ensure pins render above the map.
    /// <param name="posX">Grid X coordinate</param>
    /// <param name="posY">Grid Y coordinate</param>
    /// <returns>World-space Vector3 for Instantiate placement</returns>
    private Vector3 GridToWorldPosition(float posX, float posY) { return Vector3.zero; }
}
