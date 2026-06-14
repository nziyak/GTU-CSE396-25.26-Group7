/// <summary>
/// File:    DashboardInitializer.cs
/// Brief:   Runtime auto-binding bootstrap for the MOD-05 operator dashboard scene.
/// </summary>

using UnityEngine;

public class DashboardInitializer : MonoBehaviour
{
    [Header("Critical Components")]
    [SerializeField] private RobotManager robotManager;
    [SerializeField] private UIManager uiManager;
    [SerializeField] private MapManager mapManager;
    [SerializeField] private MapManager_AcousticBeam acousticBeam;
    [SerializeField] private OperatorCameraController cameraController;

    private void Awake()
    {
        AutoBind();
    }

    [ContextMenu("Auto Bind Dashboard")]
    public void AutoBind()
    {
        robotManager = robotManager != null ? robotManager : FindObjectOfType<RobotManager>();
        uiManager = uiManager != null ? uiManager : FindObjectOfType<UIManager>();
        mapManager = mapManager != null ? mapManager : FindObjectOfType<MapManager>();
        acousticBeam = acousticBeam != null ? acousticBeam : FindObjectOfType<MapManager_AcousticBeam>();
        cameraController = cameraController != null ? cameraController : FindObjectOfType<OperatorCameraController>();

        if (robotManager != null)
        {
            robotManager.SendMessage("AutoBindSceneReferences", SendMessageOptions.DontRequireReceiver);
        }
        else
        {
            Debug.LogError("DashboardInitializer: RobotManager is missing from the scene.");
        }

        if (uiManager != null)
        {
            uiManager.SendMessage("AutoBindDashboardElements", SendMessageOptions.DontRequireReceiver);
        }
        else
        {
            Debug.LogError("DashboardInitializer: UIManager is missing from the scene.");
        }

        if (mapManager != null)
        {
            mapManager.SendMessage("AutoBindMapElements", SendMessageOptions.DontRequireReceiver);
        }
        else
        {
            Debug.LogError("DashboardInitializer: MapManager is missing from the scene.");
        }

        if (acousticBeam != null)
        {
            acousticBeam.SendMessage("AutoBindBeamComponents", SendMessageOptions.DontRequireReceiver);
        }

        if (cameraController != null)
        {
            GameObject robotMarker = GameObject.Find("RobotMarker");
            if (robotMarker != null)
            {
                cameraController.SetTarget(robotMarker.transform);
            }
            else
            {
                Debug.LogError("DashboardInitializer: RobotMarker is missing; camera target was not assigned.");
            }
        }
        else
        {
            Debug.LogError("DashboardInitializer: OperatorCameraController is missing from the scene camera.");
        }
    }
}
