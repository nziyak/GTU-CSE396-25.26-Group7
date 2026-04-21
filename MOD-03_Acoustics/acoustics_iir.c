/**
 * @file    acoustics_iir.c
 * @brief   MOD-03 STM32 Acoustic Signal Processing — Implementation
 * @author  Uğur Anıl Güney
 * @date    2026-04-21
 * @version 0.1
 *
 * @details
 *     Implements the acoustic processing pipeline declared in acoustics_iir.h.
 *     Provides:
 *       - 4th-order IIR bandpass filter for motor noise attenuation
 *       - Bearing computation stub (pending hardware integration)
 *       - Spin-scan stub (pending MOD-01 motor integration)
 *       - Bearing-to-motor-command mapper using FSM dead-zone constants
 */

#include "acoustics_iir.h"
#include "fsm_acoustic.h"   /* FSM_BEARING_DEAD_ZONE_DEG */

#include <stddef.h>
#include <string.h>
#include <math.h>


/* ===================================================================
 *  acoustics_iir_filter_apply()
 *
 *  4th-order Direct-Form II Transposed IIR filter (two cascaded
 *  biquad sections) targeting the motor/gearbox noise band ~50-200 Hz
 *  at Fs = 8000 Hz.
 *
 *  NOTE: The coefficients below are design-time placeholders derived
 *  from a Butterworth high-pass prototype (fc = 250 Hz, Fs = 8000 Hz).
 *  They MUST be re-tuned after physical motor noise measurement on the
 *  actual hardware — record a noise floor sample at each PWM duty cycle
 *  and re-run the filter design (e.g. scipy.signal.iirdesign) to match
 *  the real noise spectrum before deployment.
 *
 *  Biquad section coefficients: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2]
 *                                             - a1*y[n-1] - a2*y[n-2]
 *  (a0 is normalised to 1.0)
 * =================================================================== */

/* Biquad section 1 — placeholder, tune after motor noise measurement */
#define IIR_B0_S1   0.8948f
#define IIR_B1_S1  (-1.7897f)
#define IIR_B2_S1   0.8948f
#define IIR_A1_S1  (-1.7786f)
#define IIR_A2_S1   0.8009f

/* Biquad section 2 — placeholder, tune after motor noise measurement */
#define IIR_B0_S2   0.8948f
#define IIR_B1_S2  (-1.7897f)
#define IIR_B2_S2   0.8948f
#define IIR_A1_S2  (-1.7572f)
#define IIR_A2_S2   0.6038f

acoustics_status_t acoustics_iir_filter_apply(const float *samples,
                                               uint16_t     length,
                                               float       *out)
{
    if ((samples == NULL) || (out == NULL) || (length == 0u)) {
        return ACOUSTICS_ERR_INIT;
    }

    /* Filter memory — persist across calls so edge samples are handled
     * correctly when processing streaming blocks.                        */
    static float x_prev1 = 0.0f;   /* x[n-1] section 1 input  */
    static float x_prev2 = 0.0f;   /* x[n-2] section 1 input  */
    static float y_prev1 = 0.0f;   /* y[n-1] section 1 output */
    static float y_prev2 = 0.0f;   /* y[n-2] section 1 output */

    /* Section-2 memory */
    static float x2_prev1 = 0.0f;
    static float x2_prev2 = 0.0f;
    static float y2_prev1 = 0.0f;
    static float y2_prev2 = 0.0f;

    for (uint16_t i = 0u; i < length; i++) {
        float x = samples[i];

        /* Biquad section 1 */
        float s1 = IIR_B0_S1 * x
                 + IIR_B1_S1 * x_prev1
                 + IIR_B2_S1 * x_prev2
                 - IIR_A1_S1 * y_prev1
                 - IIR_A2_S1 * y_prev2;

        x_prev2 = x_prev1;
        x_prev1 = x;
        y_prev2 = y_prev1;
        y_prev1 = s1;

        /* Biquad section 2 — cascaded after section 1 */
        float s2 = IIR_B0_S2 * s1
                 + IIR_B1_S2 * x2_prev1
                 + IIR_B2_S2 * x2_prev2
                 - IIR_A1_S2 * y2_prev1
                 - IIR_A2_S2 * y2_prev2;

        x2_prev2 = x2_prev1;
        x2_prev1 = s1;
        y2_prev2 = y2_prev1;
        y2_prev1 = s2;

        out[i] = s2;
    }

    return ACOUSTICS_OK;
}


/* ===================================================================
 *  acoustics_compute_bearing()
 *
 *  TODO: Implement phase-difference triangulation across 3x MAX4466
 *  microphones to compute acoustic bearing.  Each microphone pair
 *  yields a time-delay-of-arrival (TDOA) estimate via cross-correlation
 *  of the filtered buffers; the three TDOA values are combined using
 *  the known microphone geometry to resolve a 2D bearing angle in the
 *  range [-180, +180] degrees.  Full implementation is deferred until
 *  hardware integration is complete (mic positions on PCB must be
 *  measured, and impulse response calibration must be performed in an
 *  anechoic environment).
 * =================================================================== */
acoustics_status_t acoustics_compute_bearing(const float        *mic_buffers[],
                                              uint16_t            length,
                                              acoustics_result_t *out)
{
    if ((mic_buffers == NULL) || (out == NULL) || (length == 0u)) {
        return ACOUSTICS_ERR_INIT;
    }

    /* Safe defaults — caller can inspect hit_detected before using bearing */
    out->bearing_deg  = 0.0f;
    out->hit_detected = false;
    out->timestamp_ms = 0u;

    return ACOUSTICS_ERR_NO_HIT;
}


/* ===================================================================
 *  acoustics_spinscan_execute()
 *
 *  TODO: Implement 360-degree rotation with ultrasonic pinging to
 *  populate the 2D occupancy grid.  Requires MOD-01 motor integration:
 *  the robot must issue incremental rotate commands via uart_comm.h
 *  (Ziya, MOD-01), wait for the STM32 acknowledgement at each angular
 *  step (e.g. 5-degree increments), read the HC-SR04 ultrasonic range,
 *  and map the measured distance to the grid cell corresponding to that
 *  bearing.  Implementation is deferred until the MOD-01 UART motor
 *  command protocol is finalised and testable on the physical chassis.
 * =================================================================== */
acoustics_status_t acoustics_spinscan_execute(acoustics_grid_t *grid)
{
    if (grid == NULL) {
        return ACOUSTICS_ERR_INIT;
    }

    /* Zero-initialise grid to a known safe state */
    memset(grid->cells, 0, sizeof(grid->cells));
    grid->size          = ACOUSTICS_GRID_SIZE;
    grid->resolution_cm = 0.0f;  /* will be set after MOD-01 integration */

    return ACOUSTICS_OK;
}


/* ===================================================================
 *  acoustics_homing_navigate()
 *
 *  Maps a bearing angle to a discrete motor command using the dead-zone
 *  constant from fsm_acoustic.h (FSM_BEARING_DEAD_ZONE_DEG = ±10°).
 *
 *    |bearing| <= dead zone  →  NAV_FORWARD  (source is straight ahead)
 *    bearing  <  -dead zone  →  NAV_LEFT     (source is to the left)
 *    bearing  >  +dead zone  →  NAV_RIGHT    (source is to the right)
 * =================================================================== */
acoustics_status_t acoustics_homing_navigate(float                bearing_deg,
                                              acoustics_nav_cmd_t *cmd)
{
    if (cmd == NULL) {
        return ACOUSTICS_ERR_INIT;
    }

    if (fabsf(bearing_deg) <= FSM_BEARING_DEAD_ZONE_DEG) {
        *cmd = ACOUSTICS_NAV_FORWARD;
    } else if (bearing_deg < -FSM_BEARING_DEAD_ZONE_DEG) {
        *cmd = ACOUSTICS_NAV_LEFT;
    } else {
        *cmd = ACOUSTICS_NAV_RIGHT;
    }

    return ACOUSTICS_OK;
}
