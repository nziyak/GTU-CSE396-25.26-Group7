/// <summary>
/// File: DataContracts.cs
/// Brief: Serializable data models for JSON parsing between Flask and Unity
/// Author: Ziya 210104004027
/// Date: 2026-03-27
/// Version: 0.1
/// 
/// Changelog:
/// v0.1 - Defined VictimStatus enum and TelemetryData struct.
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
/// </summary>
[Serializable]
public struct TelemetryData
{
    /// <summary> Robot's X position on the 2D grid </summary>
    public float posX;
    
    /// <summary> Robot's Y position on the 2D grid </summary>
    public float posY;
    
    /// <summary> Current temperature in Celsius </summary>
    public float temperature;
    
    /// <summary> True if smoke threshold is exceeded </summary>
    public bool smokeDetected;
    
    /// <summary> The AI-classified status of the victim in view </summary>
    public VictimStatus victimStatus;
    
    /// <summary> Priority level for the Unity Map Pin (1=Red, 2=Yellow, 3=Green) </summary>
    public int priorityLevel;

    /// <summary> True if a distress call is detected by the acoustic sensor array </summary>
    public bool acousticHit;     

    /// <summary> Bearing angle to the acoustic source in degrees (-180.0 to +180.0) </summary>
    public float acousticAngle;
}