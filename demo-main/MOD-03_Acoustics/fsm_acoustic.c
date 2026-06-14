/**
 * @file    fsm_acoustic.c
 * @brief   MOD-03 — FSM Acoustic Branching & Mode Transition Implementation
 * @author  Uğur Anıl Güney
 * @date    2026-04-21
 * @version 0.1
 *
 * @details
 *     Implements the acoustic finite state machine described in fsm_acoustic.h.
 *     Maintains an internal static context (hit_streak, cooldown timer,
 *     homing session timer) so callers need no external state management.
 */

#include "fsm_acoustic.h"

#include <stddef.h>   /* NULL */


/* ===================================================================
 *  INTERNAL CONTEXT
 * =================================================================== */

/**
 * @brief Module-internal persistent state across FSM ticks.
 */
typedef struct {
    uint32_t hit_streak;         /**< Consecutive A_Hit=1 events counted      */
    uint32_t cooldown_start_ms;  /**< Timestamp when last homing was triggered */
    uint32_t homing_start_ms;    /**< Timestamp when ACOUSTIC_HOMING began    */
    bool     cooldown_active;    /**< True when post-homing cooldown is running */
} fsm_acoustic_ctx_t;

static fsm_acoustic_ctx_t ctx;


/* ===================================================================
 *  INTERNAL HELPERS
 * =================================================================== */

/**
 * @brief Return true if the cooldown period has fully elapsed given
 *        the supplied current timestamp.
 */
static bool cooldown_elapsed(uint32_t now_ms)
{
    if (!ctx.cooldown_active) {
        return true;
    }
    return (now_ms - ctx.cooldown_start_ms) >= FSM_ACOUSTIC_COOLDOWN_MS;
}

/**
 * @brief Return true if current_state is one that can be interrupted
 *        by an acoustic detection.
 */
static bool is_interruptible_state(fsm_state_t state)
{
    return (state == FSM_STATE_EXPLORE) || (state == FSM_STATE_SPIN_MAP);
}


/* ===================================================================
 *  PUBLIC FUNCTION IMPLEMENTATIONS
 * =================================================================== */

void FSM_Acoustic_Init(void)
{
    ctx.hit_streak        = 0;
    ctx.cooldown_start_ms = 0;
    ctx.homing_start_ms   = 0;
    ctx.cooldown_active   = false;
}

bool FSM_Acoustic_Update(
    fsm_state_t                current_state,
    const fsm_acoustic_event_t *event,
    fsm_acoustic_result_t      *result)
{
    /* NULL-guard — invalid parameters */
    if ((event == NULL) || (result == NULL)) {
        return false;
    }

    /* Initialise output to "no transition" */
    result->new_state         = current_state;
    result->transition_fired  = false;
    result->confirmed_bearing = 0.0f;

    /* ---- Step 1: no hit detected ---------------------------------------- */
    if (!event->a_hit) {
        ctx.hit_streak = 0;
        return true;
    }

    /* ---- Step 2a: validate bearing range --------------------------------- */
    if ((event->a_ang < FSM_BEARING_MIN_DEG) ||
        (event->a_ang > FSM_BEARING_MAX_DEG)) {
        result->transition_fired = false;
        return true;
    }

    /* ---- Step 2b: accumulate streak -------------------------------------- */
    ctx.hit_streak++;

    /* ---- Step 2c: not enough confirmations yet ---------------------------- */
    if (ctx.hit_streak < FSM_ACOUSTIC_MIN_CONFIRMS) {
        result->transition_fired = false;
        return true;
    }

    /* ---- Step 2d: cooldown guard ----------------------------------------- */
    if (ctx.cooldown_active &&
        ((event->timestamp_ms - ctx.cooldown_start_ms) < FSM_ACOUSTIC_COOLDOWN_MS)) {
        result->transition_fired = false;
        return true;
    }

    /* ---- Step 2e: trigger ACOUSTIC_HOMING for interruptible states ------- */
    if (is_interruptible_state(current_state)) {
        result->new_state         = FSM_STATE_ACOUSTIC_HOMING;
        result->transition_fired  = true;
        result->confirmed_bearing = event->a_ang;

        /* Record internal timestamps and reset streak */
        ctx.homing_start_ms   = event->timestamp_ms;
        ctx.cooldown_start_ms = event->timestamp_ms;
        ctx.cooldown_active   = true;
        ctx.hit_streak        = 0;
        return true;
    }

    /* ---- Step 2f: non-interruptible state -------------------------------- */
    result->transition_fired = false;
    return true;
}

bool FSM_Acoustic_IsHomingTimedOut(uint32_t now_ms)
{
    return (now_ms - ctx.homing_start_ms) >= FSM_ACOUSTIC_HOMING_TIMEOUT_MS;
}

bool FSM_Acoustic_ShouldInterruptExplore(
    fsm_state_t                current_state,
    const fsm_acoustic_event_t *event)
{
    if (event == NULL) {
        return false;
    }

    /* All four conditions must be satisfied simultaneously */
    return is_interruptible_state(current_state)
        && event->a_hit
        && (ctx.hit_streak >= FSM_ACOUSTIC_MIN_CONFIRMS)
        && cooldown_elapsed(event->timestamp_ms);
}

void FSM_Acoustic_ResetStreak(void)
{
    ctx.hit_streak      = 0;
    ctx.cooldown_active = false;
}

const char *FSM_Acoustic_GetStateName(fsm_state_t state)
{
    switch (state) {
        case FSM_STATE_IDLE:            return "IDLE";
        case FSM_STATE_SPIN_MAP:        return "SPIN_MAP";
        case FSM_STATE_EXPLORE:         return "EXPLORE";
        case FSM_STATE_ACOUSTIC_HOMING: return "ACOUSTIC_HOMING";
        case FSM_STATE_APPROACH_TARGET: return "APPROACH_TARGET";
        case FSM_STATE_VICTIM_ANALYSIS: return "VICTIM_ANALYSIS";
        case FSM_STATE_EVALUATE_VICTIM: return "EVALUATE_VICTIM";
        case FSM_STATE_WAKEUP_PROTOCOL: return "WAKEUP_PROTOCOL";
        case FSM_STATE_MANUAL_OVERRIDE: return "MANUAL_OVERRIDE";
        case FSM_STATE_BEACON_MODE:     return "BEACON_MODE";
        case FSM_STATE_RTH:             return "RTH";
        default:                        return "UNKNOWN";
    }
}
