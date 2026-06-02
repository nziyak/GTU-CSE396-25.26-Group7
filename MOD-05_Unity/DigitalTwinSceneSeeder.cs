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
    [SerializeField] private int gridWidth = 16;
    [SerializeField] private int gridHeight = 11;
    [SerializeField] private float cellSize = 1f;
    [SerializeField] private Color floorColor = new Color(0.115f, 0.125f, 0.13f, 1f);
    [SerializeField] private Color roomFloorColor = new Color(0.16f, 0.175f, 0.185f, 1f);
    [SerializeField] private Color corridorFloorColor = new Color(0.105f, 0.12f, 0.14f, 1f);
    [SerializeField] private Color gridLineColor = new Color(0.37f, 0.52f, 0.58f, 0.28f);

    [Header("Scene Props")]
    [SerializeField] private Color wallColor = new Color(0.42f, 0.45f, 0.47f, 1f);
    [SerializeField] private Color roomLabelColor = new Color(0.78f, 0.88f, 0.92f, 0.9f);
    [SerializeField] private Color debrisColor = new Color(0.36f, 0.25f, 0.18f, 1f);
    [SerializeField] private Color heatZoneColor = new Color(1f, 0.18f, 0.05f, 0.35f);
    [SerializeField] private bool generateOnStart = true;

    private static readonly Vector2[] VictimPositions =
    {
        new Vector2(2.2f, 2.1f),
        new Vector2(9.8f, 3.2f),
        new Vector2(13.2f, 8.5f)
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
        CreateRoomZones();
        CreateGridLines();
        CreateHouseWalls();
        CreateFurnitureAndHazards();
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

    private void CreateRoomZones()
    {
        CreateRoom("LivingRoom", new Vector3(2f, -0.015f, 2f), new Vector3(4f, 0.035f, 4f), "LIVING");
        CreateRoom("Kitchen", new Vector3(6.7f, -0.015f, 2f), new Vector3(4.2f, 0.035f, 4f), "KITCHEN");
        CreateRoom("Bedroom", new Vector3(12.2f, -0.015f, 2f), new Vector3(4.6f, 0.035f, 4f), "BEDROOM");
        CreateRoom("Bath", new Vector3(2f, -0.015f, 8.2f), new Vector3(4f, 0.035f, 4.4f), "BATH");
        CreateRoom("Storage", new Vector3(6.8f, -0.015f, 8.2f), new Vector3(4.4f, 0.035f, 4.4f), "STORAGE");
        CreateRoom("RescueRoom", new Vector3(12.2f, -0.015f, 8.2f), new Vector3(4.6f, 0.035f, 4.4f), "RESCUE");

        CreateBlock("CentralCorridor", new Vector3(7.5f, -0.01f, 5.3f), new Vector3(15.6f, 0.025f, 1.15f), corridorFloorColor);
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

    private void CreateHouseWalls()
    {
        float wallHeight = 0.68f;
        float wallY = wallHeight * 0.5f;
        float thickness = 0.16f;

        CreateBlock("OuterWall_North_A", new Vector3(3.1f, wallY, -0.55f), new Vector3(6.2f, wallHeight, thickness), wallColor);
        CreateBlock("OuterWall_North_B", new Vector3(11.5f, wallY, -0.55f), new Vector3(7.0f, wallHeight, thickness), wallColor);
        CreateBlock("OuterWall_South", new Vector3(7.5f, wallY, 10.55f), new Vector3(16.1f, wallHeight, thickness), wallColor);
        CreateBlock("OuterWall_West", new Vector3(-0.55f, wallY, 5.0f), new Vector3(thickness, wallHeight, 10.9f), wallColor);
        CreateBlock("OuterWall_East", new Vector3(15.55f, wallY, 5.0f), new Vector3(thickness, wallHeight, 10.9f), wallColor);

        CreateWallWithDoor("Wall_Living_Kitchen", 4.4f, 1.1f, 4.3f, 2.4f, true);
        CreateWallWithDoor("Wall_Kitchen_Bedroom", 9.2f, 1.1f, 4.3f, 2.5f, true);
        CreateWallWithDoor("Wall_Bath_Storage", 4.4f, 6.4f, 4.1f, 7.8f, true);
        CreateWallWithDoor("Wall_Storage_Rescue", 9.2f, 6.4f, 4.1f, 7.8f, true);
        CreateWallWithDoor("Wall_North_Corridor", 7.5f, 4.35f, 15.7f, 7.5f, false);
        CreateWallWithDoor("Wall_South_Corridor", 7.5f, 6.35f, 15.7f, 8.0f, false);
    }

    private void CreateFurnitureAndHazards()
    {
        CreateBlock("Sofa", new Vector3(1.7f, 0.18f, 1.1f), new Vector3(1.9f, 0.35f, 0.7f), new Color(0.2f, 0.3f, 0.38f, 1f));
        CreateBlock("CoffeeTable", new Vector3(2.3f, 0.13f, 2.4f), new Vector3(1.2f, 0.25f, 0.65f), new Color(0.32f, 0.22f, 0.12f, 1f));
        CreateBlock("KitchenCounter", new Vector3(6.3f, 0.22f, 1.0f), new Vector3(2.8f, 0.42f, 0.5f), new Color(0.24f, 0.26f, 0.28f, 1f));
        CreateBlock("Bed", new Vector3(12.7f, 0.18f, 1.45f), new Vector3(2.0f, 0.35f, 1.25f), new Color(0.18f, 0.28f, 0.46f, 1f));
        CreateBlock("BathFixture", new Vector3(1.4f, 0.16f, 8.7f), new Vector3(1.2f, 0.32f, 0.85f), new Color(0.74f, 0.82f, 0.84f, 1f));
        CreateBlock("StorageShelf", new Vector3(7.3f, 0.28f, 8.7f), new Vector3(2.2f, 0.55f, 0.55f), new Color(0.28f, 0.22f, 0.15f, 1f));
        CreateBlock("DebrisPile_A", new Vector3(5.8f, 0.18f, 7.8f), new Vector3(1.6f, 0.35f, 1.1f), debrisColor);
        CreateBlock("DebrisPile_B", new Vector3(11.2f, 0.16f, 7.4f), new Vector3(1.3f, 0.32f, 1.4f), debrisColor);
        CreateBlock("ThermalHazardZone", new Vector3(13.1f, 0.02f, 7.7f), new Vector3(2.4f, 0.03f, 2.0f), heatZoneColor);
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

    private void CreateRoom(string name, Vector3 position, Vector3 scale, string label)
    {
        CreateBlock($"{name}_Floor", position, scale, roomFloorColor);
        CreateRoomLabel($"{name}_Label", label, new Vector3(position.x, 0.055f, position.z));
    }

    private void CreateWallWithDoor(string name, float centerX, float centerZ, float length, float doorCenter, bool vertical)
    {
        float wallHeight = 0.68f;
        float wallY = wallHeight * 0.5f;
        float thickness = 0.16f;
        float doorWidth = 1.05f;
        float half = length * 0.5f;
        float doorOffset = doorCenter - (vertical ? centerZ : centerX);
        float firstLength = Mathf.Max(0.2f, half + doorOffset - doorWidth * 0.5f);
        float secondLength = Mathf.Max(0.2f, half - doorOffset - doorWidth * 0.5f);

        if (vertical)
        {
            float firstCenter = centerZ - half + firstLength * 0.5f;
            float secondCenter = centerZ + half - secondLength * 0.5f;
            CreateBlock($"{name}_A", new Vector3(centerX, wallY, firstCenter), new Vector3(thickness, wallHeight, firstLength), wallColor);
            CreateBlock($"{name}_B", new Vector3(centerX, wallY, secondCenter), new Vector3(thickness, wallHeight, secondLength), wallColor);
            return;
        }

        float firstX = centerX - half + firstLength * 0.5f;
        float secondX = centerX + half - secondLength * 0.5f;
        CreateBlock($"{name}_A", new Vector3(firstX, wallY, centerZ), new Vector3(firstLength, wallHeight, thickness), wallColor);
        CreateBlock($"{name}_B", new Vector3(secondX, wallY, centerZ), new Vector3(secondLength, wallHeight, thickness), wallColor);
    }

    private void CreateRoomLabel(string name, string text, Vector3 position)
    {
        GameObject label = new GameObject(name);
        label.transform.SetParent(environmentParent, false);
        label.transform.localPosition = position;
        label.transform.localRotation = Quaternion.Euler(90f, 0f, 0f);

        TextMesh mesh = label.AddComponent<TextMesh>();
        mesh.text = text;
        mesh.anchor = TextAnchor.MiddleCenter;
        mesh.alignment = TextAlignment.Center;
        mesh.characterSize = 0.18f;
        mesh.fontSize = 48;
        mesh.color = roomLabelColor;
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
