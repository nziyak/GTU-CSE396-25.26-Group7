/// <summary>
/// File:    IndustrialPerceptionPanel.cs
/// Brief:   Styles the right-side camera and perception panel as an industrial HUD card stack.
/// </summary>

using System;
using UnityEngine;
using UnityEngine.UI;

[ExecuteAlways]
public class IndustrialPerceptionPanel : MonoBehaviour
{
    [Serializable]
    public class HudTheme
    {
        public Color panelBackground = new Color(0f, 0f, 0f, 0f);
        public Color cardBackground = new Color(0.04f, 0.055f, 0.066f, 0.92f);
        public Color videoPlaceholder = new Color(0.012f, 0.018f, 0.024f, 1f);
        public Color titleColor = new Color(0.24f, 0.95f, 1f, 1f);
        public Color labelColor = new Color(0.58f, 0.70f, 0.76f, 1f);
        public Color valueColor = new Color(0.90f, 0.98f, 1f, 1f);
        public Color okColor = new Color(0.2f, 1f, 0.42f, 1f);
        public Color warningColor = new Color(1f, 0.72f, 0.12f, 1f);
        public Color dangerColor = new Color(1f, 0.16f, 0.20f, 1f);
        public Color mutedColor = new Color(0.38f, 0.48f, 0.54f, 1f);
    }

    [SerializeField] private HudTheme theme = new HudTheme();
    [SerializeField] private float panelWidth = 340f;
    [SerializeField] private float panelPadding = 12f;
    [SerializeField] private float cardSpacing = 10f;
    [SerializeField] private float cameraCardHeight = 238f;
    [SerializeField] private float analysisCardHeight = 172f;
    [SerializeField] private float cardPadding = 10f;
    [SerializeField] private int titleFontSize = 14;
    [SerializeField] private int labelFontSize = 10;
    [SerializeField] private int valueFontSize = 13;
    [SerializeField] private float refreshIntervalSeconds = 0.12f;

    private float nextRefreshTime;

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

    private void Update()
    {
        if (Time.unscaledTime < nextRefreshTime)
        {
            return;
        }

        nextRefreshTime = Time.unscaledTime + refreshIntervalSeconds;
        RefreshDynamicColors();
    }

    [ContextMenu("Apply Industrial Perception Style")]
    public void ApplyStyle()
    {
        StyleRootPanel();
        HideLoosePanelTexts();

        Transform cameraCard = EnsureCard("PerceptionCard_0_LIVE_CAMERA", 0, cameraCardHeight);
        EnsureTitle(cameraCard, "LIVE CAMERA", "RASPBERRY PI VIDEO STREAM");
        EnsureCameraFrame(cameraCard);
        EnsureStatusPill(cameraCard);

        Transform analysisCard = EnsureCard("PerceptionCard_1_AI_PERCEPTION", 1, analysisCardHeight);
        EnsureTitle(analysisCard, "AI PERCEPTION", "VICTIM AND VOICE ANALYSIS");
        EnsureMetricRow(analysisCard, 0, "VIC", "Victim", "PerceptionVictimStatusText", "VICTIM STATUS: NONE");
        EnsureMetricRow(analysisCard, 1, "PRI", "Priority", "PerceptionPriorityText", "PRIORITY: NONE");
        EnsureMetricRow(analysisCard, 2, "AUD", "Acoustic", "PerceptionAcousticText", "ACOUSTIC: CLEAR");
        EnsureMetricRow(analysisCard, 3, "PTT", "Voice", "PerceptionSpeechText", "VOICE: --");

        HideLoosePanelGraphics();
        RefreshDynamicColors();
        LayoutRebuilder.ForceRebuildLayoutImmediate(GetComponent<RectTransform>());
    }

    private void StyleRootPanel()
    {
        Image image = GetComponent<Image>();
        if (image == null)
        {
            image = gameObject.AddComponent<Image>();
        }

        image.color = new Color(0f, 0f, 0f, 0f);
        image.raycastTarget = false;

        LayoutGroup[] layouts = GetComponents<LayoutGroup>();
        for (int i = 0; i < layouts.Length; i++)
        {
            layouts[i].enabled = false;
        }

        RectTransform rect = GetComponent<RectTransform>();
        if (rect != null)
        {
            rect.sizeDelta = new Vector2(panelWidth, rect.sizeDelta.y);
        }
    }

    private Transform EnsureCard(string objectName, int index, float height)
    {
        Transform card = transform.Find(objectName);
        if (card == null)
        {
            GameObject cardObject = new GameObject(objectName, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
            cardObject.transform.SetParent(transform, false);
            card = cardObject.transform;
        }

        float y = -panelPadding - (index * (height + cardSpacing));
        if (index == 1)
        {
            y = -panelPadding - cameraCardHeight - cardSpacing;
        }

        SetTopLeftRect(card.GetComponent<RectTransform>(), panelPadding, y, GetCardWidth(), height);

        Image cardImage = card.GetComponent<Image>();
        cardImage.color = theme.cardBackground;

        LayoutGroup[] layouts = card.GetComponents<LayoutGroup>();
        for (int i = 0; i < layouts.Length; i++)
        {
            layouts[i].enabled = false;
        }

        LayoutElement element = card.GetComponent<LayoutElement>();
        if (element == null)
        {
            element = card.gameObject.AddComponent<LayoutElement>();
        }

        element.ignoreLayout = true;
        element.preferredHeight = height;
        element.flexibleHeight = 0f;
        return card;
    }

    private void EnsureTitle(Transform card, string title, string subtitle)
    {
        Text titleText = EnsureChildText(card, "Title");
        SetTopLeftRect(titleText.GetComponent<RectTransform>(), cardPadding, -6f, GetInnerWidth(), 22f);
        titleText.text = title;
        titleText.font = GetBuiltinUiFont();
        titleText.fontSize = titleFontSize;
        titleText.fontStyle = FontStyle.Bold;
        titleText.alignment = TextAnchor.MiddleLeft;
        titleText.color = theme.titleColor;

        Text subtitleText = EnsureChildText(card, "Subtitle");
        SetTopLeftRect(subtitleText.GetComponent<RectTransform>(), cardPadding, -28f, GetInnerWidth(), 18f);
        subtitleText.text = subtitle;
        subtitleText.font = GetBuiltinUiFont();
        subtitleText.fontSize = labelFontSize;
        subtitleText.fontStyle = FontStyle.Normal;
        subtitleText.alignment = TextAnchor.MiddleLeft;
        subtitleText.color = theme.labelColor;
    }

    private void EnsureCameraFrame(Transform card)
    {
        Image videoBackground = EnsureChildImage(card, "CameraFeedBackground");
        SetTopLeftRect(videoBackground.GetComponent<RectTransform>(), cardPadding, -52f, GetInnerWidth(), 154f);
        videoBackground.color = theme.videoPlaceholder;
        videoBackground.raycastTarget = false;
        videoBackground.transform.SetSiblingIndex(2);

        RawImage video = FindOrCreateRawImage(card, "VideoFrameRawImage");
        SetTopLeftRect(video.GetComponent<RectTransform>(), cardPadding, -52f, GetInnerWidth(), 154f);
        video.color = video.texture == null ? new Color(1f, 1f, 1f, 0f) : Color.white;
        video.raycastTarget = false;
        video.transform.SetAsLastSibling();

        Image border = EnsureChildImage(card, "CameraFrameBorder");
        SetTopLeftRect(border.GetComponent<RectTransform>(), cardPadding - 2f, -50f, GetInnerWidth() + 4f, 158f);
        border.color = new Color(0.05f, 0.38f, 0.50f, 0.18f);
        border.raycastTarget = false;
        border.transform.SetSiblingIndex(1);

        Text placeholder = EnsureChildText(card, "CameraPlaceholderText");
        SetTopLeftRect(placeholder.GetComponent<RectTransform>(), cardPadding, -115f, GetInnerWidth(), 24f);
        placeholder.text = "WAITING FOR CAMERA";
        placeholder.font = GetBuiltinUiFont();
        placeholder.fontSize = labelFontSize;
        placeholder.fontStyle = FontStyle.Bold;
        placeholder.alignment = TextAnchor.MiddleCenter;
        placeholder.color = theme.mutedColor;
        placeholder.raycastTarget = false;
        placeholder.gameObject.SetActive(video.texture == null);
    }

    private void EnsureStatusPill(Transform card)
    {
        Text hint = EnsureChildText(card, "PerceptionHintText");
        SetTopLeftRect(hint.GetComponent<RectTransform>(), cardPadding, -212f, GetInnerWidth(), 18f);
        if (string.IsNullOrWhiteSpace(hint.text) || hint.text == "New Text")
        {
            hint.text = "CAMERA FEED: WAITING FOR VIDEO_FRAME";
        }

        hint.font = GetBuiltinUiFont();
        hint.fontSize = labelFontSize;
        hint.fontStyle = FontStyle.Bold;
        hint.alignment = TextAnchor.MiddleLeft;
        hint.color = theme.mutedColor;
    }

    private void EnsureMetricRow(Transform card, int index, string icon, string label, string valueName, string initialValue)
    {
        string rowName = $"Row_{index}_{valueName}";
        Transform row = card.Find(rowName);
        if (row == null)
        {
            GameObject rowObject = new GameObject(rowName, typeof(RectTransform));
            rowObject.transform.SetParent(card, false);
            row = rowObject.transform;
        }

        float y = -56f - (index * 28f);
        SetTopLeftRect(row.GetComponent<RectTransform>(), cardPadding, y, GetInnerWidth(), 27f);

        Text iconText = EnsureChildText(row, "Icon");
        SetTopLeftRect(iconText.GetComponent<RectTransform>(), 0f, 0f, 34f, 27f);
        iconText.text = icon;
        iconText.font = GetBuiltinUiFont();
        iconText.fontSize = labelFontSize;
        iconText.fontStyle = FontStyle.Bold;
        iconText.alignment = TextAnchor.MiddleCenter;
        iconText.color = theme.titleColor;

        Text labelText = EnsureChildText(row, "Label");
        SetTopLeftRect(labelText.GetComponent<RectTransform>(), 40f, 0f, 76f, 27f);
        labelText.text = label.ToUpperInvariant();
        labelText.font = GetBuiltinUiFont();
        labelText.fontSize = labelFontSize;
        labelText.alignment = TextAnchor.MiddleLeft;
        labelText.color = theme.labelColor;

        Text valueText = FindOrCreatePanelText(valueName, row, initialValue);
        SetTopLeftRect(valueText.GetComponent<RectTransform>(), 122f, 0f, GetInnerWidth() - 122f, 27f);
        valueText.font = GetBuiltinUiFont();
        valueText.fontSize = valueFontSize;
        valueText.fontStyle = index == 0 ? FontStyle.Bold : FontStyle.Normal;
        valueText.alignment = TextAnchor.MiddleLeft;
        valueText.horizontalOverflow = HorizontalWrapMode.Wrap;
        valueText.verticalOverflow = VerticalWrapMode.Truncate;
        valueText.color = theme.valueColor;
    }

    private void RefreshDynamicColors()
    {
        ColorByContent("PerceptionVictimStatusText", theme.valueColor, theme.warningColor, theme.dangerColor);
        ColorByContent("PerceptionPriorityText", theme.valueColor, theme.warningColor, theme.dangerColor);
        ColorByContent("PerceptionAcousticText", theme.okColor, theme.warningColor, theme.titleColor);
        ColorByContent("PerceptionSpeechText", theme.valueColor, theme.titleColor, theme.warningColor);
    }

    private void ColorByContent(string textObjectName, Color normal, Color warning, Color danger)
    {
        Text text = FindPanelText(textObjectName);
        if (text == null)
        {
            return;
        }

        string value = text.text.ToUpperInvariant();
        if (value.Contains("TRAPPED") || value.Contains("PRIORITY: 1") || value.Contains("ERROR"))
        {
            text.color = danger;
            return;
        }

        if (value.Contains("LYING") || value.Contains("PRIORITY: 2") || value.Contains("WAITING"))
        {
            text.color = warning;
            return;
        }

        if (value.Contains("--") || value.Contains("NONE"))
        {
            text.color = theme.mutedColor;
            return;
        }

        text.color = normal;
    }

    private void HideLoosePanelTexts()
    {
        Text[] texts = GetComponentsInChildren<Text>(true);
        for (int i = 0; i < texts.Length; i++)
        {
            bool cardChild = FindPerceptionCardParent(texts[i].transform) != null;
            if (!cardChild)
            {
                texts[i].gameObject.SetActive(false);
            }
        }
    }

    private void HideLoosePanelGraphics()
    {
        Graphic[] graphics = GetComponentsInChildren<Graphic>(true);
        for (int i = 0; i < graphics.Length; i++)
        {
            Transform graphicTransform = graphics[i].transform;
            if (graphicTransform == transform)
            {
                continue;
            }

            bool cardChild = FindPerceptionCardParent(graphicTransform) != null;
            if (!cardChild)
            {
                graphics[i].gameObject.SetActive(false);
            }
        }
    }

    private Text FindOrCreatePanelText(string objectName, Transform row, string initialValue)
    {
        Text text = FindPanelText(objectName);
        if (text == null)
        {
            text = EnsureChildText(row, objectName);
        }
        else if (text.transform.parent != row)
        {
            text.transform.SetParent(row, false);
        }

        if (string.IsNullOrWhiteSpace(text.text) || text.text == "New Text")
        {
            text.text = initialValue;
        }

        text.gameObject.SetActive(true);
        return text;
    }

    private Text FindPanelText(string objectName)
    {
        Text[] texts = GetComponentsInChildren<Text>(true);
        for (int i = 0; i < texts.Length; i++)
        {
            if (texts[i].name == objectName)
            {
                texts[i].gameObject.SetActive(true);
                return texts[i];
            }
        }

        GameObject target = GameObject.Find(objectName);
        Text sceneText = target != null ? target.GetComponent<Text>() : null;
        if (sceneText != null && sceneText.transform.IsChildOf(transform))
        {
            sceneText.gameObject.SetActive(true);
            return sceneText;
        }

        return null;
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

    private static Image EnsureChildImage(Transform parent, string name)
    {
        Transform child = parent.Find(name);
        if (child == null)
        {
            GameObject childObject = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
            childObject.transform.SetParent(parent, false);
            child = childObject.transform;
        }

        Image image = child.GetComponent<Image>();
        return image != null ? image : child.gameObject.AddComponent<Image>();
    }

    private static RawImage FindOrCreateRawImage(Transform parent, string name)
    {
        Transform existing = parent.Find(name);
        if (existing != null)
        {
            RawImage existingImage = existing.GetComponent<RawImage>();
            if (existingImage != null)
            {
                existingImage.gameObject.SetActive(true);
                return existingImage;
            }
        }

        GameObject target = GameObject.Find(name);
        if (target == null)
        {
            target = new GameObject(name, typeof(RectTransform), typeof(CanvasRenderer), typeof(RawImage));
        }

        target.transform.SetParent(parent, false);
        target.SetActive(true);
        RawImage image = target.GetComponent<RawImage>();
        return image != null ? image : target.AddComponent<RawImage>();
    }

    private static Transform FindPerceptionCardParent(Transform child)
    {
        Transform current = child;
        while (current != null)
        {
            if (current.name.StartsWith("PerceptionCard_", StringComparison.Ordinal))
            {
                return current;
            }

            current = current.parent;
        }

        return null;
    }

    private float GetCardWidth()
    {
        return Mathf.Max(260f, panelWidth - (panelPadding * 2f));
    }

    private float GetInnerWidth()
    {
        return Mathf.Max(220f, GetCardWidth() - (cardPadding * 2f));
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
