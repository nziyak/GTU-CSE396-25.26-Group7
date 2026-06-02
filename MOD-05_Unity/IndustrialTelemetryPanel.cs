/// <summary>
/// File:    IndustrialTelemetryPanel.cs
/// Brief:   Turns the left telemetry list into a dark industrial HUD/FUI panel.
/// </summary>

using System;
using UnityEngine;
using UnityEngine.UI;

[ExecuteAlways]
public class IndustrialTelemetryPanel : MonoBehaviour
{
    [Serializable]
    public class HudTheme
    {
        public Color panelBackground = new Color(0.015f, 0.021f, 0.027f, 0.78f);
        public Color cardBackground = new Color(0.04f, 0.055f, 0.066f, 0.92f);
        public Color cardBorder = new Color(0.08f, 0.55f, 0.72f, 0.55f);
        public Color titleColor = new Color(0.24f, 0.95f, 1f, 1f);
        public Color labelColor = new Color(0.58f, 0.70f, 0.76f, 1f);
        public Color valueColor = new Color(0.90f, 0.98f, 1f, 1f);
        public Color okColor = new Color(0.2f, 1f, 0.42f, 1f);
        public Color warningColor = new Color(1f, 0.72f, 0.12f, 1f);
        public Color dangerColor = new Color(1f, 0.16f, 0.20f, 1f);
        public Color mutedColor = new Color(0.38f, 0.48f, 0.54f, 1f);
    }

    [Serializable]
    public class MetricBinding
    {
        public string textObjectName;
        public string icon;
        public string label;
        public bool prominent;

        public MetricBinding(string textObjectName, string icon, string label, bool prominent = false)
        {
            this.textObjectName = textObjectName;
            this.icon = icon;
            this.label = label;
            this.prominent = prominent;
        }
    }

    [Serializable]
    public class TelemetryCard
    {
        public string title;
        public MetricBinding[] metrics;

        public TelemetryCard(string title, params MetricBinding[] metrics)
        {
            this.title = title;
            this.metrics = metrics;
        }
    }

    [SerializeField] private HudTheme theme = new HudTheme();
    [SerializeField] private int titleFontSize = 14;
    [SerializeField] private int valueFontSize = 12;
    [SerializeField] private int prominentValueFontSize = 15;
    [SerializeField] private float cardSpacing = 10f;
    [SerializeField] private float refreshIntervalSeconds = 0.12f;
    [SerializeField] private float panelWidth = 360f;
    [SerializeField] private float panelPadding = 12f;
    [SerializeField] private float cardHeight = 122f;
    [SerializeField] private float cardPadding = 10f;
    [SerializeField] private float titleHeight = 24f;
    [SerializeField] private float rowHeight = 27f;
    [SerializeField] private float iconWidth = 34f;
    [SerializeField] private float labelWidth = 78f;
    [SerializeField] private float columnGap = 6f;

    private TelemetryCard[] cards;
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

    [ContextMenu("Apply Industrial Telemetry Style")]
    public void ApplyStyle()
    {
        EnsureCardConfig();
        StyleRootPanel();
        HideLoosePanelTexts();

        for (int cardIndex = 0; cardIndex < cards.Length; cardIndex++)
        {
            Transform card = EnsureCard(cards[cardIndex], cardIndex);
            for (int metricIndex = 0; metricIndex < cards[cardIndex].metrics.Length; metricIndex++)
            {
                BuildMetricRow(card, cards[cardIndex].metrics[metricIndex], metricIndex);
            }
        }

        RefreshDynamicColors();
        LayoutRebuilder.ForceRebuildLayoutImmediate(GetComponent<RectTransform>());
    }

    private void EnsureCardConfig()
    {
        if (cards != null && cards.Length > 0)
        {
            return;
        }

        cards = new[]
        {
            new TelemetryCard(
                "LINK",
                new MetricBinding("ConnectionStatusText", "NET", "Connection", true),
                new MetricBinding("LatencyText", "SIG", "Latency"),
                new MetricBinding("UartConnectionText", "UART", "Serial")
            ),
            new TelemetryCard(
                "ROBOT",
                new MetricBinding("PositionText", "XY", "Position", true),
                new MetricBinding("StuckStatusText", "WARN", "Mobility"),
                new MetricBinding("UartDistanceText", "DIST", "Distance")
            ),
            new TelemetryCard(
                "ENVIRONMENT",
                new MetricBinding("TemperatureText", "TEMP", "Temperature", true),
                new MetricBinding("SmokeStatusText", "SMK", "Smoke"),
                new MetricBinding("UartEnvironmentText", "HUM", "Temp / Hum")
            ),
            new TelemetryCard(
                "PERCEPTION",
                new MetricBinding("VictimStatusText", "VIC", "Victim", true),
                new MetricBinding("PriorityLevelText", "PRI", "Priority"),
                new MetricBinding("AcousticStatusText", "AUD", "Acoustic")
            ),
            new TelemetryCard(
                "DIAGNOSTICS",
                new MetricBinding("UartOrientationText", "IMU", "Orientation"),
                new MetricBinding("UartMicrophoneText", "MIC", "Microphone"),
                new MetricBinding("SpeechCommandText", "PTT", "Voice", true)
            )
        };
    }

    private void StyleRootPanel()
    {
        Image image = GetComponent<Image>();
        if (image == null)
        {
            image = gameObject.AddComponent<Image>();
        }

        image.color = theme.panelBackground;

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

    private Transform EnsureCard(TelemetryCard config, int index)
    {
        string objectName = $"TelemetryCard_{index}_{config.title}";
        Transform card = transform.Find(objectName);
        if (card == null)
        {
            GameObject cardObject = new GameObject(objectName, typeof(RectTransform), typeof(CanvasRenderer), typeof(Image));
            cardObject.transform.SetParent(transform, false);
            card = cardObject.transform;
        }

        float cardWidth = Mathf.Max(260f, panelWidth - (panelPadding * 2f));
        RectTransform cardRect = card.GetComponent<RectTransform>();
        SetTopLeftRect(cardRect, panelPadding, -panelPadding - (index * (cardHeight + cardSpacing)), cardWidth, cardHeight);

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
        element.minHeight = cardHeight;
        element.preferredHeight = cardHeight;
        element.flexibleHeight = 0f;

        EnsureTitle(card, config.title);
        return card;
    }

    private void EnsureTitle(Transform card, string title)
    {
        Transform titleTransform = card.Find("Title");
        if (titleTransform == null)
        {
            GameObject titleObject = new GameObject("Title", typeof(RectTransform), typeof(CanvasRenderer), typeof(Text));
            titleObject.transform.SetParent(card, false);
            titleTransform = titleObject.transform;
        }

        RectTransform titleRect = titleTransform.GetComponent<RectTransform>();
        SetTopLeftRect(titleRect, cardPadding, -6f, GetCardInnerWidth(), titleHeight);

        Text titleText = titleTransform.GetComponent<Text>();
        titleText.text = title;
        titleText.font = GetBuiltinUiFont();
        titleText.fontSize = titleFontSize;
        titleText.fontStyle = FontStyle.Bold;
        titleText.alignment = TextAnchor.MiddleLeft;
        titleText.color = theme.titleColor;

        LayoutElement element = titleTransform.GetComponent<LayoutElement>();
        if (element == null)
        {
            element = titleTransform.gameObject.AddComponent<LayoutElement>();
        }

        element.ignoreLayout = true;
        element.preferredHeight = titleHeight;
    }

    private void BuildMetricRow(Transform card, MetricBinding binding, int index)
    {
        string rowName = $"Row_{index}_{binding.textObjectName}";
        Transform row = card.Find(rowName);
        if (row == null)
        {
            GameObject rowObject = new GameObject(rowName, typeof(RectTransform));
            rowObject.transform.SetParent(card, false);
            row = rowObject.transform;
        }

        float rowY = -6f - titleHeight - 5f - (index * rowHeight);
        RectTransform rowRect = row.GetComponent<RectTransform>();
        SetTopLeftRect(rowRect, cardPadding, rowY, GetCardInnerWidth(), rowHeight);

        LayoutGroup[] layouts = row.GetComponents<LayoutGroup>();
        for (int i = 0; i < layouts.Length; i++)
        {
            layouts[i].enabled = false;
        }

        LayoutElement rowElement = row.GetComponent<LayoutElement>();
        if (rowElement == null)
        {
            rowElement = row.gameObject.AddComponent<LayoutElement>();
        }

        rowElement.ignoreLayout = true;
        rowElement.minHeight = rowHeight;
        rowElement.preferredHeight = rowHeight;
        rowElement.flexibleHeight = 0f;

        Text iconText = EnsureChildText(row, "Icon");
        SetTopLeftRect(iconText.GetComponent<RectTransform>(), 0f, 0f, iconWidth, rowHeight);
        iconText.text = binding.icon;
        iconText.fontSize = 10;
        iconText.fontStyle = FontStyle.Bold;
        iconText.color = theme.titleColor;
        iconText.alignment = TextAnchor.MiddleCenter;

        Text labelText = EnsureChildText(row, "Label");
        float labelX = iconWidth + columnGap;
        SetTopLeftRect(labelText.GetComponent<RectTransform>(), labelX, 0f, labelWidth, rowHeight);
        labelText.text = binding.label.ToUpperInvariant();
        labelText.fontSize = 10;
        labelText.fontStyle = FontStyle.Normal;
        labelText.color = theme.labelColor;
        labelText.alignment = TextAnchor.MiddleLeft;

        Text valueText = FindMetricText(binding.textObjectName, row);
        if (valueText == null)
        {
            valueText = EnsureChildText(row, binding.textObjectName);
        }
        else if (valueText.transform.parent != row)
        {
            valueText.transform.SetParent(row, false);
        }

        float valueX = iconWidth + labelWidth + (columnGap * 2f);
        float valueWidth = Mathf.Max(130f, GetCardInnerWidth() - valueX);
        SetTopLeftRect(valueText.GetComponent<RectTransform>(), valueX, 0f, valueWidth, rowHeight);
        valueText.font = GetBuiltinUiFont();
        valueText.fontSize = binding.prominent ? prominentValueFontSize : valueFontSize;
        valueText.fontStyle = binding.prominent ? FontStyle.Bold : FontStyle.Normal;
        valueText.alignment = TextAnchor.MiddleLeft;
        valueText.horizontalOverflow = HorizontalWrapMode.Wrap;
        valueText.verticalOverflow = VerticalWrapMode.Truncate;
        valueText.color = theme.valueColor;
    }

    private void RefreshDynamicColors()
    {
        ColorByContent("ConnectionStatusText", theme.okColor, theme.warningColor, theme.dangerColor);
        ColorByContent("UartConnectionText", theme.okColor, theme.warningColor, theme.dangerColor);
        ColorByContent("SmokeStatusText", theme.okColor, theme.warningColor, theme.dangerColor);
        ColorByContent("StuckStatusText", theme.okColor, theme.warningColor, theme.dangerColor);
        ColorByContent("VictimStatusText", theme.valueColor, theme.warningColor, theme.dangerColor);
        ColorByContent("PriorityLevelText", theme.valueColor, theme.warningColor, theme.dangerColor);
        ColorByContent("AcousticStatusText", theme.okColor, theme.warningColor, theme.titleColor);
        ColorByContent("SpeechCommandText", theme.valueColor, theme.titleColor, theme.warningColor);
    }

    private void ColorByContent(string textObjectName, Color normal, Color warning, Color danger)
    {
        Text text = FindPanelText(textObjectName);
        if (text == null)
        {
            return;
        }

        string value = text.text.ToUpperInvariant();
        if (value.Contains("DISCONNECTED") || value.Contains("STUCK") || value.Contains("SMOKE DETECTED") || value.Contains("TRAPPED"))
        {
            text.color = danger;
            return;
        }

        if (value.Contains("CONNECTING") || value.Contains("LYING") || value.Contains("PRIORITY 2") || value.Contains("WARN"))
        {
            text.color = warning;
            return;
        }

        if (value.Contains("--") || value.Contains("UNKNOWN"))
        {
            text.color = theme.mutedColor;
            return;
        }

        text.color = normal;
    }

    private bool IsManagedMetric(string objectName)
    {
        EnsureCardConfig();
        for (int cardIndex = 0; cardIndex < cards.Length; cardIndex++)
        {
            MetricBinding[] metrics = cards[cardIndex].metrics;
            for (int metricIndex = 0; metricIndex < metrics.Length; metricIndex++)
            {
                if (metrics[metricIndex].textObjectName == objectName)
                {
                    return true;
                }
            }
        }

        return false;
    }

    private void HideLoosePanelTexts()
    {
        Text[] texts = GetComponentsInChildren<Text>(true);
        for (int i = 0; i < texts.Length; i++)
        {
            Transform textTransform = texts[i].transform;
            bool isCardChild = FindTelemetryCardParent(textTransform) != null;
            if (!isCardChild)
            {
                texts[i].gameObject.SetActive(false);
            }
        }
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

    private static Transform FindTelemetryCardParent(Transform child)
    {
        Transform current = child;
        while (current != null)
        {
            if (current.name.StartsWith("TelemetryCard_", StringComparison.Ordinal))
            {
                return current;
            }

            current = current.parent;
        }

        return null;
    }

    private float GetCardInnerWidth()
    {
        return Mathf.Max(220f, panelWidth - (panelPadding * 2f) - (cardPadding * 2f));
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
        Font font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
        return font != null ? font : Resources.GetBuiltinResource<Font>("Arial.ttf");
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

    private Text FindMetricText(string objectName, Transform row)
    {
        Transform direct = row.Find(objectName);
        if (direct != null)
        {
            Text directText = direct.GetComponent<Text>();
            if (directText != null)
            {
                directText.gameObject.SetActive(true);
                return directText;
            }
        }

        Text[] texts = GetComponentsInChildren<Text>(true);
        for (int i = 0; i < texts.Length; i++)
        {
            if (texts[i].name != objectName)
            {
                continue;
            }

            texts[i].gameObject.SetActive(true);
            return texts[i];
        }

        return null;
    }

}
