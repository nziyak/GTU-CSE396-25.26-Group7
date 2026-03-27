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
API SummaryFunction / EventParametersReturn ValueDescriptionConnectstring ipAddressvoidConnects to the Pi 5 WebSocket server.DisconnectvoidvoidGracefully closes the socket connection.SendOperatorCommandstring commandvoidSends manual override commands to Pi.SendAudioBlobbyte[] wavDatavoidTransmits recorded audio for STT.OnTelemetryReceivedTelemetryData dataevent ActionTriggered when JSON is deserialized.TODO: [Dicle'nin Fonks.]......UpdateMapPins(), UpdateUI() vb. eklenecek.TODO: [Evrim'in Fonks.]......StartRecording(), StopAndEncode() vb. eklenecek.Known Limitations and TODOSTODO: Decide on exact Unity coordinate system scaling relative to physical arena (e.g., 1 Unity Unit = 10 cm).Limitation: Audio blobs must be compressed or kept short (<5 seconds) to prevent WebSocket buffer overflow.Version Historyv0.1 (2026-03-18): Initial draft, defined Network Client interfaces and JSON Telemetry structures.