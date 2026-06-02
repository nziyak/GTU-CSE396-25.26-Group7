/// <summary>
/// File: DataContracts.cs
/// Brief: Shared MOD-05 telemetry contracts for Unity Digital Twin systems.
/// </summary>

using System;

/// <summary>
/// AI classification result for the victim currently detected by the robot.
/// </summary>
public enum VictimStatus
{
    None = 0,
    NONE = 0,
    Standing = 1,
    STANDING = 1,
    Lying = 2,
    LYING = 2,
    Trapped = 3,
    TRAPPED = 3
}

public enum AcousticBeamStyle
{
    Default = 0,
    Pulse = 1,
    Gradient = 2,
    Solid = 3,
    DirectionArrow = 4,
    RadarSweep = 5
}

public enum NetworkConnectionState
{
    Disconnected = 0,
    Connecting = 1,
    Connected = 2
}

/// <summary>
/// Primary telemetry packet consumed by the MOD-05 event-driven Digital Twin.
/// </summary>
[Serializable]
public struct TelemetryData
{
    /// <summary>Robot X coordinate on the 2D rescue grid.</summary>
    public float posX;

    /// <summary>Robot Y coordinate on the 2D rescue grid.</summary>
    public float posY;

    /// <summary>Ambient temperature in Celsius.</summary>
    public float temperature;

    /// <summary>True when the smoke sensor reports a dangerous threshold.</summary>
    public bool smokeDetected;

    /// <summary>Computer-vision victim classification result.</summary>
    public VictimStatus victimStatus;

    /// <summary>True when MOD-01 reports an IMU-based stuck condition.</summary>
    public bool isStuck;

    /// <summary>Rescue priority from MOD-02: 0=None, 1=Trapped, 2=Lying, 3=Standing.</summary>
    public int priorityLevel;

    /// <summary>True after MOD-03 confirms an acoustic hit.</summary>
    public bool acousticHit;

    /// <summary>Direction of the detected acoustic distress call in degrees.</summary>
    public float acousticAngle;

    /// <summary>Latest Unity-to-MOD-04 ping estimate in milliseconds.</summary>
    public float networkLatencyMs;
}

[Serializable]
public struct UartDebugData
{
    public int distFront;
    public int distBack;
    public int mic;
    public float yaw;
    public float pitch;
    public float roll;
    public float temp;
    public float hum;
    public bool connected;
}

[Serializable]
public struct SpeechCommandParsedData
{
    public string rawText;
    public string intent;
    public float confidence;
}

[Serializable]
public class AcousticBeamData
{
    public float bearingDeg;
    public bool hitDetected;
    public float posX;
    public float posY;
    public uint timestampMs;
}
