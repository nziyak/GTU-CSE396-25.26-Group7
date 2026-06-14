using UnityEngine;

// Bu script, Kurban (Victim) olarak kullanılacak küp/silindir objelerine takılacak.
// Dicle Unity üzerinden bu scripti objeye sürüklediğinde, ekranında "Severity" ayarı çıkacak.
// Oradan her kurban için "TRAPPED", "STANDING" veya "LYING" seçebilecek.
public class VictimInfo : MonoBehaviour
{
    [Header("Yapay Zeka Görüş Çıktısı (Simülasyon)")]
    [Tooltip("Bu kurbanın durumu nedir? (Hoca için simüle ediliyor)")]
    public VictimStatus severity = VictimStatus.STANDING;
}
