/// <summary>
/// File:    DigitalTwinSceneSeeder.cs
/// Brief:   Builds a presentation-ready sample rescue environment around MapRoot.
/// </summary>

using UnityEngine;

public class DigitalTwinSceneSeeder : MonoBehaviour
{
    [Header("Parents")]
    [SerializeField] private Transform environmentParent;
    [SerializeField] private Transform victimParent;

    [Header("Grid")]
    [SerializeField] private int gridWidth = 14;
    [SerializeField] private int gridHeight = 10;
    [SerializeField] private float cellSize = 1f;
    [SerializeField] private Color floorColor = new Color(0.18f, 0.18f, 0.16f, 1f);
    [SerializeField] private Color gridLineColor = new Color(0.38f, 0.42f, 0.44f, 0.45f);

    [Header("Scene Props")]
    [SerializeField] private Color wallColor = new Color(0.28f, 0.29f, 0.31f, 1f);
    [SerializeField] private Color debrisColor = new Color(0.36f, 0.25f, 0.18f, 1f);
    [SerializeField] private Color heatZoneColor = new Color(1f, 0.18f, 0.05f, 0.35f);
    [SerializeField] private bool generateOnStart = true;

    private static readonly Vector2[] VictimPositions =
    {
        new Vector2(2f, 2f),
        new Vector2(7f, 4f),
        new Vector2(11f, 8f)
    };

    private static readonly VictimStatus[] VictimStatuses =
    {
        VictimStatus.TRAPPED,
        VictimStatus.LYING,
        VictimStatus.STANDING
    };

    private void Start()
    {
        if (generateOnStart)
        {
            GenerateSampleEnvironment();
        }
    }

    [ContextMenu("Generate Sample Environment")]
    public void GenerateSampleEnvironment()
    {
        EnsureParents();
        ClearChildren(environmentParent);
        ClearChildren(victimParent);

        CreateFloor();
        CreateGridLines();
        CreateProps();
        CreateVictims();
    }

    private void EnsureParents()
    {
        if (environmentParent == null)
        {
            GameObject environment = new GameObject("GeneratedEnvironment");
            environment.transform.SetParent(transform, false);
            environmentParent = environment.transform;
        }

        if (victimParent == null)
        {
            GameObject victims = new GameObject("GeneratedVictims");
            victims.transform.SetParent(transform, false);
            victimParent = victims.transform;
        }
    }

    private void CreateFloor()
    {
        GameObject floor = GameObject.CreatePrimitive(PrimitiveType.Cube);
        floor.name = "RescueGridFloor";
        floor.transform.SetParent(environmentParent, false);
        floor.transform.localPosition = new Vector3((gridWidth - 1) * cellSize * 0.5f, -0.04f, (gridHeight - 1) * cellSize * 0.5f);
        floor.transform.localScale = new Vector3(gridWidth * cellSize, 0.04f, gridHeight * cellSize);
        ApplyColor(floor, floorColor);
    }

    private void CreateGridLines()
    {
        for (int x = 0; x <= gridWidth; x++)
        {
            CreateLine("GridLineX", new Vector3((x - 0.5f) * cellSize, 0.01f, -0.5f * cellSize), new Vector3((x - 0.5f) * cellSize, 0.01f, (gridHeight - 0.5f) * cellSize));
        }

        for (int y = 0; y <= gridHeight; y++)
        {
            CreateLine("GridLineZ", new Vector3(-0.5f * cellSize, 0.01f, (y - 0.5f) * cellSize), new Vector3((gridWidth - 0.5f) * cellSize, 0.01f, (y - 0.5f) * cellSize));
        }
    }

    private void CreateProps()
    {
        CreateBlock("CollapsedWall_A", new Vector3(4f, 0.35f, 2f), new Vector3(2.8f, 0.7f, 0.35f), wallColor);
        CreateBlock("CollapsedWall_B", new Vector3(9f, 0.35f, 6f), new Vector3(0.35f, 0.7f, 3f), wallColor);
        CreateBlock("DebrisPile_A", new Vector3(3f, 0.18f, 7f), new Vector3(1.4f, 0.35f, 1.1f), debrisColor);
        CreateBlock("DebrisPile_B", new Vector3(10f, 0.16f, 2f), new Vector3(1.2f, 0.32f, 1.4f), debrisColor);
        CreateBlock("ThermalHazardZone", new Vector3(6f, 0.02f, 7f), new Vector3(2.4f, 0.03f, 2.4f), heatZoneColor);
    }

    private void CreateVictims()
    {
        for (int i = 0; i < VictimPositions.Length; i++)
        {
            Vector2 position = VictimPositions[i];
            VictimStatus status = VictimStatuses[i];
            GameObject marker = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
            marker.name = $"Victim_{status}_{i + 1}";
            marker.transform.SetParent(victimParent, false);
            marker.transform.localPosition = new Vector3(position.x, 0.08f, position.y);
            marker.transform.localScale = new Vector3(0.45f, 0.08f, 0.45f);
            ApplyColor(marker, ResolveVictimColor(status));

            GameObject beacon = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            beacon.name = $"VictimBeacon_{status}_{i + 1}";
            beacon.transform.SetParent(marker.transform, false);
            beacon.transform.localPosition = new Vector3(0f, 1.6f, 0f);
            beacon.transform.localScale = Vector3.one * 0.35f;
            ApplyColor(beacon, ResolveVictimColor(status));
        }
    }

    private void CreateBlock(string name, Vector3 position, Vector3 scale, Color color)
    {
        GameObject block = GameObject.CreatePrimitive(PrimitiveType.Cube);
        block.name = name;
        block.transform.SetParent(environmentParent, false);
        block.transform.localPosition = position;
        block.transform.localScale = scale;
        ApplyColor(block, color);
    }

    private void CreateLine(string name, Vector3 start, Vector3 end)
    {
        GameObject lineObject = new GameObject(name);
        lineObject.transform.SetParent(environmentParent, false);
        LineRenderer line = lineObject.AddComponent<LineRenderer>();
        line.positionCount = 2;
        line.useWorldSpace = false;
        line.startWidth = 0.015f;
        line.endWidth = 0.015f;
        line.material = new Material(Shader.Find("Sprites/Default"));
        line.startColor = gridLineColor;
        line.endColor = gridLineColor;
        line.SetPosition(0, start);
        line.SetPosition(1, end);
    }

    private static void ApplyColor(GameObject target, Color color)
    {
        Renderer renderer = target.GetComponent<Renderer>();
        if (renderer != null)
        {
            renderer.material.color = color;
        }
    }

    private static Color ResolveVictimColor(VictimStatus status)
    {
        switch (status)
        {
            case VictimStatus.TRAPPED:
                return new Color(1f, 0.04f, 0.02f, 1f);
            case VictimStatus.LYING:
                return new Color(1f, 0.48f, 0.02f, 1f);
            case VictimStatus.STANDING:
                return new Color(1f, 0.92f, 0.08f, 1f);
            default:
                return Color.white;
        }
    }

    private static void ClearChildren(Transform parent)
    {
        for (int i = parent.childCount - 1; i >= 0; i--)
        {
            Destroy(parent.GetChild(i).gameObject);
        }
    }
}
