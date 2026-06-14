/// <summary>
/// File:    MapManager_AcousticBeam.cs
/// Brief:   Renders the acoustic distress-call direction as a lightweight beam.
/// </summary>

using UnityEngine;

public class MapManager_AcousticBeam : MonoBehaviour
{
    [Header("Beam")]
    [SerializeField] private LineRenderer beamRenderer;
    [SerializeField] private AcousticBeamVisualizer beamVisualizer;
    [SerializeField] private Transform beamOrigin;
    [SerializeField] private float beamLength = 5f;
    [SerializeField] private float timeoutSeconds = 30f;
    [SerializeField] private bool visibleOnStart = false;

    private float currentAngle;
    private float lastShowTime = -999f;

    private void Awake()
    {
        AutoBindBeamComponents();

        if (visibleOnStart)
        {
            ShowAcousticBeam();
        }
        else
        {
            HideAcousticBeam();
        }
    }

    public void AutoBindBeamComponents()
    {
        if (beamRenderer == null)
        {
            beamRenderer = GetComponent<LineRenderer>();
        }

        if (beamVisualizer == null)
        {
            beamVisualizer = GetComponent<AcousticBeamVisualizer>();
        }

        if (beamRenderer != null)
        {
            beamRenderer.positionCount = 2;
            beamRenderer.useWorldSpace = true;
        }
    }

    /// <summary>Shows the beam using the latest angle supplied by telemetry.</summary>
    public void ShowAcousticBeam()
    {
        if (beamRenderer == null)
        {
            if (beamVisualizer != null)
            {
                beamVisualizer.Show(currentAngle);
            }

            return;
        }

        beamRenderer.enabled = true;
        if (beamVisualizer != null)
        {
            beamVisualizer.Show(currentAngle);
        }

        lastShowTime = Time.time;
        RefreshBeamGeometry();
    }

    /// <summary>
    /// Backward-compatible overload for older RobotManager versions that pass
    /// AcousticBeamData and AcousticBeamStyle directly.
    /// </summary>
    public void ShowAcousticBeam(AcousticBeamData data, AcousticBeamStyle style)
    {
        if (data == null || !data.hitDetected)
        {
            HideAcousticBeam();
            return;
        }

        currentAngle = data.bearingDeg;
        ShowAcousticBeam();
    }

    /// <summary>Hides the beam without destroying scene objects.</summary>
    public void HideAcousticBeam()
    {
        if (beamRenderer != null)
        {
            beamRenderer.enabled = false;
        }

        if (beamVisualizer != null)
        {
            beamVisualizer.Hide();
        }
    }

    /// <summary>Updates the rendered beam angle in degrees.</summary>
    public void UpdateAcousticBeamAngle(float angle)
    {
        currentAngle = angle;

        if (beamRenderer != null && beamRenderer.enabled)
        {
            lastShowTime = Time.time;
            RefreshBeamGeometry();
        }

        if (beamVisualizer != null)
        {
            beamVisualizer.SetAngle(currentAngle);
        }
    }

    private void Update()
    {
        if (beamRenderer == null || !beamRenderer.enabled)
        {
            return;
        }

        if (Time.time - lastShowTime >= timeoutSeconds)
        {
            HideAcousticBeam();
        }
    }

    private void RefreshBeamGeometry()
    {
        Transform originTransform = beamOrigin != null ? beamOrigin : transform;
        Vector3 start = originTransform.position;
        Quaternion rotation = Quaternion.Euler(0f, 0f, currentAngle);
        Vector3 end = start + (rotation * Vector3.right * beamLength);

        beamRenderer.SetPosition(0, start);
        beamRenderer.SetPosition(1, end);
    }
}
