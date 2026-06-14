/**
 * @file    fsm_acoustic.h
 * @brief   MOD-03 — FSM Acoustic Branching & Mode Transition Interface
 * @author  Tuana Melisa Aksoy [Ogrenci No Yaz]
 * @date    2026-03-29
 * @version 0.3
 *
 * @details
 *     This header defines the Finite State Machine (FSM) states, acoustic
 *     event structures, and the branching logic interface that governs
 *     transitions triggered by acoustic detections.
 *
 *     Primary responsibility: When the robot is in EXPLORE mode and the
 *     STM32 reports consecutive acoustic hits (A_Hit=1) via UART, this
 *     module's FSM_Acoustic_Update() decides whether to interrupt
 *     exploration and transition to ACOUSTIC_HOMING.
 *
 *     Consumed by:
 *         - M4_MainFSM.py      (fsm_acoustic_update mirrors this header 1:1)
 *         - acoustic_homing.py  (Evrim) reads fsm_state_t for transition signals
 *
 *     Depends on:
 *         - uart_comm.h         (Ziya, MOD-01) — uart_telemetry_t.acoustic_angle
 *         - acoustics_iir.h     (Ugur, MOD-03) — acoustics_result_t bearing data
 *         - pwr_management.h    (Ziya, MOD-01) — PWR_MAX_OPERATION_MINS for RTH
 *
 *     Data flow:
 *         acoustics_iir.h Acoustic_ComputeBearing()
 *              -> acoustics_result_t { bearing_deg, hit_detected, timestamp_ms }
 *              -> uart_comm.h uart_telemetry_t.acoustic_angle  (over UART)
 *              -> M4 serial_read_telemetry()
 *              -> fsm_acoustic_event_t { a_hit, a_ang, timestamp_ms }
 *              -> FSM_Acoustic_Update()
 *              -> fsm_acoustic_result_t { new_state, transition_fired, confirmed_bearing }
 *
 * Changelog:
 *     v0.4 (2026-03-29) — Expanded FSM states from 7 to 11 to match new M4_MainFSM.py.
 *     v0.3 (2026-03-29) — Fixed FSM_Acoustic_Update() signature to match
 *                          M4_MainFSM.py (3 params, internal context).
 *                          Include guard aligned with acoustics_iir.h naming.
 *     v0.2 (2026-03-29) — Aligned with M4_MainFSM.py v2.0 enums and
 *                          acoustic_homing.py v0.1 data contracts.
 *                          FSM_ACOUSTIC_MIN_CONFIRMS set to 3 (matching
 *                          Evrim's HOMING_MIN_HIT_CONFIRMS).
 *     v0.1 (2026-03-25) — Initial architecture draft.
 */

#ifndef MOD03_FSM_ACOUSTIC_H
#define MOD03_FSM_ACOUSTIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>


/* ===================================================================
 *  CONSTANTS
 * =================================================================== */

/**
 * @brief Minimum consecutive acoustic hits required before FSM commits
 *        to EXPLORE -> ACOUSTIC_HOMING transition.
 *
 * Aligned with:
 *   acoustic_homing.py  HOMING_MIN_HIT_CONFIRMS = 3
 *   M4_MainFSM.py       fsm_acoustic_update() guard condition
 */
#define FSM_ACOUSTIC_MIN_CONFIRMS       3

/**
 * @brief Cooldown period (milliseconds) between two consecutive
 *        ACOUSTIC_HOMING transitions.
 *
 * Avoids rapid oscillation when echoes or reflections produce
 * multiple hits in quick succession after the first homing session.
 */
#define FSM_ACOUSTIC_COOLDOWN_MS        3000

/**
 * @brief Maximum time (milliseconds) the robot may stay in
 *        ACOUSTIC_HOMING before automatically falling back to EXPLORE.
 *
 * Prevents chasing phantom sounds indefinitely.
 */
#define FSM_ACOUSTIC_HOMING_TIMEOUT_MS  30000

/**
 * @brief Lower bound of valid bearing range (degrees).
 *
 * Aligned with acoustics_iir.h: ACOUSTICS_BEARING_MIN = -180.0f
 */
#define FSM_BEARING_MIN_DEG             (-180.0f)

/**
 * @brief Upper bound of valid bearing range (degrees).
 *
 * Aligned with acoustics_iir.h: ACOUSTICS_BEARING_MAX = 180.0f
 */
#define FSM_BEARING_MAX_DEG             (180.0f)

/**
 * @brief Bearing values within +/- DEAD_ZONE degrees of 0 mean
 *        "source is directly ahead" — no rotation issued, drive forward.
 *
 * Aligned with acoustic_homing.py: HOMING_BEARING_TOLERANCE_DEG = 10.0
 */
#define FSM_BEARING_DEAD_ZONE_DEG       10.0f


/* ===================================================================
 *  DATA TYPES — FSM States
 *  Aligned 1:1 with M4_MainFSM.py FSMState enum (7 states).
 *  Integer values are the wire format used by M5 Unity as well.
 * =================================================================== */

/**
 * @brief Enumeration of all top-level FSM states.
 *
 * Integer values match M4_MainFSM.py FSMState exactly:
 *   M4 IDLE=0, SPIN_MAP=1, EXPLORE=2, ACOUSTIC_HOMING=3,
 *   APPROACH_TARGET=4, VICTIM_ANALYSIS=5, RETURN_TO_HOME=6
 */
typedef enum {
    FSM_STATE_IDLE            = 0,  /**< Power-on, awaiting START command   */
    FSM_STATE_SPIN_MAP        = 1,  /**< Initial 360-degree ultrasonic sweep */
    FSM_STATE_EXPLORE         = 2,  /**< Frontier-based patrol               */
    FSM_STATE_ACOUSTIC_HOMING = 3,  /**< Rotating toward acoustic source     */
    FSM_STATE_APPROACH_TARGET = 4,  /**< Moving toward YOLO target           */
    FSM_STATE_VICTIM_ANALYSIS = 5,  /**< Running Edge VLM / CNN              */
    FSM_STATE_EVALUATE_VICTIM = 6,  /**< Status Evaluation                   */
    FSM_STATE_WAKEUP_PROTOCOL = 7,  /**< Execute Wake-up routine             */
    FSM_STATE_MANUAL_OVERRIDE = 8,  /**< Operator Override                   */
    FSM_STATE_BEACON_MODE     = 9,  /**< SOS Beacon / Power save             */
    FSM_STATE_RTH             = 10  /**< Return-to-Home                      */
} fsm_state_t;

/**
 * @brief Total number of FSM states.
 */
#define FSM_STATE_COUNT                 11


/* ===================================================================
 *  DATA TYPES — Acoustic Event (input to FSM_Acoustic_Update)
 *  Aligned 1:1 with M4_MainFSM.py FSMAcousticEvent dataclass.
 *
 *  Field mapping from upstream modules:
 *    acoustics_iir.h  acoustics_result_t   ->  uart_comm.h  uart_telemetry_t
 *    .hit_detected                          ->  (derived)
 *    .bearing_deg                           ->  .acoustic_angle
 *    .timestamp_ms                          ->  (derived)
 *
 *  M4 serial_read_telemetry() populates this struct from the above.
 * =================================================================== */

/**
 * @brief Acoustic event payload extracted from UART telemetry.
 */
typedef struct {
    bool     a_hit;         /**< True if STM32 detected a distress call
                                 Maps from: acoustics_result_t.hit_detected   */
    float    a_ang;         /**< Bearing angle in degrees (-180.0 to +180.0)
                                 Maps from: uart_telemetry_t.acoustic_angle   */
    uint32_t timestamp_ms;  /**< Time of event (ms since boot)
                                 Maps from: acoustics_result_t.timestamp_ms   */
} fsm_acoustic_event_t;


/* ===================================================================
 *  DATA TYPES — Acoustic Result (output of FSM_Acoustic_Update)
 *  Aligned 1:1 with M4_MainFSM.py FSMAcousticResult dataclass.
 * =================================================================== */

/**
 * @brief Result of processing one acoustic event through FSM branching.
 *
 * The caller (M4 fsm_update loop) inspects transition_fired to decide
 * whether to call fsm_transition(new_state).
 *
 * When transition_fired == true:
 *   - new_state contains the target state (FSM_STATE_ACOUSTIC_HOMING)
 *   - confirmed_bearing contains the validated bearing angle
 *   - M4 should call fsm_transition(result.new_state)
 *   - Evrim's bridge receives the bearing via process_acoustic_event()
 *   - Dicle's MapManager_AcousticBeam.ShowAcousticBeam() is invoked
 */
typedef struct {
    fsm_state_t new_state;          /**< FSM state after processing         */
    bool        transition_fired;   /**< True if a state transition occurred */
    float       confirmed_bearing;  /**< Valid bearing if fired, else 0.0    */
} fsm_acoustic_result_t;


/* ===================================================================
 *  PUBLIC FUNCTIONS
 *
 *  Context management: This module maintains an internal static
 *  context that tracks hit streaks, cooldown timers, and homing
 *  session duration across FSM ticks.  Callers do not need to
 *  manage context — only call FSM_Acoustic_Init() once at startup.
 *
 *  Function signatures match M4_MainFSM.py mappings exactly:
 *    FSM_Acoustic_Update  ->  fsm_acoustic_update(current_state, event)
 *    FSM_Acoustic_Init    ->  (called in M4 main() startup)
 * =================================================================== */

/**
 * @brief Initialise the acoustic branching sub-system.
 *
 * Resets the internal context: hit streak counter, cooldown timer,
 * and homing session timestamp.  Must be called once before the
 * main FSM loop begins.
 *
 * Called by M4_MainFSM.py main() during startup sequence.
 */
void FSM_Acoustic_Init(void);


/**
 * @brief Core acoustic branching function — called on every FSM tick.
 *
 * Maps to M4_MainFSM.py:
 *   fsm_acoustic_update(current_state: FSMState,
 *                        event: FSMAcousticEvent) -> FSMAcousticResult
 *
 * Decision flow:
 *
 *     1. If event->a_hit is false:
 *        a. Reset internal hit_streak to 0.
 *        b. Set result->transition_fired = false.
 *        c. Return true.
 *
 *     2. If event->a_hit is true:
 *        a. Validate event->a_ang is within
 *           [FSM_BEARING_MIN_DEG, FSM_BEARING_MAX_DEG].
 *           If invalid: result->transition_fired = false, return true.
 *        b. Increment internal hit_streak.
 *        c. If hit_streak < FSM_ACOUSTIC_MIN_CONFIRMS (3):
 *           result->transition_fired = false, return true
 *           (wait for more evidence).
 *        d. If cooldown is active AND elapsed time <
 *           FSM_ACOUSTIC_COOLDOWN_MS:
 *           result->transition_fired = false, return true.
 *        e. If current_state is FSM_STATE_EXPLORE or FSM_STATE_SPIN_MAP:
 *           - result->new_state = FSM_STATE_ACOUSTIC_HOMING
 *           - result->transition_fired = true
 *           - result->confirmed_bearing = event->a_ang
 *           - Reset hit_streak, record timestamps internally
 *        f. Otherwise:
 *           result->transition_fired = false (not interruptible).
 *
 * @param[in]  current_state  Active FSM state at time of call.
 * @param[in]  event          Incoming acoustic event from UART telemetry.
 * @param[out] result         Populated with transition decision.
 *
 * @retval true   Event was processed (check result->transition_fired).
 * @retval false  Invalid parameter (NULL pointer passed).
 */
bool FSM_Acoustic_Update(
    fsm_state_t                current_state,
    const fsm_acoustic_event_t *event,
    fsm_acoustic_result_t      *result
);


/**
 * @brief Check whether the current ACOUSTIC_HOMING session has
 *        exceeded FSM_ACOUSTIC_HOMING_TIMEOUT_MS.
 *
 * Should be called once per FSM tick while in FSM_STATE_ACOUSTIC_HOMING.
 * If true, M4 should transition back to FSM_STATE_EXPLORE and call
 * FSM_Acoustic_ResetStreak().
 *
 * @param[in] now_ms  Current time in milliseconds since boot.
 *
 * @retval true   Homing has timed out — fall back to EXPLORE.
 * @retval false  Homing is still within the allowed window.
 */
bool FSM_Acoustic_IsHomingTimedOut(uint32_t now_ms);


/**
 * @brief Quick predicate: should the current EXPLORE sweep be paused?
 *
 * Convenience function for Evrim's acoustic_homing.py bridge so it
 * can pre-emptively stop motor commands before FSM_Acoustic_Update()
 * formally changes state.
 *
 * Returns true when ALL conditions are met:
 *   - current_state is FSM_STATE_EXPLORE or FSM_STATE_SPIN_MAP
 *   - event->a_hit is true
 *   - Internal hit_streak >= FSM_ACOUSTIC_MIN_CONFIRMS
 *   - Cooldown has elapsed
 *
 * @param[in] current_state  Active FSM state.
 * @param[in] event          Latest acoustic event.
 *
 * @retval true   Exploration should be interrupted.
 * @retval false  Continue exploring.
 */
bool FSM_Acoustic_ShouldInterruptExplore(
    fsm_state_t                current_state,
    const fsm_acoustic_event_t *event
);


/**
 * @brief Reset the consecutive hit counter and cooldown state.
 *
 * Called when:
 *   - ACOUSTIC_HOMING times out and FSM returns to EXPLORE.
 *   - A false positive is identified.
 *   - Evrim's IAcousticHomingBridge.reset() is invoked.
 */
void FSM_Acoustic_ResetStreak(void);


/**
 * @brief Return a human-readable label for the given FSM state.
 *
 * Used by M4 WebSocket telemetry and M5 Unity Digital Twin for
 * status display and logging.
 *
 * @param[in] state  An fsm_state_t enum value.
 *
 * @return Pointer to a static string, e.g. "EXPLORE", "ACOUSTIC_HOMING".
 *         Returns "UNKNOWN" if state is out of range.
 */
const char* FSM_Acoustic_GetStateName(fsm_state_t state);


#ifdef __cplusplus
}
#endif

#endif /* MOD03_FSM_ACOUSTIC_H */