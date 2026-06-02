/// <summary>
/// File:    IndustrialOperatorControls.cs
/// Brief:   Styles the bottom operator controls as a compact industrial command console.
/// </summary>

using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

[ExecuteAlways]
public class IndustrialOperatorControls : MonoBehaviour
{
    [SerializeField] private float panelWidth = 720f;
    [SerializeField] private float panelHeight = 150f;
    [SerializeField] private Color panelColor = new Color(0.012f, 0.018f, 0.024f, 0.86f);
    [SerializeField] private Color sectionColor = new Color(0.04f, 0.055f, 0.066f, 0.92f);
    [SerializeField] private Color commandButtonColor = new Color(0.13f, 0.18f, 0.22f, 0.96f);
    [SerializeField] private Color directionButtonColor = new Color(0.88f, 0.92f, 0.94f, 0.98f);
    [SerializeField] private Color commandButtonHoverColor = new Color(0.18f, 0.27f, 0.32f, 1f);
    [SerializeField] private Color stopButtonColor = new Color(0.78f, 0.08f, 0.12f, 0.98f);
    [SerializeField] private Color pttButtonColor = new Color(0.03f, 0.42f, 0.58f, 0.98f);
    [SerializeField] private Color titleColor = new Color(0.24f, 0.95f, 1f, 1f);
    [SerializeField] private Color mutedColor = new Color(0.58f, 0.70f, 0.76f, 1f);

    private void Reset()
    {
        ApplyStyle();
    }

    private void Awake()
    {
        ApplyStyle();
    }

    private void OnEnable()
    {
        ApplyStyle();
    }

    [ContextMenu("Apply Industrial Operator Controls Style")]
    public void ApplyStyle()
    {
        StyleRoot();
        DisableLayoutGroups(transform);

        panelWidth = 760f;
        panelHeight = 160f;

        Image driveSection = EnsureSection("DriveSection", 12f, -12f, 312f, 132f);
        EnsureHeader(driveSection.transform, "DRIVE CONTROL", "MANUAL OVERRIDE");

        StyleCommandButton(driveSection.transform, "ForwardButton", "▲", "FORWARD", 112f, -42f, 88f, 36f, directionButtonColor, new Color(0.08f, 0.11f, 0.13f, 1f));
        StyleCommandButton(driveSection.transform, "LeftButton", "◀", "LEFT", 22f, -82f, 88f, 36f, directionButtonColor, new Color(0.08f, 0.11f, 0.13f, 1f));
        StyleCommandButton(driveSection.transform, "BackwardButton", "▼", "BACK", 112f, -82f, 88f, 36f, directionButtonColor, new Color(0.08f, 0.11f, 0.13f, 1f));
        StyleCommandButton(driveSection.transform, "RightButton", "▶", "RIGHT", 202f, -82f, 88f, 36f, directionButtonColor, new Color(0.08f, 0.11f, 0.13f, 1f));

        Image safetySection = EnsureSection("SafetySection", 340f, -12f, 172f, 132f);
        EnsureHeader(safetySection.transform, "SAFETY", "IMMEDIATE HALT");
        StyleCommandButton(safetySection.transform, "StopButton", "■", "STOP", 28f, -58f, 116f, 48f, stopButtonColor, Color.white);

        Image voiceSection = EnsureSection("VoiceSection", 524f, -12f, 224f, 132f);
        EnsureHeader(voiceSection.transform, "VOICE LINK", "PUSH TO TALK");
        StyleCommandButton(voiceSection.transform, "PushToTalkButton", "●", "HOLD TO TALK", 24f, -58f, 176f, 48f, pttButtonColor, Color.white);
        EnsurePttStateText(voiceSection.transform);

        LayoutRebuilder.ForceRebuildLayoutImmediate(GetComponent<RectTransform>());
    }

    private void StyleRoot()
    {
        RectTransform rect = GetComponent<RectTransform>();
        if (rect != null)
        {
            rect.anchorMin = new Vector2(0.5f, 0f);
            rect.anchorMax = new Vector2(0.5f, 0f);
            rect.pivot = new Vector2(0.5f, 0f);
            rect.anchoredPosition = new Vector2(0f, 18f);
            rect.sizeDelta = new Vector2(panelWidth, panelHeight);
            rect.localScale = Vector3.one;
        }

        Image image = GetComponent<Image>();
        if (image == null)
        {
            image = gameObject.AddComponent<Image>();
        }

        image.color = panelColor;
        image.raycastTarget = false;
    }

    private Image EnsureSection(string name, float x, float y, float width, float height)
    {
        Transform child = transform.Find(name);
        if (child == null)
        {
            GameObject sectionObject = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
            sectionObject.transform.SetParent(transform, false);
            child = sectionObject.transform;
        }

        SetTopLeftRect(child.GetComponent<RectTransform>(), x, y, width, height);
        Image image = child.GetComponent<Image>();
        image.color = sectionColor;
        image.raycastTarget = false;
        return image;
    }

    private void EnsureHeader(Transform section, string title, string subtitle)
    {
        Text titleText = EnsureChildText(section, "Title");
        SetTopLeftRect(titleText.GetComponent<RectTransform>(), 10f, -6f, 140f, 18f);
        titleText.text = title;
        titleText.font = GetBuiltinUiFont();
        titleText.fontSize = 12;
        titleText.fontStyle = FontStyle.Bold;
        titleText.alignment = TextAnchor.MiddleLeft;
        titleText.color = titleColor;

        Text subtitleText = EnsureChildText(section, "Subtitle");
        SetTopLeftRect(subtitleText.GetComponent<RectTransform>(), 10f, -24f, 140f, 15f);
        subtitleText.text = subtitle;
        subtitleText.font = GetBuiltinUiFont();
        subtitleText.fontSize = 9;
        subtitleText.alignment = TextAnchor.MiddleLeft;
        subtitleText.color = mutedColor;
    }

    private void StyleCommandButton(Transform parent, string objectName, string icon, string label, float x, float y, float width, float height, Color color, Color textColor)
    {
        Button button = FindOrCreateButton(objectName);
        button.transform.SetParent(parent, false);
        SetTopLeftRect(button.GetComponent<RectTransform>(), x, y, width, height);

        Image image = button.GetComponent<Image>();
        image.color = color;
        image.raycastTarget = true;

        ColorBlock colors = button.colors;
        colors.normalColor = color;
        colors.highlightedColor = Color.Lerp(color, commandButtonHoverColor, 0.35f);
        colors.pressedColor = Color.Lerp(color, Color.black, 0.25f);
        colors.selectedColor = colors.highlightedColor;
        colors.disabledColor = new Color(0.1f, 0.12f, 0.14f, 0.55f);
        colors.colorMultiplier = 1f;
        button.colors = colors;

        Text labelText = EnsureChildText(button.transform, "Label");
        SetTopLeftRect(labelText.GetComponent<RectTransform>(), 0f, 0f, width, height);
        labelText.text = $"{icon}  {label}";
        labelText.font = GetBuiltinUiFont();
        labelText.fontSize = objectName == "PushToTalkButton" ? 12 : 13;
        labelText.fontStyle = FontStyle.Bold;
        labelText.alignment = TextAnchor.MiddleCenter;
        labelText.color = textColor;
        labelText.gameObject.SetActive(true);

        HideUnexpectedButtonTexts(button.transform);
        DisableLayoutGroups(button.transform);
    }

    private void EnsurePttStateText(Transform parent)
    {
        Text stateText = FindOrCreateLooseText("PTTStateText");
        stateText.transform.SetParent(parent, false);
        SetTopLeftRect(stateText.GetComponent<RectTransform>(), 24f, -110f, 176f, 12f);
        if (string.IsNullOrWhiteSpace(stateText.text) || stateText.text == "New Text")
        {
            stateText.text = "VOICE READY";
        }

        stateText.font = GetBuiltinUiFont();
        stateText.fontSize = 10;
        stateText.fontStyle = FontStyle.Bold;
        stateText.alignment = TextAnchor.MiddleCenter;
        stateText.color = mutedColor;
        stateText.gameObject.SetActive(true);
    }

    private Button FindOrCreateButton(string name)
    {
        GameObject target = GameObject.Find(name);
        if (target == null)
        {
            target = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image), typeof(Button));
        }

        target.transform.SetParent(transform, false);
        target.SetActive(true);

        Button button = target.GetComponent<Button>();
        return button != null ? button : target.AddComponent<Button>();
    }

    private Text FindOrCreateLooseText(string name)
    {
        GameObject target = GameObject.Find(name);
        if (target == null)
        {
            target = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
        }

        target.transform.SetParent(transform, false);
        target.SetActive(true);

        Text text = target.GetComponent<Text>();
        return text != null ? text : target.AddComponent<Text>();
    }

    private static Text EnsureChildText(Transform parent, string name)
    {
        Transform child = parent.Find(name);
        if (child == null)
        {
            GameObject childObject = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
            childObject.transform.SetParent(parent, false);
            child = childObject.transform;
        }

        Text text = child.GetComponent<Text>();
        return text != null ? text : child.gameObject.AddComponent<Text>();
    }

    private static void HideUnexpectedButtonTexts(Transform button)
    {
        Graphic[] graphics = button.GetComponentsInChildren<Graphic>(true);
        for (int i = 0; i < graphics.Length; i++)
        {
            if (graphics[i].transform == button)
            {
                continue;
            }

            bool isExpectedLabel = graphics[i].name == "Label" && graphics[i].GetComponent<Text>() != null;
            if (!isExpectedLabel)
            {
                graphics[i].gameObject.SetActive(false);
            }
        }

        UIBehaviour[] uiBehaviours = button.GetComponentsInChildren<UIBehaviour>(true);
        for (int i = 0; i < uiBehaviours.Length; i++)
        {
            if (uiBehaviours[i].transform == button)
            {
                continue;
            }

            bool isExpectedLabel = uiBehaviours[i].name == "Label" && uiBehaviours[i].GetComponent<Text>() != null;
            if (!isExpectedLabel)
            {
                uiBehaviours[i].gameObject.SetActive(false);
            }
        }
    }

    private static void DisableLayoutGroups(Transform target)
    {
        LayoutGroup[] layouts = target.GetComponents<LayoutGroup>();
        for (int i = 0; i < layouts.Length; i++)
        {
            layouts[i].enabled = false;
        }
    }

    private static void SetTopLeftRect(RectTransform rect, float x, float y, float width, float height)
    {
        if (rect == null)
        {
            return;
        }

        rect.anchorMin = new Vector2(0f, 1f);
        rect.anchorMax = new Vector2(0f, 1f);
        rect.pivot = new Vector2(0f, 1f);
        rect.anchoredPosition = new Vector2(x, y);
        rect.sizeDelta = new Vector2(width, height);
        rect.localScale = Vector3.one;
        rect.localRotation = Quaternion.identity;
    }

    private static Font GetBuiltinUiFont()
    {
        return Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
    }
}
