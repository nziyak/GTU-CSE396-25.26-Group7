/**
 * @file    acoustics_iir.c
 * @brief   MOD-03 STM32 Acoustic Signal Processing — Implementation
 * @author  Uğur Anıl Güney
 * @date    2026-04-21
 * @version 0.2
 *
 * @details
 *     Implements the acoustic processing pipeline declared in acoustics_iir.h.
 *     Provides:
 *       - 4th-order IIR bandpass filter for motor noise attenuation
 *       - Bearing computation via TDOA cross-correlation (this version)
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
 *  acoustics_compute_bearing()  — TDOA cross-correlation
 *
 *  Estimates the bearing to an acoustic source from the 3 MAX4466
 *  microphone buffers using Time-Difference-Of-Arrival (TDOA):
 *
 *    1. For each microphone PAIR, the relative time delay is found by
 *       locating the peak of the (normalised) cross-correlation between
 *       the two filtered buffers. A parabolic fit around the integer
 *       peak gives sub-sample resolution — this matters a lot at
 *       Fs = 8 kHz where the inter-mic delay is only a couple of
 *       samples for a small array.
 *
 *    2. Under a far-field plane-wave model, the delay for a pair maps
 *       linearly to the source direction unit-vector u:
 *
 *          (p_j - p_i) . u  =  c * tau_ij           (one scalar eqn / pair)
 *
 *       where p_i are the mic positions (metres), c the speed of sound,
 *       and tau_ij the measured TDOA. Three pairs give an over-
 *       determined 2x2 least-squares system, solved here in closed form
 *       via the normal equations.
 *
 *    3. bearing = atan2(u_y, u_x), reported in degrees [-180, +180].
 *
 *  Confidence = mean normalised cross-correlation peak across the three
 *  pairs (a coherence proxy). A diffuse/quiet field gives low coherence
 *  and is rejected by ACOUSTICS_HIT_THRESHOLD.
 *
 *  ---------------------------------------------------------------------
 *  HARDWARE-DEPENDENT CONSTANTS (must be set/verified after assembly):
 *    - Microphone XY positions on the PCB (ACOUSTICS_MICx_*).
 *      The defaults below are an equilateral triangle, circum-radius
 *      ACOUSTICS_MIC_RADIUS_M, with mic 0 pointing to the robot's
 *      forward (+Y) axis. Measure the real layout and update these.
 *    - Speed of sound (temperature dependent).
 *    - The final SIGN/ZERO of the bearing may need flipping/offsetting
 *      to match how MOD-01/Unity interpret 0deg = "straight ahead".
 *      Calibrate with a source at a known angle and adjust
 *      ACOUSTICS_BEARING_OFFSET_DEG / ACOUSTICS_BEARING_SIGN.
 * =================================================================== */

/* --- Geometry & physics (override with -D or here after measurement) --- */
#ifndef ACOUSTICS_SPEED_OF_SOUND_MS
#define ACOUSTICS_SPEED_OF_SOUND_MS   343.0f   /* ~20 C dry air */
#endif

#ifndef ACOUSTICS_MIC_RADIUS_M
#define ACOUSTICS_MIC_RADIUS_M        0.05f    /* circum-radius, metres */
#endif

/* Equilateral triangle: mic0 forward (+Y), mics 1/2 at the rear corners.
 * R*sqrt(3)/2 = 0.8660254*R,  R/2 = 0.5*R                                 */
#ifndef ACOUSTICS_MIC0_X
#define ACOUSTICS_MIC0_X  ( 0.0f)
#define ACOUSTICS_MIC0_Y  ( 1.0f      * ACOUSTICS_MIC_RADIUS_M)
#define ACOUSTICS_MIC1_X  (-0.8660254f* ACOUSTICS_MIC_RADIUS_M)
#define ACOUSTICS_MIC1_Y  (-0.5f      * ACOUSTICS_MIC_RADIUS_M)
#define ACOUSTICS_MIC2_X  ( 0.8660254f* ACOUSTICS_MIC_RADIUS_M)
#define ACOUSTICS_MIC2_Y  (-0.5f      * ACOUSTICS_MIC_RADIUS_M)
#endif

/* Post-calibration bearing trim (see header note above). */
#ifndef ACOUSTICS_BEARING_OFFSET_DEG
#define ACOUSTICS_BEARING_OFFSET_DEG  0.0f
#endif
#ifndef ACOUSTICS_BEARING_SIGN
#define ACOUSTICS_BEARING_SIGN        1.0f     /* set to -1.0f to mirror */
#endif

/* On-target timestamp source. Map to HAL_GetTick() in the firmware build,
 * e.g.  -DACOUSTICS_GET_TICK_MS=HAL_GetTick  (or define here).            */
#ifndef ACOUSTICS_GET_TICK_MS
#define ACOUSTICS_GET_TICK_MS()  (0u)
#endif

#define ACOUSTICS_PI_F           3.14159265358979f
#define ACOUSTICS_XCORR_EPS      1.0e-12f

/* Find the sub-sample lag (in samples) that best aligns buffer b to a,
 * over the search window [-max_lag, +max_lag], and report the normalised
 * correlation at the peak.
 *
 * Returns the fractional lag k that maximises  r(k) = SUM a[i]*b[i-k].
 * Convention: r(k) peaks at k = -D when  b[n] ~= a[n-D]  (b delayed by D),
 * so the caller converts TDOA via  tau_ij = -lag / Fs.                    */
static float xcorr_peak_lag(const float *a, const float *b,
                            uint16_t length, int max_lag,
                            float *out_ncc)
{
    int   best_k   = 0;
    float best_ncc = -2.0f;
    float ncc_lo   = 0.0f;   /* normalised corr at best_k - 1 */
    float ncc_hi   = 0.0f;   /* normalised corr at best_k + 1 */
    float ncc_at[3] = {0.0f, 0.0f, 0.0f};

    /* First pass: integer-lag search for the peak. */
    for (int k = -max_lag; k <= max_lag; k++) {
        int i_start = (k > 0) ? k : 0;
        int i_end   = (k < 0) ? ((int)length + k) : (int)length;

        float num = 0.0f, ea = 0.0f, eb = 0.0f;
        for (int i = i_start; i < i_end; i++) {
            float av = a[i];
            float bv = b[i - k];
            num += av * bv;
            ea  += av * av;
            eb  += bv * bv;
        }

        float denom = sqrtf(ea * eb) + ACOUSTICS_XCORR_EPS;
        float ncc   = num / denom;

        if (ncc > best_ncc) {
            best_ncc = ncc;
            best_k   = k;
        }
    }

    /* Second pass: recompute the three normalised values around the peak
     * for a parabolic sub-sample refinement (skip if peak is at an edge). */
    if (best_k > -max_lag && best_k < max_lag) {
        for (int idx = 0; idx < 3; idx++) {
            int k = best_k - 1 + idx;
            int i_start = (k > 0) ? k : 0;
            int i_end   = (k < 0) ? ((int)length + k) : (int)length;

            float num = 0.0f, ea = 0.0f, eb = 0.0f;
            for (int i = i_start; i < i_end; i++) {
                float av = a[i];
                float bv = b[i - k];
                num += av * bv;
                ea  += av * av;
                eb  += bv * bv;
            }
            float denom = sqrtf(ea * eb) + ACOUSTICS_XCORR_EPS;
            ncc_at[idx] = num / denom;
        }
        ncc_lo = ncc_at[0];
        ncc_hi = ncc_at[2];
        best_ncc = ncc_at[1];
    }

    /* Parabolic interpolation: offset of the true peak from best_k. */
    float frac = 0.0f;
    float curv = (ncc_lo - 2.0f * best_ncc + ncc_hi);
    if (fabsf(curv) > ACOUSTICS_XCORR_EPS) {
        frac = 0.5f * (ncc_lo - ncc_hi) / curv;
        if (frac > 1.0f)  frac = 1.0f;
        if (frac < -1.0f) frac = -1.0f;
    }

    if (out_ncc != NULL) {
        *out_ncc = best_ncc;
    }
    return (float)best_k + frac;
}

acoustics_status_t acoustics_compute_bearing(const float        *mic_buffers[],
                                              uint16_t            length,
                                              acoustics_result_t *out)
{
    if ((mic_buffers == NULL) || (out == NULL) || (length == 0u)) {
        return ACOUSTICS_ERR_INIT;
    }
    for (int m = 0; m < ACOUSTICS_MIC_COUNT; m++) {
        if (mic_buffers[m] == NULL) {
            return ACOUSTICS_ERR_INIT;
        }
    }

    /* Safe defaults — caller can inspect hit_detected before using bearing */
    out->bearing_deg  = 0.0f;
    out->hit_detected = false;
    out->timestamp_ms = ACOUSTICS_GET_TICK_MS();

    /* --- Microphone positions (metres) --- */
    const float px[ACOUSTICS_MIC_COUNT] = {
        ACOUSTICS_MIC0_X, ACOUSTICS_MIC1_X, ACOUSTICS_MIC2_X
    };
    const float py[ACOUSTICS_MIC_COUNT] = {
        ACOUSTICS_MIC0_Y, ACOUSTICS_MIC1_Y, ACOUSTICS_MIC2_Y
    };

    /* The three unique pairs for 3 mics. */
    const int pi[3] = {0, 0, 1};
    const int pj[3] = {1, 2, 2};

    /* Bound the lag search by the largest possible inter-mic delay. */
    float d_max = 0.0f;
    for (int p = 0; p < 3; p++) {
        float dx = px[pj[p]] - px[pi[p]];
        float dy = py[pj[p]] - py[pi[p]];
        float d  = sqrtf(dx * dx + dy * dy);
        if (d > d_max) d_max = d;
    }
    int max_lag = (int)ceilf((d_max / ACOUSTICS_SPEED_OF_SOUND_MS)
                             * (float)ACOUSTICS_SAMPLE_RATE_HZ) + 1;
    if (max_lag < 1) max_lag = 1;
    if (max_lag > (int)length - 1) max_lag = (int)length - 1;
    if (max_lag < 1) {
        return ACOUSTICS_ERR_NO_HIT;   /* buffer too short to localise */
    }

    /* --- Per-pair TDOA + build the 2x2 normal-equation accumulators --- */
    float c   = ACOUSTICS_SPEED_OF_SOUND_MS;
    float fs  = (float)ACOUSTICS_SAMPLE_RATE_HZ;

    float Sxx = 0.0f, Sxy = 0.0f, Syy = 0.0f;   /* R^T R  */
    float Sxo = 0.0f, Syo = 0.0f;               /* R^T o  */
    float ncc_sum = 0.0f;

    for (int p = 0; p < 3; p++) {
        float ncc = 0.0f;
        float lag = xcorr_peak_lag(mic_buffers[pi[p]], mic_buffers[pj[p]],
                                   length, max_lag, &ncc);
        ncc_sum += ncc;

        /* tau_ij = -(lag)/Fs ;  observation o = (p_j - p_i).u = c*tau... */
        float tau = -(lag) / fs;
        float rx  = px[pj[p]] - px[pi[p]];
        float ry  = py[pj[p]] - py[pi[p]];
        float o   = c * tau;

        Sxx += rx * rx;
        Sxy += rx * ry;
        Syy += ry * ry;
        Sxo += rx * o;
        Syo += ry * o;
    }

    float confidence = ncc_sum / 3.0f;
    if (confidence < 0.0f)  confidence = 0.0f;
    if (confidence > 1.0f)  confidence = 1.0f;

    /* --- Solve the 2x2 normal equations for u = (ux, uy) --- */
    float det = (Sxx * Syy) - (Sxy * Sxy);
    if (fabsf(det) < ACOUSTICS_XCORR_EPS) {
        /* Degenerate geometry (collinear mics) — cannot resolve bearing. */
        return ACOUSTICS_ERR_NO_HIT;
    }
    float ux = ( Syy * Sxo - Sxy * Syo) / det;
    float uy = (-Sxy * Sxo + Sxx * Syo) / det;

    float bearing = atan2f(uy, ux) * (180.0f / ACOUSTICS_PI_F);

    /* Apply calibration trim and wrap into [-180, +180]. */
    bearing = (ACOUSTICS_BEARING_SIGN * bearing) + ACOUSTICS_BEARING_OFFSET_DEG;
    while (bearing >  180.0f) bearing -= 360.0f;
    while (bearing < -180.0f) bearing += 360.0f;

    out->bearing_deg  = bearing;
    out->hit_detected = (confidence >= ACOUSTICS_HIT_THRESHOLD);

    return out->hit_detected ? ACOUSTICS_OK : ACOUSTICS_ERR_NO_HIT;
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