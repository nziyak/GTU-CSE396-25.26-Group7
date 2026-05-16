/// <summary>
/// File:    MapManager_AcousticBeam.cs
/// Brief:   MOD-03 Unity MapManager — Acoustic Beam Visualization Interface
/// Author:  Dicle Çoban [220104004088]
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
    public const float BEAM_ARROW_LENGTH   = 1.5f;  // Visual arrow length in Unity world units
    public const float BEAM_SWEEP_DURATION = 0.8f;  // Radar sweep animation duration (seconds)
    public const float BEAM_ARROW_HEAD_DEG = 28f;   // Arrow-head opening angle in degrees
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
    [Header("Beam Setup")]
    [SerializeField] private Transform mapRoot;
    [SerializeField] private Transform beamParent;
    [SerializeField] private float unitsPerGridCell = 1f;
    [SerializeField] private Vector2 mapOrigin;
    [SerializeField] private float beamHeight = 0.15f;
    [SerializeField] private float bearingOffsetDeg;
    [SerializeField] private float visibleHoldSeconds = 1.5f;

    [Header("Beam Visuals")]
    [SerializeField] private float beamWidth = 0.08f;
    [SerializeField] private float arrowHeadLength = 0.28f;
    [SerializeField] private Color arrowColor = Color.cyan;
    [SerializeField] private Color sweepColor = Color.green;
    [SerializeField] private Material beamMaterial;

    private GameObject beamObject;
    private LineRenderer beamRenderer;
    private AcousticBeamStyle currentStyle;
    private Vector3 beamStartPosition;
    private float currentBearingDeg;
    private float sweepTimer;
    private float hideAtTime;
    private bool hidePending;

    /// <summary>
    /// Render the acoustic bearing as a visual indicator on the Unity map.
    /// Called by RobotManager when telemetry contains A_Hit = true.
    /// Pin color follows VictimStatus convention from DataContracts.cs:
    ///   TRAPPED  → Red  |  LYING → Yellow  |  STANDING → Green
    /// </summary>
    /// <param name="data">Acoustic beam data from the telemetry packet</param>
    /// <param name="style">Visual style to use (DirectionArrow or RadarSweep)</param>
    public void ShowAcousticBeam(AcousticBeamData data, AcousticBeamStyle style)
    {
        if (data == null || !data.hitDetected)
        {
            HideAcousticBeam();
            return;
        }

        EnsureBeamRenderer();

        bool wasActive = beamObject.activeSelf;
        AcousticBeamStyle previousStyle = currentStyle;

        currentStyle = style;
        currentBearingDeg = data.bearingDeg;
        beamStartPosition = GridToWorldPosition(data.posX, data.posY);
        hidePending = false;

        if (!wasActive || previousStyle != style)
        {
            sweepTimer = 0f;
        }

        beamObject.SetActive(true);
        beamObject.name = $"AcousticBeam_{style}_{data.timestampMs}";

        UpdateBeamVisual(currentBearingDeg);
    }

    /// <summary>
    /// Clear and hide the acoustic beam indicator from the map.
    /// Called when FSM exits ACOUSTIC_HOMING state.
    /// </summary>
    public void HideAcousticBeam()
    {
        if (visibleHoldSeconds > 0f)
        {
            if (!hidePending)
            {
                hidePending = true;
                hideAtTime = Time.time + visibleHoldSeconds;
            }

            return;
        }

        if (beamObject != null)
        {
            beamObject.SetActive(false);
        }

        hidePending = false;
    }

    /// <summary>
    /// Update the beam direction without creating a new indicator.
    /// Used when bearing is refined by multiple consecutive readings.
    /// </summary>
    /// <param name="newBearingDeg">Updated bearing angle in degrees (-180.0 to +180.0)</param>
    public void UpdateAcousticBeamAngle(float newBearingDeg)
    {
        currentBearingDeg = newBearingDeg;

        if (beamObject == null || !beamObject.activeSelf)
        {
            return;
        }

        UpdateBeamVisual(currentBearingDeg);
    }

    private void Update()
    {
        if (beamObject == null || !beamObject.activeSelf || currentStyle != AcousticBeamStyle.RadarSweep)
        {
            if (hidePending && Time.time >= hideAtTime)
            {
                HideImmediately();
            }

            return;
        }

        if (hidePending && Time.time >= hideAtTime)
        {
            HideImmediately();
            return;
        }

        sweepTimer += Time.deltaTime;
        float sweepOffset = Mathf.Sin((sweepTimer / AcousticBeamConstants.BEAM_SWEEP_DURATION) * Mathf.PI * 2f) * 20f;
        UpdateBeamVisual(currentBearingDeg + sweepOffset);
    }

    private void EnsureBeamRenderer()
    {
        if (beamObject != null && beamRenderer != null)
        {
            return;
        }

        if (beamObject == null)
        {
            beamObject = new GameObject("AcousticBeam");
            Transform parent = beamParent != null ? beamParent : transform;
            beamObject.transform.SetParent(parent, false);
        }

        beamRenderer = beamObject.GetComponent<LineRenderer>();
        if (beamRenderer == null)
        {
            beamRenderer = beamObject.AddComponent<LineRenderer>();
        }

        beamRenderer.useWorldSpace = true;
        beamRenderer.startWidth = beamWidth;
        beamRenderer.endWidth = beamWidth * 0.35f;
        beamRenderer.numCapVertices = 4;
        beamRenderer.material = beamMaterial != null
            ? beamMaterial
            : new Material(Shader.Find("Sprites/Default"));
    }

    private void UpdateBeamVisual(float bearingDeg)
    {
        if (beamRenderer == null)
        {
            return;
        }

        Vector3 direction = BearingToDirection(bearingDeg + bearingOffsetDeg);
        Vector3 endPosition = beamStartPosition + direction * AcousticBeamConstants.BEAM_ARROW_LENGTH;

        beamRenderer.startColor = currentStyle == AcousticBeamStyle.RadarSweep ? sweepColor : arrowColor;
        beamRenderer.endColor = currentStyle == AcousticBeamStyle.RadarSweep
            ? new Color(sweepColor.r, sweepColor.g, sweepColor.b, 0.15f)
            : new Color(arrowColor.r, arrowColor.g, arrowColor.b, 0.45f);

        if (currentStyle == AcousticBeamStyle.DirectionArrow)
        {
            Vector3 headLeft = endPosition - RotateDirection(direction, AcousticBeamConstants.BEAM_ARROW_HEAD_DEG) * arrowHeadLength;
            Vector3 headRight = endPosition - RotateDirection(direction, -AcousticBeamConstants.BEAM_ARROW_HEAD_DEG) * arrowHeadLength;

            beamRenderer.positionCount = 5;
            beamRenderer.SetPosition(0, beamStartPosition);
            beamRenderer.SetPosition(1, endPosition);
            beamRenderer.SetPosition(2, headLeft);
            beamRenderer.SetPosition(3, endPosition);
            beamRenderer.SetPosition(4, headRight);
            return;
        }

        beamRenderer.positionCount = 2;
        beamRenderer.SetPosition(0, beamStartPosition);
        beamRenderer.SetPosition(1, endPosition);
    }

    private Vector3 GridToWorldPosition(float posX, float posY)
    {
        Vector3 anchorPosition = mapRoot != null ? mapRoot.position : transform.position;
        float worldX = anchorPosition.x + mapOrigin.x + (posX * unitsPerGridCell);
        float worldY = anchorPosition.y + mapOrigin.y + (posY * unitsPerGridCell);
        float worldZ = anchorPosition.z + beamHeight;
        return new Vector3(worldX, worldY, worldZ);
    }

    private static Vector3 BearingToDirection(float bearingDeg)
    {
        float radians = bearingDeg * Mathf.Deg2Rad;
        return new Vector3(Mathf.Cos(radians), Mathf.Sin(radians), 0f).normalized;
    }

    private static Vector3 RotateDirection(Vector3 direction, float degrees)
    {
        float radians = degrees * Mathf.Deg2Rad;
        float cos = Mathf.Cos(radians);
        float sin = Mathf.Sin(radians);

        return new Vector3(
            direction.x * cos - direction.y * sin,
            direction.x * sin + direction.y * cos,
            0f
        ).normalized;
    }

    private void HideImmediately()
    {
        if (beamObject != null)
        {
            beamObject.SetActive(false);
        }

        hidePending = false;
    }
}
