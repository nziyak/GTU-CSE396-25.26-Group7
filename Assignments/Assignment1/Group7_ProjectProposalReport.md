
--- Page 1 ---
    Autonomous First Responder Fire & Rescue Robot
      with Virtual Payload Delivery and Acoustic
                     Localization

CSE 396 Computer Engineering Project - Project Proposal
                        Report

                Instructor: Salih Sarp
                       Group: 7
                Term: 2025-2026 Spring





1 Introduction

1.1 Problem
In high-risk fire and disaster environments, it is critically dangerous for human rescue teams
to conduct initial area reconnaissance. While existing rescue robots can navigate these areas,
physical payload delivery significantly increases the robot’s hardware cost, power consumption,
mechanical failure rate, and overall weight.
   Furthermore, relying on cloud-based AI for scene analysis is impossible in disaster zones
where internet infrastructure is destroyed. Therefore, the specific problem this project solves
is developing an autonomous, Edge-AI-driven "First Responder" reconnaissance
robot. This robot enters the disaster zone before firefighters to map the area and continu-
ously locates victims utilizing Tri-Modal Sensor Fusion (Acoustics, Vision, and Environmental
Data). It classifies victim severity (trapped, lying down, standing) and provides initial support
(two-way audio, wake-up protocols, augmented status reports, and SOS beaconing) utilizing
strict on-device computing (Edge AI) and a zero-risk Dual Powerbank Architecture.

2 Literature Review

Traditional search and rescue robots heavily rely on teleoperation and deterministic navigation.
Recent advancements in Edge AI have introduced Vision-Language Models (VLMs), object
detection models (YOLO), and lightweight Speech-to-Text (STT) models directly onto single-
board computers, enabling semantic understanding without internet access.
   For victim localization, Acoustic Source Localization (ASL) using microphone arrays is a
proven method for finding conscious victims. Initially, this project considered utilizing a Thermal
Sensor Matrix to combat the Tyndall effect in dense smoke where RGB cameras fail. However,
due to prohibitive hardware costs and the inherent safety risks of replicating real fire/smoke
conditions for physical testing, thermal imaging was deprecated.
   Instead, this project addresses the localization challenge by fusing Acoustic Homing with
an Adaptive Edge-AI Pipeline. To ensure real-time performance on edge constraints, the
system will empirically compare the efficacy of a quantized VLM against a custom-trained Con-
volutional Neural Network (CNN) and a rule-based YOLO-Pose engine, selecting the optimal
architecture based on latency and accuracy.


    1


--- Page 2 ---
3   Methods

3.1   Data

The system will strictly operate on real-time sensory data acquired from the environment; no
external APIs will be queried.

      • Visual Data: Live RGB frames captured via a USB/Pi Camera. Used for target detection
      and severity analysis.
         • Continuous Acoustic Data: Analog audio signals continuously monitored by an array
      of directional microphones (MAX4466), sampled via the STM32 ADC.
        • Voice Command Data: .wav audio files recorded by the operator on the Web Dashboard
      for local STT processing.
            • Environmental & Spatial Data: Real-time metrics from MQ-2 (Smoke), DHT22 (Tem-
      perature), MPU6050 IMU (Movement/Stuck Detection), and HC-SR04 (Ultrasonic).

3.2   Methodology

3.2.1 System Architecture Overview

The project utilizes a Hybrid Edge Architecture. A Raspberry Pi 5 handles all AI and
networking, while an STM32 manages real-time hardware reflexes. To eliminate the fire risks
associated with Li-Po batteries in disaster robotics, the system is powered by a Dual PD
Powerbank Setup, utilizing a 12V Type-C Decoy trigger for the mobility chassis.










  2


--- Page 3 ---
    +-------------------------------------------------------------------------+
    |                             OPERATOR INTERFACES                               |
    |                                                                               |
    |    +------------------+     +-------------------------+       +------------+  |
    |    |   Unity 2D/3D    |<-->|   Local Web Dashboard      |     | Microphone |  |
    |    |  Digital Twin      |   |  (Flask + WebSockets)   |<-->| (Operator     |  |
    |    |  (Visualizer)      |   |  (Manual Override)        |                  |  |  | Voice)
    |    +--------+---------+     +-----------+-------------+       +------------+  |
    +-----------+--------------------------+----------------------------------+
              | Wi-Fi (Telemetry, Status Reports, Audio Files & Video)
              v                          v
    +-------------------------------------------------------------------------+
    |                           ROBOT CHASSIS (EDGE AI)                             |
    |                                                                               |
| +-------------------------------------------------------------------+ |
    |    |                      RASPBERRY PI 5 (8GB - The Brain)                 |  |
    |    |    +--------------+ +------------------+ +---------------------+      |  |
    |    |    | YOLO / Pose |    | VLM or Custom CNN|   | Vosk/Whisper (STT) |   |  |
    |    |    +------+-------+ +------+-----------+ +-------------+-------+      |  |
    |    |    +----------- Resource Manager & FSM -----------+                   |  |
| +---------------------------+---------------------------------------+ |
    |                                | UART                                         |
    |                                v                                              |
| +-----------------+ +---------------+ +----------------------------+ |
    |    | USB/Pi Camera    | | USB Speaker   |     | STM32 (Blue Pill) (Reflex) |  |
    |    +-----------------+ | (PAM8403 Amp) |                                   |  | | - Motor PWM Control
    |    +-----------------+ +---------------+                                   |  | | - IMU Stuck Detection
    |    | MPU6050 IMU      | +---------------+     | - Software IIR Audio Filter|  |
    |    +-----------------+ | Flashlight/LED|                                   |  | | - DHT22 & MQ-2 Reading
| +-----------------+ +---------------+ +-------------+--------------+ |
    |    | Microphones x4   | +---------------+    |                                |
    |    +-----------------+ | Env. Sensors   |     +-------------v--------------+  |
    |                         +---------------+     | Motors & Ultrasonic Sensors|  |
    |                                               +----------------------------+  |
    +-------------------------------------------------------------------------+

    3.2.2     Tri-Modal Search & Dynamic Priority Flow

    The baseline behavior of the robot is to explore the entire arena. Target detection triggers
    the Finite State Machine (FSM) via three modalities: Vision (YOLO), Acoustic (Microphones),
    or Environmental (Sensors).    Detected locations are added to a Priority Queue, temporarily
    hijacking the exploration path to assess the victim.










                                     3


--- Page 4 ---
[WEB/UNITY INTERFACE] <==(Continuous Video, Map & Telemetry Stream)==> [VOICE COMMAND]
                                                                                                  |
                    [IDLE] --(Start)--> [SPIN_MAP] --(Initial Map Done)--> [EXPLORE / PATROL] <---|
(360 Initial      (Constructs                       ^        |                                    |
Sensor Sweep)      2D Grid)                         |        |                                    |
                                                    |        v                                    |
(Interrupt: Acoustic Hit added to Priority Queue)   | (Continuous Vision)
+--------------------------------------------------------+   |                                    |
                    [APPROACH PRIORITY TARGET] <------------------------(Person Detected via YOLO)|
|                                                            |                                    |
v                                                            v                                    |
[VICTIM SCENE ANALYSIS] <----------------------------------------+                                |
(Execute Edge VLM / CNN)                                                                          |
|                                                                                                 |
v         (RESOURCE AWARE INTERRUPT) <-----------------------------+
[EVALUATE VICTIM]   (Pause Vision Models -> Run STT -> Execute Action -> Resume)
(Trapped / Lying / Standing)
|
v
[EXECUTE WAKE-UP PROTOCOL & DELIVER VIRTUAL PAYLOAD]
(Activate Buzzer/Speaker/Flashlight, Assign Mask/Extinguisher/Water)
|
                       +----------------(Loop: Continue until arena is fully swept)---------------+

3.2.3 Dynamic Resource Management & Failsafes

           To prevent the Pi 5 from overheating and to guarantee safe operations, we implement core
optimizations:

                      • Dynamic FPS Management: During standard "Explore" mode, the camera operates
               at a low framerate (e.g., 5 FPS). Upon detecting a trigger, the FSM boosts computing
      power to 100% for victim assessment.
                     • Time-Based Return-to-Home (RTH): Because modern PD Powerbanks regulate their
      output to a constant voltage until total depletion, traditional ADC voltage-reading failsafes
             are ineffective. Therefore, the FSM implements a strict time-based RTH protocol (e.g.,
            automatically returning to the starting point after 60 minutes of operation) to prevent
      stranding.
              • Dead Man’s Switch: If the WebSocket connection drops, the FSM triggers an immediate
      RTH, reversing the last 30 seconds of telemetry to regain the Wi-Fi signal.
                • Power-Saving Beacon Mode: Once a trapped/unconscious victim is secured, the robot
                powers down heavy AI pipelines and motors, sustaining only the SOS light beacon and
      two-way audio.

3.2.4 Target Prioritization & Wake-up Protocol

    Once a target is approached, the AI pipeline evaluates the frame. If multiple people are found,
the FSM sorts targets in a Priority Queue:

1. Trapped / Lying Down (High Priority): The robot approaches immediately. It exe-
                cutes the Wake-Up Protocol (flashing SOS lights, buzzer alarm, and two-way operator
                  speaker) to assess consciousness. It locks the map location with a RED/YELLOW pin
      on Unity.



      4


--- Page 5 ---
              2. Standing (Lower Priority): The robot evaluates the environment (Smoke/Heat) and
      assigns a virtual payload (e.g., Smoke Mask, Extinguisher), marking the map with a
      GREEN pin.
      3. Proximity Tie-Breaker: If multiple targets of the same priority level exist in a single
      frame, the system navigates to the closest target first (estimated via YOLO bounding box
      area or ultrasonic depth).
                4. Augmented Status Report: The robot transmits a comprehensive JSON report con-
      taining: Exact coordinates, MQ-2 Oxygen/Smoke estimate, DHT22 Temperature, and
      Victim Status.

3.2.5     Techniques

• Software-Based    Digital IIR Filtering: To cancel continuous motor noise from the
      acoustic data, we evaluated hardware (RC) filters versus software digital filters. We se-
      lected a Software-based Digital IIR (Infinite Impulse Response) Filter executed on the
      STM32. Unlike static hardware filters, the IIR filter dynamically attenuates unwanted
      frequencies with minimal computational overhead (simple multiplications and additions)
      after ADC sampling.
• Edge STT Processing & Preemptive Scheduling: Utilizing offline models (Vosk/Whisper.cpp)
      via a preemptive resource scheduling algorithm. When a voice command is received, heavy
      vision models are strictly paused to prevent Out-Of-Memory errors, prioritizing the STT
      pipeline.
                • Tri-Modal Sensor Fusion: Fusing Visual cues (YOLO), Acoustic Bearings (Mic Ar-
      rays), and Spatial/Environmental constraints (Ultrasonic + IMU + DHT22) to guarantee
      navigation.
              • Adaptive Edge Vision Pipeline: Comparing three methodologies to find the optimal
      Edge AI balance: a quantized VLM (Moondream2), a Custom Transfer-Learned CNN,
      and a rule-based YOLO-Pose engine.
              • Systematic Exploration (Spin-Scan): An initial 360-degree rotation (SPIN_MAP) to
      populate a preliminary 2D occupancy grid using ultrasonic sensors, followed by a frontier-
      based sweeping algorithm.










      5


--- Page 6 ---
   3.3 Hardware Requirements (BOM)

   Component      Model/Description                 Purpose
   Brain (SBC)    Raspberry Pi 5 (8GB) + Active     Edge AI Pipeline, Web Server,
                  Cooler                            FSM.
   Storage        64GB Micro-SD Card (A2 Class)     High IOPS storage for OS, Swap
                                                    Memory, and AI Models.
   Reflex (MCU)   STM32F103C8T6 (Blue Pill)         Motor control, IIR Audio Filtering,
                                                    ADC reading.
   Vision         Pi Camera Module V3               Frame capturing for AI.
   Acoustics      3x MAX4466 Mics & PAM8403         Sound localization & Operator
                  Amp + Speaker                     two-way audio.
   Sensors        HC-SR04,   MQ-2,         DHT22,   Navigation,          Smoke, Temp, and
                  MPU6050 (IMU)                     Stuck Detection.
   Movement       4WD Chassis + L298N Driver        Mobility platform.
   Power (Pi 5)   10000mAh   PD         Powerbank   Stable, risk-free power dedicated to
                  (5V/3A)                           the Pi 5.
Power (Motors) 10000mAh PD Powerbank + 12V Regulated 12V output for DC mo-
                  Type-C Decoy                      tors, eliminating Li-Po fire hazards.

   3.4 Module Structure and Task Distribution (8-Person Team)


Person Mod 1: Emb Mod 2: AI & Mod 3: Au- Mod 4: Web Mod 5: Unity
            & HW         Vision      dio & Nav    & STT
   Gabil    Secondary    Primary
   Ziya     Primary                                                       Secondary
   Dicle                 Secondary   Secondary                            Primary
   Ömer     Secondary                             Primary
   Uğur                  Secondary   Primary      Secondary
   Tuana                 Secondary   Secondary    Primary
   Evrim                 Primary     Secondary                            Secondary
   Fatma                 Primary                  Secondary

   3.4.1    Module Responsibilities

                         • MODULE 1: Embedded & Hardware (2 People): Physical assembly and real-time
           hardware reflexes via the STM32 microcontroller. Driving DC motors using PWM, polling
           environmental sensors, and reading the MPU6050 IMU for stuck detection. Managing the
           Dual Powerbank (12V Decoy) distribution and UART communication.
                         • MODULE 2: AI & Vision Pipeline (2 People): Managing the visual perception
           and classification layer on the Raspberry Pi 5. Integrating the Pi Camera and running
           continuous YOLO inference with dynamic FPS management. Executing the Edge VLM
           (or Custom CNN) for victim severity assessment. Implementing the pause/resume logic
           during STT interrupts.
                        • MODULE 3: Acoustics & Navigation (2 People): Sound processing, directional
           hearing, and autonomous movement logic. Implementing the Software-based Digital IIR
           Filter on the STM32 to calculate distress call bearings. Developing the initial Spin-Scan
           mapping algorithm, AND-logic obstacle avoidance, and the Acoustic Homing protocol.
                           • MODULE 4: Web Dashboard & STT (1-2 People): Operator control interface,
           communication bridging, and Voice Command processing. Developing the Flask/WebSockets

                                     6


--- Page 7 ---
    backend to stream live video and telemetry. Integrating the offline STT model (Vosk/Whisper.cpp).
    Structuring the Augmented Status Reports (JSON).
  • MODULE 5: Unity Digital Twin (1-2 People): 3D visualization and situational
    awareness for the operator. Building the Unity C# dashboard to reflect the robot’s real-
    time position. Implementing the Priority Queue mapping system to drop color-coded pins
    based on victim status, and designing the Push-to-Talk UI.

4   Evaluation & Deliverables

4.1 Evaluation
                1. AI Latency & Model Selection: The chosen vision architecture (VLM vs. CNN vs.
    YOLO-Pose) must accurately classify target severity and resolve in under 5 seconds entirely
    on the edge.
          2. Acoustic Reliability: The Digital IIR Filter must attenuate motor noise effectively
    enough to detect a human distress call.
                  3. Failsafe Executions: The IMU stuck detection, Time-Based RTH, and Wi-Fi RTH
    protocols must trigger correctly under adverse conditions.
            4. Digital Twin Synchronization: Unity must reflect the augmented status reports and
    color-coded priority pins with minimal latency.

4.2 Deliverables (Quarterly Expectations)
Must Have (Minimum Viable Product):

  • A functional physical robot driven by STM32, utilizing AND-Logic obstacle avoidance.
        • Raspberry Pi 5 successfully running an offline Vision AI (YOLO + Classifier) to detect
    and assess humans.
  • A Web Dashboard streaming the camera feed and rudimentary telemetry.

  Should Have (Target Success):

            • Successful deployment of the Augmented Status Report and Wake-Up Protocols (Buzzer
    + Speaker) for Trapped/Lying victims.
                 • Dynamic FPS management and Resource-Aware STT pausing for optimal RAM/Thermal
    performance.
   • Acoustic Source Localization (SSL) with Digital IIR Filtering to redirect the robot towards
    sound.
         • Unity 2D/3D Digital Twin tracking the robot’s movement and dropping color-coded pins.

  Nice to Have (Bonus / Optional):

  • Full integration of the Time-Based RTH and Dead Man’s Switch protocols.
  • Power-Saving Beacon Mode for extended battery life during rescue waits.

4.3 Time Plan & Syllabus Milestones

Note: Hardware procurement has been shifted to overlap with initial software architecture devel-
opment to ensure continuous progress without hardware bottlenecks.








    7


--- Page 8 ---
    Week           Phase / Milestone                Deliverable
    W 1-3          Project Ideation & Structuring   Concept finalized, team modules as-
                                                    signed, BOM created.
    W 4 (15/3)     Project Proposals                Document Submission.             Hard-
                                                    ware orders placed.
    W 5            PC-Based Dev & Hardware Setup    Parts arrive. Pi OS / STM32 setup.
        Unity UI design and AI testing start.
W 6 (24/3) Module Requirements (Head- API contracts finalized. C/C++
                   ers)                             .h files and Python interface struc-
                                                    tures pushed to Git.
    W 7            Sub-system Integration           STM32 reading sensors and driving
                                                    motors. Pi testing Digital IIR Filter.
    W 8 (7/4)      Module documentation             Technical documentation of algo-
                                                    rithms and wiring diagrams com-
                                                    plete.
    W 9            Acoustic & Failsafe Testing      Robot prioritizes targets (handling
                                                    proximity  tie-breakers),     executes
                                                    wake-up protocols,         and handles
                                                    RTH/STT interrupts.
    W 10 (21/4) Module demonstrations               Individual unit tests presented to the
                                                    instructor.
    W 11-13        Full Integration & UI Polish     WebSockets fully integrated; physi-
                                                    cal arena end-to-end testing and edge
                                                    case debugging.
    W 14 (19/5) Final Project Demostration          Video presentation, user manual, and
                                                    website.        The last grade will be
                                                    mainly based on the final demonstra-
                                                    tion on week 14.


    4.4     Risk Analysis and Mitigation

    Risk                       Prob/Impact Mitigation Strategy
    Pi 5 Overheating / OOM High / High         Use Active Cooler.     Dynamic FPS throt-
                                               tling.   Pausing Vision models during STT
                                               inference.
    Acoustic Noise / Echo      High / Med      Implement Software-based Digital IIR Fil-
                                               ter in STM32 to attenuate motor frequen-
                                               cies.
    Stranding (Power Loss)     Low / High      Utilize Dual PD Powerbanks with a Time-
                                               Based Return-to-Home (RTH) protocol
                                               (e.g., 60-minute limit).
    VLM Too Slow               High / High     Evaluate and fallback to Custom CNN or
                                               YOLO-Pose rule-based engine.










                               8


--- Page 9 ---
5 References

  [1] Instructor: Salih Sarp

  [2] CSE 396 Computer Engineering Project Syllabus, Gebze Technical University.

  [3] Ultralytics. (2023). YOLOv8 Documentation for Edge Devices.

  [4] STMicroelectronics. (2021). STM32F103 Reference Manual (ADC and DMA Configura-
      tion for Acoustic Sampling).

  [5] Valin, J. M., et al. (2007). Robust Sound Source Localization Using a Microphone Array on
      a Mobile Robot. IEEE/RSJ International Conference on Intelligent Robots and Systems.

  [6] Unity Technologies. (2023). Unity Networking and UI Toolkit Documentation.










    9