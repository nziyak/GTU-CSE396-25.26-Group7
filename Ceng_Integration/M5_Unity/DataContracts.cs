/// <summary>
/// File: DataContracts.cs
/// Brief: Serializable data models for JSON parsing between Flask and Unity
/// Author: Ziya 210104004027
/// Date: 2026-03-27
/// Version: 0.2 (Ceng_Integration)
///
/// Changelog:
/// v0.1 - Defined VictimStatus enum and TelemetryData struct.
/// v0.2 (Ceng_Integration) - Docx compliance revision:
///   [R9]  posX→pos_x, posY→pos_y, temperature→temp,
///         smokeDetected→smoke, victimStatus→victim_status,
///         priorityLevel→priority    (docx §2A JSON schema)
///   [R10] CommandPacket class added (docx §2B)
/// </summary>

using System;

/// <summary>
/// Severity levels of the detected human target.
/// </summary>
public enum VictimStatus
{
    NONE = 0,
    STANDING = 1,
    LYING = 2,
    TRAPPED = 3
}

/// <summary>
/// The primary Augmented Status Report sent from the robot to Unity.
///
/// [R9] JSON field names were changed.
/// REASON: Docx §2A JSON schema is defined as:
///   {"pos_x":12.5, "pos_y":8.0, "temp":24.5, "smoke":true,
///    "victim_status":"TRAPPED", "priority":1}
///
/// Unity JsonUtility.FromJson maps C# field names directly to JSON keys.
/// Original field names (posX, posY, temperature, smokeDetected,
/// victimStatus, priorityLevel) did not match the JSON keys
/// → deserialization would yield all default values (0/false/null).
/// This would cause a critical runtime error.
///
/// Change table:
///   posX           → pos_x          (docx: "pos_x")
///   posY           → pos_y          (docx: "pos_y")
///   temperature    → temp           (docx: "temp")
///   smokeDetected  → smoke          (docx: "smoke")
///   victimStatus   → victim_status  (docx: "victim_status", as string)
///   priorityLevel  → priority       (docx: "priority")
/// </summary>
[Serializable]
public struct TelemetryData
{
    /// <summary> Robot's X position on the 2D grid (docx: pos_x) </summary>
    public float pos_x;

    /// <summary> Robot's Y position on the 2D grid (docx: pos_y) </summary>
    public float pos_y;

    /// <summary> Current temperature in Celsius (docx: temp) </summary>
    public float temp;

    /// <summary> True if smoke threshold is exceeded (docx: smoke) </summary>
    public bool smoke;

    /// <summary> Victim status as string for JSON compat (docx: victim_status).
    /// "TRAPPED", "LYING", "STANDING" or "NONE".
    /// Original: was VictimStatus enum. Reason for change: M4 Python side
    /// writes victim_status:"TRAPPED" as a string in JSON;
    /// JsonUtility cannot parse an enum directly from a string. </summary>
    public string victim_status;

    /// <summary> Priority level: 1=Red/Trapped, 2=Yellow/Lying, 3=Green/Standing (docx: priority) </summary>
    public int priority;
}

/// <summary>
/// [R10] Operator command packet from Unity to Pi 5.
///
/// NEW CLASS — did not exist in the original.
/// Docx §2B rule: "Drive JSON: {"override": true, "cmd": "FORWARD"}"
/// When the M5 Unity operator drives via keyboard, this JSON is sent to M4.
/// When override=true, M4 FSM suspends autonomous mode (MANUAL_OVERRIDE).
/// </summary>
[Serializable]
public class CommandPacket
{
    /// <summary> True → FSM suspends autonomous mode (manual override) </summary>
    public bool @override;

    /// <summary> Drive command: "FORWARD", "BACKWARD", "LEFT", "RIGHT", "STOP" </summary>
    public string cmd;
}
