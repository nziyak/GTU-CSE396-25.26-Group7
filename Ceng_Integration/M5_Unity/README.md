# MOD-05 Unity Digital Twin Module

**Purpose:** Provides a 3D visualization and situational awareness dashboard for the operator. It receives real-time JSON telemetry to update the robot's position and drop color-coded pins, while capturing operator voice commands (Push-to-Talk) for Edge STT.

**Authors:**
* Dicle [Student ID TBD] (Primary - UI & 3D Map)
* Ziya [Student ID TBD] (Secondary - WebSockets & JSON)
* Evrim [230104004042] (Secondary - Audio Capture)

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
  * **Description:** `UpdateMapPins()`, `UpdateUI()` etc. to be added.
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

## Interface Integration Updates

The following structural changes were applied to the files in this folder as part of the `Ceng_Integration` docx compliance revision:

- **[R9] Renamed all `TelemetryData` fields to match docx §2A JSON schema** (`DataContracts.cs`): `posX→pos_x`, `posY→pos_y`, `temperature→temp`, `smokeDetected→smoke`, `victimStatus→victim_status`, `priorityLevel→priority`. Without this, `JsonUtility.FromJson<TelemetryData>()` would produce all-default values at runtime.
- **[R9] Changed `victimStatus` from `VictimStatus` enum to `string`** (`DataContracts.cs`): M4 Python sends `"TRAPPED"` as a JSON string; Unity `JsonUtility` cannot parse an enum from a string directly.
- **[R10] Added `CommandPacket` class** (`DataContracts.cs`): Docx §2B defines `{"override": true, "cmd": "FORWARD"}` — the `override` flag is essential for M4 to distinguish manual vs. autonomous commands.
- **[R11] Changed `SendOperatorCommand(string)` to `SendOperatorCommand(CommandPacket)`** (`INetworkClient.cs`): Aligns with docx §2B and the new `CommandPacket` class.
- **[R12] Updated docstring references in `MapManager.cs`**: Field name references updated to match new `TelemetryData` fields.
- **[R13] Updated docstring references in `UIManager.cs`**: Internal flow comments updated to reflect `data.temp`, `data.smoke`, `data.victim_status`.
- **`AudioManager.cs`**: No changes required — already compliant with docx §2B audio blob interface.
