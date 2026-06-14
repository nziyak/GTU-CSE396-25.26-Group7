using UnityEngine;

// Bu script doğrudan "Robot" objesine takılacak.
// Tıpkı gerçek kameranın veya sensörün etrafı taraması gibi Unity içinde çalışır.
public class VirtualSensor : MonoBehaviour
{
    [Header("Sensör Ayarları")]
    public float visionRadius = 5f; // Kameranın görüş/algılama mesafesi
    public float obstacleRayDistance = 3f; // Ultrasonik sensör menzili
    [SerializeField] private Transform sensorOrigin;

    [Header("Bağlantılar")]
    public MapManager mapManager;
    public UIManager uiManager;

    private float scanTimer = 0f;
    private float scanInterval = 1f; // Saniyede 1 kere etrafı tara (kasmaması için)

    private void Start()
    {
        // Manager'ları otomatik bul
        if (mapManager == null) mapManager = FindObjectOfType<MapManager>();
        if (uiManager == null) uiManager = FindObjectOfType<UIManager>();
    }

    private void Update()
    {
        scanTimer += Time.deltaTime;
        if (scanTimer >= scanInterval)
        {
            scanTimer = 0f;
            ScanEnvironment();
        }
    }

    private void ScanEnvironment()
    {
        Transform origin = sensorOrigin != null ? sensorOrigin : transform;

        // 1. ENGEL (OBSTACLE) ALGILAMA SİMÜLASYONU (İleriye dönük lazer atışı)
        // Eğer lazer "Obstacle" etiketli bir şeye değerse konsola uyarı yazar.
        RaycastHit hit;
        if (Physics.Raycast(origin.position, origin.forward, out hit, obstacleRayDistance))
        {
            if (hit.collider.CompareTag("Obstacle"))
            {
                Debug.LogWarning($"[MOD-01] Engel Algılandı! Mesafe: {hit.distance:0.0}m. Çarpışma önleme devrede.");
            }
        }

        // 2. KURBAN (VICTIM) ALGILAMA SİMÜLASYONU (YOLO Yapay Zeka Kamerası)
        // Robotun etrafındaki visionRadius (örn 5m) içindeki tüm objeleri tarar.
        Collider[] colliders = Physics.OverlapSphere(origin.position, visionRadius);
        int victimCount = 0;
        VictimStatus lastFoundStatus = VictimStatus.NONE;

        foreach (var col in colliders)
        {
            if (col.CompareTag("Victim"))
            {
                VictimInfo info = col.GetComponent<VictimInfo>();
                if (info != null)
                {
                    victimCount++;
                    lastFoundStatus = info.severity;

                    // Her bulduğu farklı kurban için MapManager'a ayrı bir pin koy komutu yollar
                    TelemetryData mockData = new TelemetryData
                    {
                        posX = col.transform.position.x, 
                        posY = col.transform.position.z, 
                        temperature = 28.5f,
                        smokeDetected = false,
                        victimStatus = info.severity,
                        priorityLevel = ResolvePriorityLevel(info.severity),
                        isStuck = false,
                        acousticHit = false,
                        acousticAngle = 0f
                    };

                    if (mapManager != null) 
                    {
                        mapManager.PlacePin(mockData.posX, mockData.posY, mockData.victimStatus);
                    }

                    Debug.Log($"[MOD-02] Kurban Tespit Edildi! Durum: {info.severity}");
                }
            }
        }

        // Eğer arayüz (UI) varsa sadece bir tane yazdırabiliriz, titremesin diye:
        if (uiManager != null)
        {
            if (victimCount == 0)
            {
                uiManager.UpdateVictimStatus(VictimStatus.NONE);
            }
            else if (victimCount == 1)
            {
                // Sadece 1 kurban varsa onun durumunu direkt yaz
                uiManager.UpdateVictimStatus(lastFoundStatus);
            }
            // NOT: Eğer tam o saniyede 2 kurban birden görüyorsa UI'ı güncellemiyoruz
            // (Zaten haritaya ikisinin de pin'ini çoktan yukarıda basmış oldu!)
        }
    }

    private static int ResolvePriorityLevel(VictimStatus status)
    {
        switch (status)
        {
            case VictimStatus.TRAPPED:
                return MapManagerConstants.PIN_PRIORITY_RED;
            case VictimStatus.LYING:
                return MapManagerConstants.PIN_PRIORITY_YELLOW;
            case VictimStatus.STANDING:
                return MapManagerConstants.PIN_PRIORITY_GREEN;
            default:
                return 0;
        }
    }
}
