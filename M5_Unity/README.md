# MOD-05 Unity Digital Twin Module

**Purpose:** Provides a 3D visualization and situational awareness dashboard for the operator. It receives real-time JSON telemetry to update the robot's position and drop color-coded pins, while capturing operator voice commands (Push-to-Talk) for Edge STT.

**Authors:**
* Dicle [Öğrenci Nonu Yaz] (Primary - UI & 3D Map)
* Ziya [Öğrenci Nonu Yaz] (Secondary - WebSockets & JSON)
* Evrim [Öğrenci Nonu Yaz] (Secondary - Audio Capture)

**Dependencies:**
* Unity Engine (2022.3 or newer)
* WebSocket-Sharp (or Unity Native WebSockets)
* Newtonsoft.Json (for serialization)

### Quick-Start Integration Example
```csharp
using UnityEngine;

public class RobotManager : MonoBehaviour 
{
    private INetworkClient networkClient;

    void Start() {
        networkClient = new WebSocketClient();
        networkClient.OnTelemetryReceived += UpdateDashboard;
        networkClient.Connect("ws://192.168.1.10:5000");
    }

    void UpdateDashboard(TelemetryData data) {
        Debug.Log($"Robot is at X:{data.posX}, Y:{data.posY} with victim status: {data.victimStatus}");
    }
}
```

### API Summary

* **`void Connect(string ipAddress)`**
  * **Description:** Connects to the Pi 5 WebSocket server.
* **`void Disconnect(void)`**
  * **Description:** Gracefully closes the socket connection.
* **`void SendOperatorCommand(string command)`**
  * **Description:** Sends manual override commands to Pi.
* **`void SendAudioBlob(byte[] wavData)`**
  * **Description:** Transmits recorded audio for STT.
* **`event Action OnTelemetryReceived(TelemetryData data)`**
  * **Description:** Triggered when JSON is deserialized.
* **`TODO: [Dicle]`**
  * **Description:** `UpdateMapPins()`, `UpdateUI()` vb. eklenecek.
* `void StartRecording()`
   * Description: Starts microphone capture. Bind to PTT button's OnPointerDown event.
* `void StopAndEncode()`
   * Description: Stops capture, encodes AudioClip to .wav, sends via INetworkClient.SendAudioBlob().
* `void SetNetworkClient(INetworkClient client)`
   * Description: Injects Ziya's network client. Must be called before any recording.
* `AudioCaptureState GetCaptureState()`
   * Description: Returns current state: Idle / Recording / Encoding / Sending.
* `event Action<byte[]> OnAudioBlobReady`
   * Description: Fired after encoding, before sending. UIManager subscribes for HUD feedback.
