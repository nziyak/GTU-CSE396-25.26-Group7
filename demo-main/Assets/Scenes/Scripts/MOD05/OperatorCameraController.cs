/// <summary>
/// File:    OperatorCameraController.cs
/// Brief:   Orbit, pan and zoom controls for the MOD-05 operator scene camera.
/// </summary>

using UnityEngine;

public class OperatorCameraController : MonoBehaviour
{
    [Header("Target")]
    [SerializeField] private Transform orbitTarget;
    [SerializeField] private Vector3 targetOffset = Vector3.up;

    [Header("Orbit")]
    [SerializeField] private float orbitSpeed = 160f;
    [SerializeField] private float minPitch = 15f;
    [SerializeField] private float maxPitch = 80f;

    [Header("Pan / Zoom")]
    [SerializeField] private float panSpeed = 0.02f;
    [SerializeField] private float zoomSpeed = 4f;
    [SerializeField] private float minDistance = 3f;
    [SerializeField] private float maxDistance = 25f;
    [SerializeField] private float minCameraHeight = 1.2f;
    [SerializeField] private float maxPanRadius = 18f;

    private float yaw;
    private float pitch = 45f;
    private float distance = 10f;
    private Vector3 focusPoint;

    private void Start()
    {
        focusPoint = orbitTarget != null ? orbitTarget.position + targetOffset : Vector3.zero;
        Vector3 offset = transform.position - focusPoint;
        distance = Mathf.Clamp(offset.magnitude, minDistance, maxDistance);
        Vector3 angles = transform.eulerAngles;
        yaw = angles.y;
        pitch = Mathf.Clamp(angles.x, minPitch, maxPitch);
        ApplyCameraPose();
    }

    private void LateUpdate()
    {
        if (orbitTarget != null)
        {
            focusPoint = Vector3.Lerp(focusPoint, orbitTarget.position + targetOffset, Time.deltaTime * 4f);
        }

        if (Input.GetMouseButton(0))
        {
            yaw += Input.GetAxis("Mouse X") * orbitSpeed * Time.deltaTime;
            pitch -= Input.GetAxis("Mouse Y") * orbitSpeed * Time.deltaTime;
            pitch = Mathf.Clamp(pitch, minPitch, maxPitch);
        }

        if (Input.GetMouseButton(1))
        {
            Vector3 pan = (-transform.right * Input.GetAxis("Mouse X")) + (-transform.up * Input.GetAxis("Mouse Y"));
            focusPoint += pan * (panSpeed * distance);
            ClampFocusPoint();
        }

        float scroll = Input.GetAxis("Mouse ScrollWheel");
        if (Mathf.Abs(scroll) > Mathf.Epsilon)
        {
            distance = Mathf.Clamp(distance - (scroll * zoomSpeed * distance), minDistance, maxDistance);
        }

        ApplyCameraPose();
    }

    public void SetTarget(Transform target)
    {
        orbitTarget = target;
        if (orbitTarget != null)
        {
            focusPoint = orbitTarget.position + targetOffset;
        }
    }

    private void ApplyCameraPose()
    {
        pitch = Mathf.Clamp(pitch, minPitch, maxPitch);
        distance = Mathf.Clamp(distance, minDistance, maxDistance);
        ClampFocusPoint();

        Quaternion rotation = Quaternion.Euler(pitch, yaw, 0f);
        Vector3 nextPosition = focusPoint - (rotation * Vector3.forward * distance);
        if (nextPosition.y < minCameraHeight)
        {
            nextPosition.y = minCameraHeight;
        }

        transform.position = nextPosition;
        transform.rotation = rotation;
    }

    private void ClampFocusPoint()
    {
        Vector3 center = orbitTarget != null ? orbitTarget.position + targetOffset : Vector3.zero;
        Vector3 offset = focusPoint - center;
        if (offset.magnitude > maxPanRadius)
        {
            focusPoint = center + offset.normalized * maxPanRadius;
        }
    }
}
