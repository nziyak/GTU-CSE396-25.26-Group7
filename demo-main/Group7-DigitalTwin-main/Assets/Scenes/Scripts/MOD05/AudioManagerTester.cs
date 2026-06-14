using UnityEngine;
using System.IO;

/// <summary>
/// AudioManager Test Script — Unity Editor icinde AudioManager'i test eder.
/// 
/// Kullanim:
///   1. Sahneye bos bir GameObject olustur ("AudioTester" adinda).
///   2. Bu scripti o objeye surukle-birak.
///   3. Play'e bas.
///   4. Space tusuna basili tut = kayit, birakinca = durdur + WAV kaydet.
///   5. Console'da sonuclari gor.
///   6. Kaydedilen .wav dosyasini Assets/TestRecordings/ altinda bul ve dinle.
///
/// Author: Evrim Doga Solmaz 230104004042
/// </summary>
public class AudioManagerTester : MonoBehaviour
{
    private AudioManager audioManager;
    private bool isRecording = false;

    // Kayitlarin kaydedilecegi klasor
    private string outputFolder;

    void Start()
    {
        // 1. AudioManager olustur
        audioManager = new AudioManager();

        // 2. WAV blob'u yakalamak icin event'e abone ol
        audioManager.OnAudioBlobReady += OnWavBlobReceived;
        audioManager.OnCaptureStateChanged += OnStateChanged;

        // 3. Cikti klasorunu ayarla
        outputFolder = Path.Combine(Application.dataPath, "TestRecordings");
        if (!Directory.Exists(outputFolder))
            Directory.CreateDirectory(outputFolder);

        // 4. Mikrofon kontrolu
        if (Microphone.devices.Length == 0)
        {
            Debug.LogError("[AudioTester] HATA: Mikrofon bulunamadi!");
            return;
        }
        Debug.Log($"[AudioTester] Mikrofon bulundu: {Microphone.devices[0]}");
        Debug.Log("[AudioTester] SPACE tusuna basili tut = kayit, birak = durdur + WAV kaydet");
        Debug.Log($"[AudioTester] Sample Rate: {AudioManagerConstants.AUDIO_SAMPLE_RATE_HZ} Hz");
        Debug.Log($"[AudioTester] Max Sure: {AudioManagerConstants.AUDIO_MAX_RECORD_SECS} sn");
    }

    void Update()
    {
        // Space basilinca kayda basla
        if (Input.GetKeyDown(KeyCode.Space) && !isRecording)
        {
            Debug.Log("[AudioTester] >>> KAYIT BASLADI (Space basili tut...)");
            audioManager.StartRecording();
            isRecording = true;
        }

        // Space birakilinca durdur + encode et
        if (Input.GetKeyUp(KeyCode.Space) && isRecording)
        {
            Debug.Log("[AudioTester] >>> KAYIT DURDURULDU, WAV encode ediliyor...");
            audioManager.StopAndEncode();
            isRecording = false;
        }
    }

    /// <summary>
    /// AudioManager WAV blob'u urettiginde tetiklenir.
    /// Dosyaya kaydeder + header'i dogrular.
    /// </summary>
    private void OnWavBlobReceived(byte[] wavData)
    {
        Debug.Log($"[AudioTester] WAV blob alindi: {wavData.Length} bytes");

        // --- Header Dogrulama ---
        bool headerOk = true;

        // RIFF marker
        string riff = System.Text.Encoding.ASCII.GetString(wavData, 0, 4);
        if (riff != "RIFF") { Debug.LogError($"[AudioTester] HATA: RIFF marker yanlis: {riff}"); headerOk = false; }

        // WAVE marker
        string wave = System.Text.Encoding.ASCII.GetString(wavData, 8, 4);
        if (wave != "WAVE") { Debug.LogError($"[AudioTester] HATA: WAVE marker yanlis: {wave}"); headerOk = false; }

        // Audio format (PCM = 1)
        short audioFormat = System.BitConverter.ToInt16(wavData, 20);
        if (audioFormat != 1) { Debug.LogError($"[AudioTester] HATA: AudioFormat={audioFormat}, PCM(1) olmali"); headerOk = false; }

        // Sample rate
        int sampleRate = System.BitConverter.ToInt32(wavData, 24);
        if (sampleRate != AudioManagerConstants.AUDIO_SAMPLE_RATE_HZ)
        { Debug.LogError($"[AudioTester] HATA: SampleRate={sampleRate}, {AudioManagerConstants.AUDIO_SAMPLE_RATE_HZ} olmali"); headerOk = false; }

        // Bits per sample
        short bitsPerSample = System.BitConverter.ToInt16(wavData, 34);
        if (bitsPerSample != 16) { Debug.LogError($"[AudioTester] HATA: BitsPerSample={bitsPerSample}, 16 olmali"); headerOk = false; }

        if (headerOk)
            Debug.Log("[AudioTester] WAV HEADER DOGRULANDI: RIFF/WAVE/PCM/16kHz/16-bit [OK]");

        // --- Dosyaya Kaydet ---
        string timestamp = System.DateTime.Now.ToString("yyyyMMdd_HHmmss");
        string filePath = Path.Combine(outputFolder, $"test_record_{timestamp}.wav");
        File.WriteAllBytes(filePath, wavData);
        Debug.Log($"[AudioTester] WAV dosyasi kaydedildi: {filePath}");
        Debug.Log($"[AudioTester] Dosyayi dinlemek icin Assets/TestRecordings/ klasorune bak!");
    }

    /// <summary>
    /// Durum degisikliklerini logla.
    /// </summary>
    private void OnStateChanged(AudioCaptureState newState)
    {
        Debug.Log($"[AudioTester] Durum: {newState}");
    }

    void OnDestroy()
    {
        if (audioManager != null)
        {
            audioManager.OnAudioBlobReady -= OnWavBlobReceived;
            audioManager.OnCaptureStateChanged -= OnStateChanged;
        }
    }
}
