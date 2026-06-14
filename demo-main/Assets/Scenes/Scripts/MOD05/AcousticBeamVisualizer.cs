/// <summary>
/// File:    AcousticBeamVisualizer.cs
/// Brief:   Presentation-grade pulsing/fading LineRenderer for MOD-03 acoustic bearing.
/// </summary>

using UnityEngine;

[RequireComponent(typeof(LineRenderer))]
public class AcousticBeamVisualizer : MonoBehaviour
{
    [Header("References")]
    [SerializeField] private Transform beamOrigin;
    [SerializeField] private ParticleSystem beamParticles;

    [Header("Shape")]
    [SerializeField] private float beamLength = 6f;
    [SerializeField] private float baseWidth = 0.08f;
    [SerializeField] private float pulseWidth = 0.06f;
    [SerializeField] private float pulseSpeed = 5f;
    [SerializeField] private float fadeSpeed = 4f;

    [Header("Color")]
    [SerializeField] private Color beamColor = new Color(0.1f, 0.85f, 1f, 0.95f);
    [SerializeField] private Color beamTipColor = new Color(0.1f, 0.85f, 1f, 0.1f);

    private LineRenderer lineRenderer;
    private float targetAlpha;
    private float currentAlpha;
    private float currentAngle;

    private void Awake()
    {
        lineRenderer = GetComponent<LineRenderer>();
        if (beamParticles == null)
        {
            beamParticles = GetComponentInChildren<ParticleSystem>();
        }

        lineRenderer.positionCount = 2;
        lineRenderer.useWorldSpace = true;
        lineRenderer.numCapVertices = 8;
        lineRenderer.enabled = false;

        if (lineRenderer.material == null)
        {
            lineRenderer.material = new Material(Shader.Find("Sprites/Default"));
        }
    }

    private void Update()
    {
        currentAlpha = Mathf.MoveTowards(currentAlpha, targetAlpha, fadeSpeed * Time.deltaTime);
        if (currentAlpha <= 0.001f && targetAlpha <= 0f)
        {
            lineRenderer.enabled = false;
            return;
        }

        lineRenderer.enabled = true;
        float pulse = (Mathf.Sin(Time.time * pulseSpeed) + 1f) * 0.5f;
        lineRenderer.startWidth = baseWidth + (pulseWidth * pulse);
        lineRenderer.endWidth = (baseWidth * 0.2f) + (pulseWidth * 0.2f * pulse);

        Color start = beamColor;
        Color end = beamTipColor;
        start.a *= currentAlpha;
        end.a *= currentAlpha;
        lineRenderer.startColor = start;
        lineRenderer.endColor = end;

        RefreshGeometry();
    }

    public void Show(float angleDegrees)
    {
        currentAngle = angleDegrees;
        targetAlpha = 1f;
        lineRenderer.enabled = true;
        if (beamParticles != null && !beamParticles.isPlaying)
        {
            beamParticles.Play();
        }

        RefreshGeometry();
    }

    public void SetAngle(float angleDegrees)
    {
        currentAngle = angleDegrees;
        if (lineRenderer.enabled)
        {
            RefreshGeometry();
        }
    }

    public void Hide()
    {
        targetAlpha = 0f;
        if (beamParticles != null && beamParticles.isPlaying)
        {
            beamParticles.Stop(true, ParticleSystemStopBehavior.StopEmitting);
        }
    }

    private void RefreshGeometry()
    {
        Transform originTransform = beamOrigin != null ? beamOrigin : transform;
        Vector3 start = originTransform.position;
        Vector3 direction = Quaternion.Euler(0f, currentAngle, 0f) * Vector3.forward;
        Vector3 end = start + (direction.normalized * beamLength);

        lineRenderer.SetPosition(0, start);
        lineRenderer.SetPosition(1, end);
    }
}
