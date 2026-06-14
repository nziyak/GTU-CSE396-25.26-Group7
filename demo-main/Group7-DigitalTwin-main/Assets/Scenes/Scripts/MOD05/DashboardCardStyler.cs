/// <summary>
/// File:    DashboardCardStyler.cs
/// Brief:   Applies a clean industrial card style to dashboard UI panels.
/// </summary>

using UnityEngine;
using UnityEngine.UI;

public class DashboardCardStyler : MonoBehaviour
{
    [Header("Colors")]
    [SerializeField] private Color panelColor = new Color(0.055f, 0.06f, 0.07f, 0.96f);
    [SerializeField] private Color cardColor = new Color(0.12f, 0.135f, 0.16f, 0.98f);
    [SerializeField] private Color textColor = new Color(0.93f, 0.96f, 0.98f, 1f);

    [Header("Layout")]
    [SerializeField] private int padding = 14;
    [SerializeField] private int spacing = 10;
    [SerializeField] private float cardHeight = 58f;
    [SerializeField] private int fontSize = 18;

    [ContextMenu("Apply Dashboard Style")]
    public void ApplyDashboardStyle()
    {
        StylePanel(transform);
        StyleCards(transform);
    }

    private void Reset()
    {
        ApplyDashboardStyle();
    }

    private void StylePanel(Transform panel)
    {
        Image panelImage = panel.GetComponent<Image>();
        if (panelImage == null)
        {
            panelImage = panel.gameObject.AddComponent<Image>();
        }

        panelImage.color = panelColor;

        VerticalLayoutGroup layout = panel.GetComponent<VerticalLayoutGroup>();
        if (layout == null)
        {
            layout = panel.gameObject.AddComponent<VerticalLayoutGroup>();
        }

        layout.padding = new RectOffset(padding, padding, padding, padding);
        layout.spacing = spacing;
        layout.childAlignment = TextAnchor.UpperCenter;
        layout.childControlWidth = true;
        layout.childControlHeight = false;
        layout.childForceExpandWidth = true;
        layout.childForceExpandHeight = false;
    }

    private void StyleCards(Transform panel)
    {
        for (int i = 0; i < panel.childCount; i++)
        {
            Transform card = panel.GetChild(i);
            Image cardImage = card.GetComponent<Image>();
            if (cardImage == null)
            {
                cardImage = card.gameObject.AddComponent<Image>();
            }

            cardImage.color = cardColor;

            LayoutElement layoutElement = card.GetComponent<LayoutElement>();
            if (layoutElement == null)
            {
                layoutElement = card.gameObject.AddComponent<LayoutElement>();
            }

            layoutElement.preferredHeight = cardHeight;
            layoutElement.flexibleHeight = 0f;

            for (int childIndex = 0; childIndex < card.childCount; childIndex++)
            {
                Text text = card.GetChild(childIndex).GetComponent<Text>();
                if (text == null)
                {
                    continue;
                }

                text.color = textColor;
                text.fontSize = fontSize;
                text.alignment = TextAnchor.MiddleLeft;
                text.horizontalOverflow = HorizontalWrapMode.Overflow;
                text.verticalOverflow = VerticalWrapMode.Truncate;

                RectTransform rect = text.GetComponent<RectTransform>();
                rect.anchorMin = Vector2.zero;
                rect.anchorMax = Vector2.one;
                rect.offsetMin = new Vector2(14f, 6f);
                rect.offsetMax = new Vector2(-14f, -6f);
                rect.localScale = Vector3.one;
            }
        }
    }
}
