/// File:    MapManager.cs
/// Brief:   2D Map Pin Placement and Robot Position Tracking for MOD-05 Unity Digital Twin

using System.Collections.Generic;
using UnityEngine;

public static class MapManagerConstants
{
    public const float MAP_PIN_HEIGHT      = 0.1f; // Z-offset so pins render above the map layer
    public const int   PIN_PRIORITY_RED    = 1;    // TRAPPED  — highest priority
    public const int   PIN_PRIORITY_YELLOW = 2;    // LYING    — medium priority
    public const int   PIN_PRIORITY_GREEN  = 3;    // STANDING — low priority
}

/// Work with INetworkClient.OnTelemetryReceived - Nuri Ziya made it
/// and forward posX/posY/victimStatus/priorityLevel to the relevant methods.
public class MapManager : MonoBehaviour
{
    [Header("Scene References")]
    [SerializeField] private Transform mapRoot;
    [SerializeField] private Transform pinParent;
    [SerializeField] private Transform robotMarker;

    [Header("Pin Prefabs")]
    [SerializeField] private GameObject redPinPrefab;
    [SerializeField] private GameObject yellowPinPrefab;
    [SerializeField] private GameObject greenPinPrefab;
    [SerializeField] private GameObject fallbackPinPrefab;

    [Header("Map Conversion")]
    [SerializeField] private Vector2 mapOrigin;
    [SerializeField] private float unitsPerGridCell = 1f;
    [SerializeField] private bool replacePinAtSameCell = true;

    private readonly List<GameObject> spawnedPins = new List<GameObject>();
    private readonly Dictionary<string, GameObject> pinsByCell = new Dictionary<string, GameObject>();

    /// 2D Map color-coded.
    /// Pin colour is determined by VictimStatus:
    ///   TRAPPED  → Red    (priorityLevel 1)
    ///   LYING    → Yellow (priorityLevel 2)
    ///   STANDING → Green  (priorityLevel 3)
    ///   NONE     → no pin placed
    /// <param name="posX">Robot's X position on the 2D grid (from TelemetryData)</param>
    /// <param name="posY">Robot's Y position on the 2D grid (from TelemetryData)</param>
    /// <param name="status">AI sınıflandırmalı => victim status (from TelemetryData.victimStatus)</param>
    public void PlacePin(float posX, float posY, VictimStatus status)
    {
        GameObject pinPrefab = ResolvePinPrefab(status);
        if (pinPrefab == null)
        {
            return;
        }

        Vector3 spawnPosition = GridToWorldPosition(posX, posY);
        string cellKey = BuildCellKey(posX, posY);

        if (replacePinAtSameCell && pinsByCell.TryGetValue(cellKey, out GameObject existingPin) && existingPin != null)
        {
            spawnedPins.Remove(existingPin);
            Destroy(existingPin);
        }

        Transform parent = pinParent != null ? pinParent : transform;
        GameObject pinInstance = Instantiate(pinPrefab, spawnPosition, Quaternion.identity, parent);
        pinInstance.name = $"VictimPin_{status}_{cellKey}";

        spawnedPins.Add(pinInstance);
        pinsByCell[cellKey] = pinInstance;
    }

    /// Moves the robot marker to the latest X-Y coordinates received from telemetry.
    /// Called every time a new TelemetryData packet arrives.
    ///
    /// <param name="posX">Robot's current X position on the 2D grid</param>
    /// <param name="posY">Robot's current Y position on the 2D grid</param>
    public void UpdateRobotPosition(float posX, float posY)
    {
        if (robotMarker == null)
        {
            return;
        }

        robotMarker.position = GridToWorldPosition(posX, posY);
    }

    /// Removes all pins currently placed on the map.
    /// Useful for mission reset or new run start.
    public void ClearAllPins()
    {
        for (int i = spawnedPins.Count - 1; i >= 0; i--)
        {
            if (spawnedPins[i] != null)
            {
                Destroy(spawnedPins[i]);
            }
        }

        spawnedPins.Clear();
        pinsByCell.Clear();
    }

    /// Resolves the correct pin prefab (Red/Yellow/Green) based on VictimStatus.
    /// Returns null for VictimStatus.NONE — caller must guard against null.
    /// <param name="status">Victim status to resolve</param>
    /// <returns>Matching pin GameObject prefab, or null if status is NONE</returns>
    private GameObject ResolvePinPrefab(VictimStatus status)
    {
        switch (status)
        {
            case VictimStatus.TRAPPED:
                return redPinPrefab != null ? redPinPrefab : fallbackPinPrefab;
            case VictimStatus.LYING:
                return yellowPinPrefab != null ? yellowPinPrefab : fallbackPinPrefab;
            case VictimStatus.STANDING:
                return greenPinPrefab != null ? greenPinPrefab : fallbackPinPrefab;
            case VictimStatus.NONE:
                return null;
            default:
                return fallbackPinPrefab;
        }
    }

    /// Converts raw grid coordinates to Unity world-space Vector3 position.
    /// Applies MAP_PIN_HEIGHT as Z-offset to ensure pins render above the map.
    /// <param name="posX">Grid X coordinate</param>
    /// <param name="posY">Grid Y coordinate</param>
    /// <returns>World-space Vector3 for Instantiate placement</returns>
    private Vector3 GridToWorldPosition(float posX, float posY)
    {
        Vector3 anchorPosition = mapRoot != null ? mapRoot.position : transform.position;
        float worldX = anchorPosition.x + mapOrigin.x + (posX * unitsPerGridCell);
        float worldY = anchorPosition.y + mapOrigin.y + (posY * unitsPerGridCell);
        float worldZ = anchorPosition.z + MapManagerConstants.MAP_PIN_HEIGHT;
        return new Vector3(worldX, worldY, worldZ);
    }

    private static string BuildCellKey(float posX, float posY)
    {
        return $"{Mathf.RoundToInt(posX)}_{Mathf.RoundToInt(posY)}";
    }
}
