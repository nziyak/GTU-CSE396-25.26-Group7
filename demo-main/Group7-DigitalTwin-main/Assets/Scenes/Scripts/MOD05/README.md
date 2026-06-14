# MOD-05 Unity Digital Twin Module

## Purpose

This module provides the Unity-side operator dashboard for the rescue robot. It receives telemetry from the Raspberry Pi, visualizes robot position and victim status on the map, updates the HUD, and captures Push-to-Talk audio for Edge STT processing.

---

## Authors

* **Dicle Çoban** `220104004088` — Primary — UI and map visualization
* **Ziya** `210104004027` — Secondary — WebSocket and JSON integration
* **Evrim Doğa Solmaz** `[230104004042]` — Secondary — Audio capture pipeline

## Dependencies

### Software

* Unity Engine (`2022.3` or newer)
* C# / Unity scripting runtime
* A WebSocket client library such as WebSocket-Sharp or Unity Native WebSockets

### Inter-Module Dependencies

* **MOD-04 Web Dashboard & STT**
  * sends telemetry JSON to Unity
  * receives operator commands and audio blobs from Unity
* **MOD-02 AI & Vision**
  * provides victim classification data that reaches Unity through telemetry
* **MOD-03 Acoustics & Navigation**
  * may provide acoustic beam data for map visualization through `MapManager_AcousticBeam.cs`

---

## Quick-Start Integration Example

```csharp
using UnityEngine;

public class RobotManager : MonoBehaviour
{
    private INetworkClient networkClient;
    private MapManager mapManager;
    private UIManager uiManager;

    void Start()
    {
        networkClient = new WebSocketClient();   // concrete implementation provided later
        mapManager = FindObjectOfType<MapManager>();
        uiManager = FindObjectOfType<UIManager>();

        networkClient.OnTelemetryReceived += UpdateDashboard;
        networkClient.Connect("ws://192.168.1.10:5000");
    }

    void UpdateDashboard(TelemetryData data)
    {
        mapManager.UpdateRobotPosition(data.posX, data.posY);
        mapManager.PlacePin(data.posX, data.posY, data.victimStatus);
        uiManager.UpdateHUD(data);
    }
}
```

---

## API Summary

### `DataContracts.cs`

| Type | Fields / Members | Description |
|---|---|---|
| `VictimStatus` | `NONE`, `STANDING`, `LYING`, `TRAPPED` | Victim severity enum used by Unity-side visualization |
| `TelemetryData` | `posX`, `posY`, `temperature`, `smokeDetected`, `victimStatus`, `priorityLevel`, `isStuck`, `acousticHit`, `acousticAngle` | Main telemetry packet deserialized from JSON |

### `INetworkClient.cs`

| Member | Return | Description |
|---|---|---|
| `event Action<TelemetryData> OnTelemetryReceived` | event | Fired when telemetry JSON is received and parsed |
| `Connect(string ipAddress)` | `void` | Connects to the Raspberry Pi WebSocket server |
| `Disconnect()` | `void` | Gracefully closes the connection |
| `SendOperatorCommand(string command)` | `void` | Sends a manual override command such as `"FORWARD"` or `"STOP"` |
| `SendAudioBlob(byte[] wavData)` | `void` | Sends recorded `.wav` bytes to MOD-04 |

### `AudioManager.cs`

| Member | Return | Description |
|---|---|---|
| `AudioManagerConstants.AUDIO_SAMPLE_RATE_HZ` | `int` | Audio sample rate constant (`16000`) |
| `AudioManagerConstants.AUDIO_MAX_RECORD_SECS` | `int` | Maximum recording duration (`10` seconds) |
| `AudioCaptureState` | enum | `Idle`, `Recording`, `Encoding`, `Sending` |
| `event Action<byte[]> OnAudioBlobReady` | event | Fired after audio encoding completes |
| `event Action<AudioCaptureState> OnCaptureStateChanged` | event | Fired whenever the capture state changes |
| `StartRecording()` | `void` | Starts microphone capture |
| `StopAndEncode()` | `void` | Stops capture, encodes audio to `.wav`, and prepares it for sending |
| `GetCaptureState()` | `AudioCaptureState` | Returns current audio pipeline state |

### `MapManager.cs`

| Member | Return | Description |
|---|---|---|
| `MapManagerConstants.MAP_PIN_HEIGHT` | `float` | Z-offset for map pins |
| `MapManagerConstants.PIN_PRIORITY_RED` | `int` | Priority constant for trapped victims |
| `MapManagerConstants.PIN_PRIORITY_YELLOW` | `int` | Priority constant for lying victims |
| `MapManagerConstants.PIN_PRIORITY_GREEN` | `int` | Priority constant for standing victims |
| `PlacePin(float posX, float posY, VictimStatus status)` | `void` | Places a color-coded victim pin on the map |
| `UpdateRobotPosition(float posX, float posY)` | `void` | Updates robot marker location on the configured map plane |
| `ClearAllPins()` | `void` | Clears all active victim pins |

### `UIManager.cs`

| Member | Return | Description |
|---|---|---|
| `UIManagerConstants.SMOKE_DETECTED_TEXT` | `string` | Warning label for smoke detection |
| `UIManagerConstants.SMOKE_CLEAR_TEXT` | `string` | Normal smoke label |
| `UIManagerConstants.TEMPERATURE_UNIT` | `string` | Display suffix for temperature |
| `UpdateTemperature(float temperature)` | `void` | Updates HUD temperature field |
| `UpdateSmokeStatus(bool smokeDetected)` | `void` | Updates smoke indicator |
| `UpdateVictimStatus(VictimStatus status)` | `void` | Updates victim status label |
| `UpdateHUD(TelemetryData data)` | `void` | Refreshes all HUD fields from one telemetry packet |
| `UpdatePTTState(AudioCaptureState state)` | `void` | Displays Push-to-Talk status |

### `FileNetworkClient.cs` (Mocking)

| Member | Return | Description |
|---|---|---|
| `FileNetworkClient(float updateInterval)` | constructor | Initializes the mock client with a specific update rate |
| `Connect(string filePath)` | `void` | Loads a JSON file from `StreamingAssets` and starts playback |

---

## Demo Mode: Mock Telemetry from Files

To facilitate testing without a live backend, MOD-05 supports reading telemetry from a JSON file:

1.  In the Unity Inspector for `RobotManager`, check **Use Mock File Data**.
2.  Provide a **Mock File Name** (e.g., `mock_telemetry.json`).
3.  Ensure the file exists in `Assets/StreamingAssets/`.
4.  The file format should be:
    ```json
    {
      "packets": [
        { "posX": 1.0, "posY": 2.0, "temperature": 25.0, ... },
        ...
      ]
    }
    ```

---

## File Responsibilities

### `DataContracts.cs`

Defines the Unity-side telemetry data model and victim severity enum.

### `INetworkClient.cs`

Defines the communication contract between Unity and the Raspberry Pi server.

### `AudioManager.cs`

Defines the Push-to-Talk audio capture pipeline and audio state events.

### `MapManager.cs`

Defines map-side visualization responsibilities such as robot position updates and victim pin placement.

### `UIManager.cs`

Defines HUD-side visualization responsibilities such as temperature, smoke, victim status, and PTT state display.

---

## Known Risks & Open Questions

### Risks

* **Telemetry JSON mismatch**
  * If MOD-04 changes field names or enum/string formatting, Unity deserialization may silently fail or display wrong data.

* **Audio upload latency**
  * Large audio blobs or unstable Wi-Fi may delay STT interactions and reduce operator responsiveness.

* **Map scaling uncertainty**
  * Exact coordinate scaling between robot-space telemetry and Unity world-space still needs calibration.

### Open Questions

* Should `victimStatus` remain an enum in Unity, or should future dashboard payloads switch to string-based status values?
* What final `unitsPerGridCell`, `mapOrigin`, and coordinate-plane settings should be used after physical map calibration?

---

## Known Limitations

* Student IDs are still incomplete in some documentation blocks.

---

## Version History

* **v0.3 (2026-04-19)** — Added `FileNetworkClient` and mock JSON support for demo purposes.
* **v0.2 (2026-03-29)** — README updated to match the current Unity-side files exactly (`DataContracts.cs`, `INetworkClient.cs`, `AudioManager.cs`, `MapManager.cs`, `UIManager.cs`)
* **v0.1 (2026-03-29)** — Initial draft of MOD-05 Unity Digital Twin documentation
