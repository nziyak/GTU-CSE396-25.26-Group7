/// <summary>
/// File:    RobotVisualAnimator.cs
/// Brief:   Drives the imported robot visual Animator from RobotMarker movement.
/// </summary>

using UnityEngine;

public class RobotVisualAnimator : MonoBehaviour
{
    [Header("References")]
    [SerializeField] private Transform movementSource;
    [SerializeField] private Animator animator;

    [Header("Animator State Names")]
    [SerializeField] private string idleStateName = "Idle";
    [SerializeField] private string walkStateName = "Walk";

    [Header("Movement Detection")]
    [SerializeField] private float movingThreshold = 0.015f;
    [SerializeField] private float animationCrossFadeSeconds = 0.12f;
    [SerializeField] private bool rotateVisualToMovement = true;
    [SerializeField] private float rotationSmoothness = 10f;
    [SerializeField] private float modelForwardYawOffset = 0f;

    private Vector3 lastSourcePosition;
    private bool wasMoving;
    private bool initialized;

    private void Awake()
    {
        AutoBind();
        InitializePosition();
    }

    private void OnEnable()
    {
        AutoBind();
        InitializePosition();
        PlayState(idleStateName);
    }

    private void Update()
    {
        if (movementSource == null || animator == null)
        {
            AutoBind();
            if (movementSource == null || animator == null)
            {
                return;
            }
        }

        Vector3 currentPosition = movementSource.position;
        Vector3 delta = currentPosition - lastSourcePosition;
        delta.y = 0f;

        bool isMoving = delta.magnitude > movingThreshold;
        if (isMoving != wasMoving)
        {
            PlayState(isMoving ? walkStateName : idleStateName);
            wasMoving = isMoving;
        }

        if (rotateVisualToMovement && isMoving)
        {
            RotateToward(delta);
        }

        lastSourcePosition = currentPosition;
        initialized = true;
    }

    [ContextMenu("Auto Bind Robot Visual Animator")]
    public void AutoBind()
    {
        if (movementSource == null)
        {
            movementSource = transform.parent != null ? transform.parent : transform;
        }

        if (animator == null)
        {
            animator = GetComponent<Animator>();
        }
    }

    private void InitializePosition()
    {
        if (initialized || movementSource == null)
        {
            return;
        }

        lastSourcePosition = movementSource.position;
        initialized = true;
    }

    private void PlayState(string stateName)
    {
        if (animator == null || string.IsNullOrWhiteSpace(stateName))
        {
            return;
        }

        animator.CrossFadeInFixedTime(stateName, animationCrossFadeSeconds);
    }

    private void RotateToward(Vector3 movementDelta)
    {
        if (movementDelta.sqrMagnitude <= 0.0001f)
        {
            return;
        }

        Quaternion targetRotation = Quaternion.LookRotation(movementDelta.normalized, Vector3.up)
            * Quaternion.Euler(0f, modelForwardYawOffset, 0f);

        transform.rotation = Quaternion.Slerp(
            transform.rotation,
            targetRotation,
            Time.deltaTime * rotationSmoothness
        );
    }
}
