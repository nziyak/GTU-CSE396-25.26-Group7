
--- Page 1 ---
  Gebze Technical University

Computer Engineering Department



CSE 396 Computer Engineering Project

            ASSIGNMENT 2
  Module Interface & Header Design




    Autonomous First Responder Fire & Rescue Robot

with Virtual Payload Delivery and Acoustic Localization







    Submission Date: 30 March 2026
    Instructor:    Salih Sarp
    Group:        7





    Team Members & Assigned Modules


    Name                   Student ID     Assigned Module(s)
    Nuri Ziya Kırtepe      210104004027 MOD-01, MOD-05
    Ömer Nacar             210104004814 MOD-01, MOD-04
    Gabil Rahimli          230104004902   MOD-01, MOD-02
    Evrim Doğa Solmaz      230104004042   MOD-02, MOD-03, MOD-05
    Dicle Çoban            220104004088   MOD-02, MOD-03, MOD-05
    Uğur Anıl Güney        210104004011   MOD-02, MOD-03, MOD-04
    Tuana Melisa Aksoy     230104004903   MOD-02, MOD-03, MOD-04
    Fatma Öztürk           230104004152   MOD-02, MOD-04


--- Page 2 ---
1 Module Structure and Task Distribution (8-Person Team)


Person                Mod          1:   Mod 2: AI     Mod 3: Au-    Mod      4:   Mod     5:
                      Emb          &    & Vision      dio & Nav     Web      &    Unity
                      HW                                            STT
Gabil Rahimli         Secondary         Primary
Ziya                  Primary                                                     Secondary
Dicle Çoban                             Secondary     Secondary                   Primary
Ömer                  Secondary                                     Primary
Uğur Anıl Güney                         Secondary     Primary       Secondary
Tuana Melisa Aksoy                      Secondary     Secondary     Primary
Evrim Doğa Solmaz                       Primary       Secondary                   Secondary
Fatma Öztürk                            Primary                     Secondary

2 Module-Student Matching Table


Module ID Module Name         Responsible Student(s)      Brief Role
MOD-01 Embedded & HW          Ziya, Ömer, Gabil           STM32 firmware:               motor
                                                          control, environmental sens-
                                                          ing, stuck detection, power
                                                          management,                and UART
                                                          bridge to Pi 5.
MOD-02 AI & Vision Pipeline   Fatma, Gabil, Evrim, Tu-    Camera handling,              human
                              ana, Uğur, Dicle            detection,         victim    sever-
                                                          ity analysis,           and runtime
                                                          pause/resume support on
                                                          Pi 5.
MOD-03 Audio & Navigation     Uğur, Evrim, Tuana, Dicle   IIR filtering, bearing com-
                                                          putation,              FSM acoustic
                                                          branching, and Unity beam
                                                          visualization calculations.
MOD-04 Web Dashboard & STT Ömer, Tuana, Uğur, Fatma       Flask/WebSocket               dash-
                                                          board      bridge,          teleme-
                                                          try/video streaming, man-
                                                          ual override routing,           and
                                                          offline STT on Pi 5.
MOD-05 Unity Digital Twin     Dicle, Ziya, Evrim          Operator dashboard:             Map
                                                          visualization, HUD, Push-
                                                          to-Talk audio, and network
                                                          client.


3   System / Module Overview

3.1 Overall Architecture

The project is a multi-module search and rescue robot system built on three main computational
layers: an STM32 microcontroller for low-level hardware control, a Raspberry Pi 5 for high-level
autonomy and AI inference, and a Unity-based operator dashboard.







                                        1


--- Page 3 ---
    [WEB/UNITY INTERFACE (MOD-05)] <--(JSON Telemetry & Video)--> [WEB DASHBOARD & STT (MOD-04)]
       |                                      |
  (PTT Audio)               (Python Function Calls)
       |                                      |
       v                                      v
    [VISION PIPELINE (MOD-02)] <--(Pause/Resume Interrupts)--> [FSM & NAVIGATION (MOD-03)]
       |                                      |
(Camera Frame)              (NavCommand / UART)
       |                                      |
        +--------------------------------------------------------------+
                        | (115200 Baud UART)
                      v
                  [EMBEDDED REFLEXES (MOD-01)]
                      |
+---------------------+---------------------+
    |                 |              |
  (PWM)             (ADC)          (I2C)
DC Motors        Microphones    MPU6050 IMU

    3.2 Platform Summary


    Category             Details
    MCU                  STM32F103C8T6 (Blue Pill)
    SBC                  Raspberry Pi 5 (8 GB RAM) + Official Active Cooler
    Camera               Pi Camera Module V3
    Microphones          3x MAX4466 electret microphone amplifier modules
    Ultrasonic Sensors   HC-SR04 (front, back, left, right)
    Sensors              MPU-6050 (IMU), DHT22 (Temperature), MQ-2 (Smoke)
    Motor Driver         L298N H-Bridge
    UART                 115200 baud, 8N1
    Web Dashboard        Flask + WebSocket based dashboard communication
    STT Backend          Offline Vosk or Whisper.cpp style speech-to-text pipeline
    Unity Side           Network client, HUD, map visualization, operator control panel


    4   Inter-Module Communication Description

    4.1 MOD-01 → Upper Layers: UART Telemetry


    Field              From To              Type          Dir.         Frequency
    uart_telemetry_t   MOD-01 Pi-side FSM   UART string   01 → Upper   50 Hz (Every tick)

    Representative wire format:
    T:<temp>|S:<smoke>|ST:<stuck>|A_Ang:<acoustic_angle>|Y:<yaw>|UF:<front>|UB:<back>|...

    4.2 MOD-02 ↔ MOD-04: Vision Interface Calls

    Function                From  To        Type          Notes
    get_latest_target()     MOD-04 MOD-02 Python Call Returns TargetData or None.
 pause_vision_pipeline() MOD-04 MOD-02 Python Call Called before STT to free RAM.
 resume_vision_pipeline() MOD-04 MOD-02 Python Call Called after STT concludes.


                                            2


--- Page 4 ---
   Target Data schema:
   TargetData(pos_x, pos_y, distance_cm, severity, confidence)

   4.3 MOD-04 ↔ MOD-05: Dashboard Telemetry & Commands

    Function / Payload     From  To          Type     Notes
broadcast_telemetry() MOD-04 MOD-05 WebSocket Sends AugmentedStatusReport.
stream_video_frame() MOD-04 MOD-05 WebSocket Streams compressed JPEG frames.
    Operator Command       MOD-05 MOD-04 JSON         Receives manual override payload.
    process_audio_blob()   MOD-05 MOD-04 Byte[]       Converts incoming .wav bytes.

   Augmented Status Report Schema (JSON):
   AugmentedStatusReport(
       pos_x=12.5, pos_y=8.0,
       temperature=25.1, smoke_detected=False,
       victim_status="TRAPPED", is_stuck=False,
       priority_level=1, acoustic_hit=True, acoustic_angle=-45.0
   )
          Voice Command Schema (Processed internally in MOD-04):
   VoiceCommandData(
       raw_text="return home", intent="RETURN_HOME", confidence=0.91
   )

   5   Header File Summary


    Header / Interface File          Module      Lang Key Types Exposed
    uart_comm.h                      MOD-01       C       uart_direction_t, uart_telemetry_t
    environment_sensors.h            MOD-01       C       env_sensor_type_t,
                                                          environment_data_t
    motor_control.h                  MOD-01       C       motor_status_t, motor_direction_t
    stuck_detection.h                MOD-01       C       stuck_status_t,
                                                          stuck_detection_result_t
    pwr_management.h                 MOD-01       C       pwr_status_t
    ai_vision_interface.py           MOD-02        Py     TargetData, IVisionPipeline
    camera_internal.py               MOD-02        Py     CameraFrame, CameraInternal
    human_detector_internal.py       MOD-02        Py     CameraBBox, HumanDetection
    victim_analyzer_internal.py      MOD-02        Py     VictimAnalyzerInternal
    fsm_acoustic.h                   MOD-03       C       fsm_state_t, fsm_acoustic_result_t
    acoustics_iir.h                  MOD-03       C       acoustics_result_t,
                                                          acoustics_nav_cmd_t
    acoustic_homing.py               MOD-03        Py     AcousticTelemetry, NavCommand
    MapManager_AcousticBeam.cs       MOD-03/05     C#     AcousticBeamData, AcousticBeamStyle
    comms_dashboard_interface.py     MOD-04        Py     AugmentedStatusReport,
                                                          IWebDashboard
    stt_engine_interface.py          MOD-04        Py     VoiceCommandData, ISTTEngine
    DataContracts.cs                 MOD-05        C#     VictimStatus, TelemetryData
    INetworkClient.cs                MOD-05        C#     INetworkClient interface
    AudioManager.cs                  MOD-05        C#     AudioCaptureState, IAudioManager
    MapManager.cs                    MOD-05        C#     MapManagerConstants, MapManager
    UIManager.cs                     MOD-05        C#     UIManagerConstants, UIManager


                                     3


--- Page 5 ---
    6 Integration Change Log


    Code File / Change                                           Reason
    R1    uart_comm.h: Defined UART telemetry & cmd interface.   Base embedded communication
                                                                 contract.
    R2    environment_sensors.h: DHT22 and MQ-2 abstraction.     Separates sensing from other
                                                                 MCU tasks.
    R3    motor_control.h: L298N interface and PWM API.          Encapsulates robot motion con-
                                                                 trol.
    R4    stuck_detection.h: IMU stuck detection contract.       Supports autonomous recovery
                                                                 behavior.
    R5    ai_vision_interface.py: Public MOD-02 interface.       External   access to     target,
                                                                 pause/resume.
    R6    camera_internal.py: Camera and FPS management.         Separates  hardware      control
                                                                 from AI.
    R7    human_detector_internal.py: Detection helpers.         Modularizes YOLO detection
                                                                 logic.
    R8    victim_analyzer_internal.py: Severity helper.          Separates VLM analysis from
                                                                 detection.
    R9    fsm_acoustic.h: FSM acoustic behavior interface.       Transitions for acoustic naviga-
                                                                 tion logic.
    R10   acoustic_homing.py: Telemetry/nav Python bridge.       Connects acoustics to higher-
        level FSM.
R11 comms_dashboard_interface.py: Flask/WebSocket API. Matches MOD-04 comms role
        in root.
    R12   stt_engine_interface.py: Offline STT contract.         Matches    MOD-04        speech-
                                                                 processing role.
    R13   UIManager.cs, MapManager.cs: Operator HUD layers.      Completes Unity-side monitor-
                                                                 ing.


    7 Known Risks and Open Issues

                       • OOM Risks During STT Integration: Running YOLO, VLM, and Vosk/Whisper
      simultaneously on the Pi 5 will severely stress RAM. While pause_vision_pipeline() is
      defined, transition latency requires empirical benchmarking.
      • Mechanical Trim Calibration: Lacking wheel encoders, mapping relies on dead-reckoning.
      Differences in DC motors may cause drift. The motor_set_trim() constants are pending
      arena tests.
          • Unity Coordinate Scaling: The exact scaling factor between the Pi 5’s Grid and the
      Unity World-Space (GridToWorldPosition) is unconfirmed.
               • Python Interfaces vs. Headers: MOD-02 and MOD-04 expose Python ABC interfaces
      instead of C headers due to the framework requirements (Flask, PyTorch).
         • Placeholder IDs: Some student IDs remain placeholders in module-level documentation
      and need finalization before presentation.
              • WebSocket Constants: Exact WebSocket event names in MOD-04 are still marked as
      TODO in the repository README.

    8 Conclusion

      This report formally summarizes the module interfaces, public headers, and communication
    contracts of the Group 7 Search & Rescue Robot project. The resulting design robustly cap-
        tures the division of embedded (MOD-01), AI (MOD-02), acoustic (MOD-03), dashboard/STT


          4


--- Page 6 ---
(MOD-04), and Unity (MOD-05) responsibilities, satisfying all requirements of Assignment 2
and preventing integration bottlenecks in subsequent phases.










5