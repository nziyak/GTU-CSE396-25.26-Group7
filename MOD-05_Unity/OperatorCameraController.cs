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
    [SerializeField] private bool followTargetOnStart = false;

    [Header("Orbit")]
    [SerializeField] private float orbitSpeed = 160f;
    [SerializeField] private float minPitch = 15f;
    [SerializeField] private float maxPitch = 80f;

    [Header("Pan / Zoom")]
    [SerializeField] private float panSpeed = 0.02f;
    [SerializeField] private float keyboardPanSpeed = 8f;
    [SerializeField] private float zoomSpeed = 4f;
    [SerializeField] private float minDistance = 3f;
    [SerializeField] private float maxDistance = 45f;
    [SerializeField] private float minCameraHeight = 1.2f;
    [SerializeField] private float maxPanRadius = 40f;

    private float yaw;
    private float pitch = 45f;
    private float distance = 10f;
    private Vector3 focusPoint;
    private bool followTarget;

    private void Start()
    {
        followTarget = followTargetOnStart;
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
        if (Input.GetKeyDown(KeyCode.F))
        {
            FocusOnTarget();
        }

        if (Input.GetKeyDown(KeyCode.G))
        {
            followTarget = !followTarget;
        }

        if (orbitTarget != null && followTarget)
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
            followTarget = false;
            ClampFocusPoint();
        }

        HandleKeyboardPan();

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

    public void FocusOnTarget()
    {
        if (orbitTarget == null)
        {
            return;
        }

        focusPoint = orbitTarget.position + targetOffset;
        followTarget = true;
    }

    private void HandleKeyboardPan()
    {
        float horizontal = Input.GetAxisRaw("Horizontal");
        float vertical = Input.GetAxisRaw("Vertical");

        if (Mathf.Approximately(horizontal, 0f) && Mathf.Approximately(vertical, 0f))
        {
            return;
        }

        Vector3 forward = Vector3.ProjectOnPlane(transform.forward, Vector3.up).normalized;
        Vector3 right = Vector3.ProjectOnPlane(transform.right, Vector3.up).normalized;
        Vector3 movement = (right * horizontal) + (forward * vertical);
        focusPoint += movement.normalized * (keyboardPanSpeed * Time.deltaTime);
        followTarget = false;
        ClampFocusPoint();
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
