/// <summary>
/// File:    DashboardInitializer.cs
/// Brief:   Runtime auto-binding bootstrap for the MOD-05 operator dashboard scene.
/// </summary>

using UnityEngine;
using UnityEngine.UI;

public class DashboardInitializer : MonoBehaviour
{
    [Header("Critical Components")]
    [SerializeField] private RobotManager robotManager;
    [SerializeField] private UIManager uiManager;
    [SerializeField] private MapManager mapManager;
    [SerializeField] private MapManager_AcousticBeam acousticBeam;
    [SerializeField] private OperatorCameraController cameraController;
    [SerializeField] private bool applyProfessionalLayout = true;

    private void Awake()
    {
        if (applyProfessionalLayout)
        {
            ApplyProfessionalDashboardLayout();
        }

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

    [ContextMenu("Apply Professional Dashboard Layout")]
    public void ApplyProfessionalDashboardLayout()
    {
        Canvas canvas = FindObjectOfType<Canvas>();
        if (canvas == null)
        {
            return;
        }

        CanvasScaler scaler = canvas.GetComponent<CanvasScaler>();
        if (scaler != null)
        {
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(1920f, 1080f);
            scaler.matchWidthOrHeight = 0.5f;
        }

        RectTransform canvasRect = canvas.GetComponent<RectTransform>();
        RectTransform leftPanel = ConfigurePanel("LeftPanel_Health", canvasRect, new Vector2(0f, 0.18f), new Vector2(0f, 0.96f), new Vector2(0f, 1f), new Vector2(18f, -18f), new Vector2(280f, 0f));
        RectTransform rightPanel = ConfigurePanel("RightPanel_Perception", canvasRect, new Vector2(1f, 0.42f), new Vector2(1f, 0.96f), new Vector2(1f, 1f), new Vector2(-18f, -18f), new Vector2(300f, 0f));
        RectTransform centerPanel = ConfigurePanel("CenterPanel_DigitalTwin", canvasRect, new Vector2(0f, 0f), new Vector2(1f, 1f), new Vector2(0.5f, 0.5f), Vector2.zero, Vector2.zero);
        RectTransform controlsPanel = ConfigurePanel("OperatorControlsPanel", canvasRect, new Vector2(0.5f, 0f), new Vector2(0.5f, 0f), new Vector2(0.5f, 0f), new Vector2(0f, 18f), new Vector2(760f, 160f));

        if (centerPanel != null)
        {
            centerPanel.SetAsFirstSibling();
            centerPanel.offsetMin = new Vector2(24f, 124f);
            centerPanel.offsetMax = new Vector2(-24f, -24f);
            SetPanelImage(centerPanel, new Color(0.02f, 0.025f, 0.032f, 0.02f));
            Image centerImage = centerPanel.GetComponent<Image>();
            if (centerImage != null)
            {
                centerImage.raycastTarget = false;
            }
        }

        ConfigureTelemetryPanel(leftPanel);
        ConfigurePerceptionPanel(rightPanel);
        ConfigureControlsPanel(controlsPanel);

        if (leftPanel != null)
        {
            IndustrialTelemetryPanel telemetryPanel = leftPanel.GetComponent<IndustrialTelemetryPanel>();
            if (telemetryPanel == null)
            {
                telemetryPanel = leftPanel.gameObject.AddComponent<IndustrialTelemetryPanel>();
            }

            telemetryPanel.ApplyStyle();
        }

        if (rightPanel != null)
        {
            IndustrialPerceptionPanel perceptionPanel = rightPanel.GetComponent<IndustrialPerceptionPanel>();
            if (perceptionPanel == null)
            {
                perceptionPanel = rightPanel.gameObject.AddComponent<IndustrialPerceptionPanel>();
            }

            perceptionPanel.ApplyStyle();
        }

        if (controlsPanel != null)
        {
            IndustrialOperatorControls operatorControls = controlsPanel.GetComponent<IndustrialOperatorControls>();
            if (operatorControls == null)
            {
                operatorControls = controlsPanel.gameObject.AddComponent<IndustrialOperatorControls>();
            }

            operatorControls.ApplyStyle();
        }
    }

    private static RectTransform ConfigurePanel(string name, Transform parent, Vector2 anchorMin, Vector2 anchorMax, Vector2 pivot, Vector2 anchoredPosition, Vector2 sizeDelta)
    {
        GameObject target = GameObject.Find(name);
        if (target == null)
        {
            target = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
            target.transform.SetParent(parent, false);
        }

        RectTransform rect = target.GetComponent<RectTransform>();
        rect.anchorMin = anchorMin;
        rect.anchorMax = anchorMax;
        rect.pivot = pivot;
        rect.anchoredPosition = anchoredPosition;
        rect.sizeDelta = sizeDelta;
        rect.localScale = Vector3.one;
        SetPanelImage(rect, new Color(0.035f, 0.045f, 0.055f, 0.68f));
        return rect;
    }

    private static void ConfigureTelemetryPanel(RectTransform panel)
    {
        if (panel == null)
        {
            return;
        }

        VerticalLayoutGroup layout = EnsureComponent<VerticalLayoutGroup>(panel.gameObject);
        layout.padding = new RectOffset(12, 12, 12, 12);
        layout.spacing = 4f;
        layout.childAlignment = TextAnchor.UpperLeft;
        layout.childControlWidth = true;
        layout.childControlHeight = false;
        layout.childForceExpandWidth = true;
        layout.childForceExpandHeight = false;

        EnsureHudText(panel, "ConnectionStatusText", "DISCONNECTED");
        EnsureHudText(panel, "LatencyText", "LATENCY -- ms");
        EnsureHudText(panel, "TemperatureText", "TEMP -- C");
        EnsureHudText(panel, "SmokeStatusText", "SMOKE --");
        EnsureHudText(panel, "VictimStatusText", "VICTIM NONE");
        EnsureHudText(panel, "PriorityLevelText", "PRIORITY --");
        EnsureHudText(panel, "StuckStatusText", "MOBILITY --");
        EnsureHudText(panel, "PositionText", "POSITION X: -- Y: --");
        EnsureHudText(panel, "AcousticStatusText", "ACOUSTIC --");
        EnsureHudText(panel, "UartConnectionText", "UART --");
        EnsureHudText(panel, "UartDistanceText", "DISTANCE FRONT: -- BACK: --");
        EnsureHudText(panel, "UartOrientationText", "IMU YAW: -- PITCH: -- ROLL: --");
        EnsureHudText(panel, "UartEnvironmentText", "UART ENV TEMP: -- HUM: --");
        EnsureHudText(panel, "UartMicrophoneText", "MIC --");
        EnsureHudText(panel, "SpeechCommandText", "VOICE --");
    }

    private static void ConfigurePerceptionPanel(RectTransform panel)
    {
        if (panel == null)
        {
            return;
        }

        VerticalLayoutGroup layout = EnsureComponent<VerticalLayoutGroup>(panel.gameObject);
        layout.padding = new RectOffset(12, 12, 12, 12);
        layout.spacing = 7f;
        layout.childAlignment = TextAnchor.UpperCenter;
        layout.childControlWidth = true;
        layout.childControlHeight = false;
        layout.childForceExpandWidth = true;
        layout.childForceExpandHeight = false;

        RawImage video = FindOrCreateRawImage(panel, "VideoFrameRawImage");
        LayoutElement videoLayout = EnsureComponent<LayoutElement>(video.gameObject);
        videoLayout.preferredHeight = 176f;
        videoLayout.flexibleHeight = 0f;
        video.color = Color.white;

        EnsureHudText(panel, "PerceptionVictimStatusText", "VICTIM STATUS: NONE");
        EnsureHudText(panel, "PerceptionPriorityText", "PRIORITY: NONE");
        EnsureHudText(panel, "PerceptionAcousticText", "ACOUSTIC: CLEAR");
        EnsureHudText(panel, "PerceptionSpeechText", "VOICE: --");
        EnsureHudText(panel, "PerceptionHintText", "CAMERA FEED: WAITING FOR VIDEO_FRAME");
    }

    private static void ConfigureControlsPanel(RectTransform panel)
    {
        if (panel == null)
        {
            return;
        }

        VerticalLayoutGroup vertical = panel.GetComponent<VerticalLayoutGroup>();
        if (vertical != null)
        {
            vertical.enabled = false;
        }

        GridLayoutGroup grid = EnsureComponent<GridLayoutGroup>(panel.gameObject);
        grid.enabled = false;
        grid.padding = new RectOffset(14, 14, 12, 12);
        grid.spacing = new Vector2(8f, 8f);
        grid.cellSize = new Vector2(96f, 38f);
        grid.constraint = GridLayoutGroup.Constraint.FixedColumnCount;
        grid.constraintCount = 3;
        grid.childAlignment = TextAnchor.MiddleCenter;

        string[] buttons = { "ForwardButton", "LeftButton", "RightButton", "BackwardButton", "StopButton", "PushToTalkButton" };
        for (int i = 0; i < buttons.Length; i++)
        {
            Button button = FindOrCreateButton(panel, buttons[i]);
            StyleButton(button, buttons[i]);
        }
    }

    private static Text EnsureHudText(Transform parent, string name, string initialText)
    {
        GameObject target = GameObject.Find(name);
        if (target == null)
        {
            target = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
            target.transform.SetParent(parent, false);
        }
        else if (target.transform.parent != parent)
        {
            target.transform.SetParent(parent, false);
        }

        Text text = target.GetComponent<Text>();
        if (text == null)
        {
            text = target.AddComponent<Text>();
        }

        if (string.IsNullOrWhiteSpace(text.text) || text.text == "New Text")
        {
            text.text = initialText;
        }

        text.font = GetBuiltinUiFont();
        text.fontSize = 14;
        text.alignment = TextAnchor.MiddleLeft;
        text.color = new Color(0.88f, 0.94f, 0.96f, 1f);
        text.horizontalOverflow = HorizontalWrapMode.Overflow;
        text.verticalOverflow = VerticalWrapMode.Truncate;

        LayoutElement element = EnsureComponent<LayoutElement>(target);
        element.preferredHeight = 24f;
        element.flexibleHeight = 0f;
        return text;
    }

    private static RawImage FindOrCreateRawImage(Transform parent, string name)
    {
        GameObject target = GameObject.Find(name);
        if (target == null)
        {
            target = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(RawImage));
            target.transform.SetParent(parent, false);
        }
        else if (target.transform.parent != parent)
        {
            target.transform.SetParent(parent, false);
        }

        RawImage image = target.GetComponent<RawImage>();
        return image != null ? image : target.AddComponent<RawImage>();
    }

    private static Button FindOrCreateButton(Transform parent, string name)
    {
        GameObject target = GameObject.Find(name);
        if (target == null)
        {
            target = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image), typeof(Button));
            target.transform.SetParent(parent, false);
            EnsureButtonLabel(target.transform, ResolveButtonLabel(name));
        }
        else if (target.transform.parent != parent)
        {
            target.transform.SetParent(parent, false);
        }

        EnsureButtonLabel(target.transform, ResolveButtonLabel(name));
        Button button = target.GetComponent<Button>();
        return button != null ? button : target.AddComponent<Button>();
    }

    private static Text EnsureButtonLabel(Transform parent, string labelText)
    {
        Transform existing = parent.Find("Label");
        GameObject target;
        if (existing == null)
        {
            target = new GameObject("Label", typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
            target.transform.SetParent(parent, false);
        }
        else
        {
            target = existing.gameObject;
        }

        RectTransform rect = target.GetComponent<RectTransform>();
        rect.anchorMin = Vector2.zero;
        rect.anchorMax = Vector2.one;
        rect.offsetMin = Vector2.zero;
        rect.offsetMax = Vector2.zero;

        Text text = target.GetComponent<Text>();
        text.text = labelText;
        return text;
    }

    private static void StyleButton(Button button, string name)
    {
        Image image = button.GetComponent<Image>();
        if (image != null)
        {
            if (name == "StopButton")
            {
                image.color = new Color(0.72f, 0.08f, 0.12f, 0.96f);
            }
            else if (name == "PushToTalkButton")
            {
                image.color = new Color(0.04f, 0.42f, 0.58f, 0.96f);
            }
            else
            {
                image.color = new Color(0.18f, 0.22f, 0.26f, 0.96f);
            }
        }

        Text label = button.GetComponentInChildren<Text>();
        if (label != null)
        {
            label.text = ResolveButtonLabel(name);
            label.font = GetBuiltinUiFont();
            label.fontSize = 12;
            label.fontStyle = FontStyle.Bold;
            label.alignment = TextAnchor.MiddleCenter;
            label.color = Color.white;
        }
    }

    private static string ResolveButtonLabel(string name)
    {
        switch (name)
        {
            case "ForwardButton":
                return "FORWARD";
            case "BackwardButton":
                return "BACKWARD";
            case "LeftButton":
                return "LEFT";
            case "RightButton":
                return "RIGHT";
            case "StopButton":
                return "STOP";
            case "PushToTalkButton":
                return "HOLD TO TALK";
            default:
                return name.ToUpperInvariant();
        }
    }

    private static void SetPanelImage(RectTransform rect, Color color)
    {
        Image image = rect.GetComponent<Image>();
        if (image == null)
        {
            image = rect.gameObject.AddComponent<Image>();
        }

        image.color = color;
    }

    private static T EnsureComponent<T>(GameObject target) where T : Component
    {
        T component = target.GetComponent<T>();
        return component != null ? component : target.AddComponent<T>();
    }

    private static Font GetBuiltinUiFont()
    {
        Font font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
        return font != null ? font : Resources.GetBuiltinResource<Font>("Arial.ttf");
    }
}
