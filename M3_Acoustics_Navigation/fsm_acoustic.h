#ifndef MOD03_FSM_ACOUSTIC_H
#define MOD03_FSM_ACOUSTIC_H

/**
 * @file    fsm_acoustic.h
 * @brief   MOD-03 FSM Acoustic Branching — state transition interface
 * @author  Tuana Melisa Aksoy [Öğrenci No Yaz]
 * @date    2026-03-28
 * @version 0.1
 *
 * Changelog:
 * v0.1 (2026-03-28) - Initial draft: FSM state enum and acoustic
 *                     transition function stubs defined.
 *
 * Consumed by: MOD-04 (fsm_update loop on Raspberry Pi)
 * Depends on:  acoustics_iir.h (Uğur) for acoustics_result_t,
 *              acoustic_homing.py (Evrim) for bridge signals
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants & Macros ---------------------------------------------------- */

#define FSM_ACOUSTIC_MIN_CONFIRMS    3     /**< Min consecutive A_Hit=1 before transition */
#define FSM_ACOUSTIC_TIMEOUT_MS   5000    /**< Max ms to wait for bearing confirmation */

/* -- Data Types ------------------------------------------------------------ */

/**
 * @brief FSM states relevant to acoustic-driven navigation.
 *        Aligned with the system FSM diagram in the Project Proposal Report.
 */
typedef enum {
    FSM_STATE_IDLE             = 0,
    FSM_STATE_SPIN_MAP         = 1,
    FSM_STATE_EXPLORE          = 2,
    FSM_STATE_ACOUSTIC_HOMING  = 3,
    FSM_STATE_APPROACH_TARGET  = 4,
    FSM_STATE_VICTIM_ANALYSIS  = 5,
    FSM_STATE_RTH              = 6
} fsm_state_t;

/**
 * @brief Acoustic event passed into the FSM update function.
 *        Fields A_Hit and A_Ang come from the MOD-01 UART telemetry packet.
 */
typedef struct {
    bool     a_hit;         /**< True if STM32 detected a distress call (A_Hit field) */
    float    a_ang;         /**< Bearing angle in degrees (-180.0 to +180.0) */
    uint32_t timestamp_ms;  /**< Time of event (ms since boot) */
} fsm_acoustic_event_t;

/**
 * @brief Result of an FSM acoustic update call.
 */
typedef struct {
    fsm_state_t new_state;         /**< FSM state after processing the event */
    bool        transition_fired;  /**< True if a state transition occurred */
    float       confirmed_bearing; /**< Valid bearing if transition_fired, else 0.0f */
} fsm_acoustic_result_t;

/* -- Public Functions ------------------------------------------------------ */

/**
 * @brief  Initialise the acoustic FSM branching module.
 *         Resets hit counter and sets initial state to FSM_STATE_IDLE.
 * @return true on success
 */
bool FSM_Acoustic_Init(void);

/**
 * @brief  Process one acoustic telemetry event inside the FSM update loop.
 *         Implements the EXPLORE → ACOUSTIC_HOMING guard condition.
 *         Must be called every FSM tick when current state is FSM_STATE_EXPLORE.
 * @param  current_state  Active FSM state at time of call
 * @param  event          Pointer to incoming acoustic event data
 * @param  result         Pointer to fsm_acoustic_result_t to be filled
 * @return true if result is valid
 */
bool FSM_Acoustic_Update(fsm_state_t current_state,
                          const fsm_acoustic_event_t *event,
                          fsm_acoustic_result_t *result);

/**
 * @brief  Reset the acoustic hit counter.
 *         Called when FSM re-enters EXPLORE after a false positive or timeout.
 */
void FSM_Acoustic_Reset(void);

/**
 * @brief  Query the current confirmed hit count.
 * @return Number of consecutive A_Hit=1 readings accumulated so far
 */
uint8_t FSM_Acoustic_GetHitCount(void);

#endif /* MOD03_FSM_ACOUSTIC_H */
