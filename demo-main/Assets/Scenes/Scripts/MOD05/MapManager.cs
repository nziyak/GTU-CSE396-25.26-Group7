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

public enum MapCoordinatePlane
{
    XY = 0,
    XZ = 1
}

/// Consumes RobotManager.OnTelemetryUpdated via event-driven fan-out.
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

    [Header("Pin Colors")]
    [SerializeField] private Color trappedPinColor = new Color(1f, 0.05f, 0.02f, 1f);
    [SerializeField] private Color lyingPinColor = new Color(1f, 0.45f, 0f, 1f);
    [SerializeField] private Color standingPinColor = new Color(1f, 0.92f, 0.1f, 1f);

    [Header("Map Conversion")]
    [SerializeField] private MapCoordinatePlane coordinatePlane = MapCoordinatePlane.XZ;
    [SerializeField] private Vector2 mapOrigin;
    [SerializeField] private float unitsPerGridCell = 1f;
    [SerializeField] private float robotMoveSpeed = 4f;
    [SerializeField] private float robotHeight = 0.3f;
    [SerializeField] private bool deferPinsUntilRobotArrives = true;
    [SerializeField] private float pinPlacementDistance = 0.15f;
    [SerializeField] private float pinTtlSeconds = 300f;
    [SerializeField] private bool replacePinAtSameCell = true;

    private readonly List<GameObject> spawnedPins = new List<GameObject>();
    private readonly Dictionary<string, GameObject> pinsByCell = new Dictionary<string, GameObject>();
    private readonly Dictionary<string, float> pinLastUpdatedByCell = new Dictionary<string, float>();
    private readonly List<string> expiredCellBuffer = new List<string>(32);
    private MaterialPropertyBlock pinPropertyBlock;
    private Vector3 targetRobotPosition;
    private bool hasRobotTarget;
    private float nextPinCleanupTime;
    private bool hasPendingPin;
    private float pendingPinX;
    private float pendingPinY;
    private VictimStatus pendingPinStatus;

    public Transform RobotMarker => robotMarker;

    private void OnValidate()
    {
        if (unitsPerGridCell <= 0f)
        {
            unitsPerGridCell = 1f;
        }
    }

    private void Awake()
    {
        pinPropertyBlock = new MaterialPropertyBlock();
    }

    public void AutoBindMapElements()
    {
        if (mapRoot == null)
        {
            GameObject mapRootObject = GameObject.Find("MapRoot");
            mapRoot = mapRootObject != null ? mapRootObject.transform : transform;
        }

        if (pinParent == null)
        {
            GameObject pinParentObject = GameObject.Find("PinParent");
            pinParent = pinParentObject != null ? pinParentObject.transform : transform;
        }

        if (robotMarker == null)
        {
            GameObject robotMarkerObject = GameObject.Find("RobotMarker");
            robotMarker = robotMarkerObject != null ? robotMarkerObject.transform : null;
        }

        if (robotMarker == null)
        {
            Debug.LogError("MapManager: RobotMarker could not be auto-bound. Create a GameObject named 'RobotMarker'.");
        }
    }

    private void Update()
    {
        if (robotMarker != null && hasRobotTarget)
        {
            robotMarker.position = Vector3.MoveTowards(
                robotMarker.position,
                targetRobotPosition,
                robotMoveSpeed * Time.deltaTime
            );

            if (hasPendingPin && Vector3.Distance(robotMarker.position, targetRobotPosition) <= pinPlacementDistance)
            {
                PlacePinImmediate(pendingPinX, pendingPinY, pendingPinStatus);
                hasPendingPin = false;
            }
        }

        if (Time.time >= nextPinCleanupTime)
        {
            nextPinCleanupTime = Time.time + 5f;
            ClearExpiredCells();
        }
    }

    /// 2D Map color-coded.
    /// Pin colour is determined by VictimStatus:
    ///   Trapped  -> Red
    ///   Lying    -> Yellow
    ///   Standing -> Green
    ///   None     -> no pin placed
    /// <param name="posX">Robot's X position on the 2D grid (from TelemetryData)</param>
    /// <param name="posY">Robot's Y position on the 2D grid (from TelemetryData)</param>
    /// <param name="status">AI sınıflandırmalı => victim status (from TelemetryData.victimStatus)</param>
    public void PlacePin(float posX, float posY, VictimStatus status)
    {
        if (deferPinsUntilRobotArrives && robotMarker != null && hasRobotTarget)
        {
            hasPendingPin = true;
            pendingPinX = posX;
            pendingPinY = posY;
            pendingPinStatus = status;
            return;
        }

        PlacePinImmediate(posX, posY, status);
    }

    private void PlacePinImmediate(float posX, float posY, VictimStatus status)
    {
        GameObject pinPrefab = ResolvePinPrefab(status);
        if (pinPrefab == null)
        {
            return;
        }

        Vector3 spawnPosition = GridToWorldPosition(posX, posY);
        string cellKey = BuildCellKey(posX, posY);

        if (pinsByCell.TryGetValue(cellKey, out GameObject existingPin) && existingPin != null)
        {
            pinLastUpdatedByCell[cellKey] = Time.time;
            if (existingPin.name.StartsWith($"VictimPin_{status}_", System.StringComparison.Ordinal))
            {
                ApplyPinColor(existingPin, status);
                return;
            }

            if (!replacePinAtSameCell)
            {
                return;
            }

            spawnedPins.Remove(existingPin);
            Destroy(existingPin);
        }

        Transform parent = pinParent != null ? pinParent : transform;
        GameObject pinInstance = Instantiate(pinPrefab, spawnPosition, Quaternion.identity, parent);
        pinInstance.name = $"VictimPin_{status}_{cellKey}";
        ApplyPinColor(pinInstance, status);

        spawnedPins.Add(pinInstance);
        pinsByCell[cellKey] = pinInstance;
        pinLastUpdatedByCell[cellKey] = Time.time;
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

        targetRobotPosition = GridToWorldPosition(posX, posY, robotHeight);
        hasRobotTarget = true;
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
        pinLastUpdatedByCell.Clear();
    }

    public void ClearExpiredCells()
    {
        ClearExpiredCells(pinTtlSeconds);
    }

    public void ClearExpiredCells(float maxAgeSeconds)
    {
        float now = Time.time;
        expiredCellBuffer.Clear();

        foreach (KeyValuePair<string, float> entry in pinLastUpdatedByCell)
        {
            if (now - entry.Value >= maxAgeSeconds)
            {
                expiredCellBuffer.Add(entry.Key);
            }
        }

        for (int i = 0; i < expiredCellBuffer.Count; i++)
        {
            string cellKey = expiredCellBuffer[i];
            if (pinsByCell.TryGetValue(cellKey, out GameObject pin) && pin != null)
            {
                spawnedPins.Remove(pin);
                Destroy(pin);
            }

            pinsByCell.Remove(cellKey);
            pinLastUpdatedByCell.Remove(cellKey);
        }
    }

    /// Resolves the correct pin prefab (Red/Yellow/Green) based on VictimStatus.
    /// Returns null for VictimStatus.None; caller must guard against null.
    /// <param name="status">Victim status to resolve</param>
    /// <returns>Matching pin GameObject prefab, or null if status is None</returns>
    private GameObject ResolvePinPrefab(VictimStatus status)
    {
        switch (status)
        {
            case VictimStatus.Trapped:
                return redPinPrefab != null ? redPinPrefab : fallbackPinPrefab;
            case VictimStatus.Lying:
                return yellowPinPrefab != null ? yellowPinPrefab : fallbackPinPrefab;
            case VictimStatus.Standing:
                return greenPinPrefab != null ? greenPinPrefab : fallbackPinPrefab;
            case VictimStatus.None:
                return null;
            default:
                return fallbackPinPrefab;
        }
    }

    private void ApplyPinColor(GameObject pinInstance, VictimStatus status)
    {
        Color color = ResolvePinColor(status);

        SpriteRenderer spriteRenderer = pinInstance.GetComponentInChildren<SpriteRenderer>();
        if (spriteRenderer != null)
        {
            spriteRenderer.color = color;
        }

        Renderer renderer = pinInstance.GetComponentInChildren<Renderer>();
        if (renderer != null)
        {
            if (pinPropertyBlock == null)
            {
                pinPropertyBlock = new MaterialPropertyBlock();
            }

            renderer.GetPropertyBlock(pinPropertyBlock);
            pinPropertyBlock.SetColor("_Color", color);
            renderer.SetPropertyBlock(pinPropertyBlock);
        }
    }

    private Color ResolvePinColor(VictimStatus status)
    {
        switch (status)
        {
            case VictimStatus.Trapped:
                return trappedPinColor;
            case VictimStatus.Lying:
                return lyingPinColor;
            case VictimStatus.Standing:
                return standingPinColor;
            default:
                return Color.white;
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
        return GridToWorldPosition(posX, posY, MapManagerConstants.MAP_PIN_HEIGHT);
    }

    private Vector3 GridToWorldPosition(float posX, float posY, float height)
    {
        Vector3 anchorPosition = mapRoot != null ? mapRoot.position : transform.position;
        float worldX = anchorPosition.x + mapOrigin.x + (posX * unitsPerGridCell);
        float gridY = mapOrigin.y + (posY * unitsPerGridCell);

        if (coordinatePlane == MapCoordinatePlane.XZ)
        {
            return new Vector3(worldX, anchorPosition.y + height, anchorPosition.z + gridY);
        }

        return new Vector3(worldX, anchorPosition.y + gridY, anchorPosition.z + height);
    }

    private static string BuildCellKey(float posX, float posY)
    {
        return $"{Mathf.RoundToInt(posX)}_{Mathf.RoundToInt(posY)}";
    }
}
