#ifndef MOD03_ACOUSTICS_IIR_H
#define MOD03_ACOUSTICS_IIR_H

/**
 * @file    acoustics_iir.h
 * @brief   MOD-03 STM32 Acoustic Processing & Navigation — public interface
 * @author  Uğur Anıl Güney [Öğrenci No Yaz]
 * @date    2026-03-28
 * @version 0.2
 *
 * Changelog:
 * v0.2 (2026-03-29) - Aligned acoustics_nav_cmd_t with M1 UART standard ('W', 'S', 'A', 'D', 'Q').
 *                     Renamed functions to use 'acoustics_' prefix to match header assignments.
 * v0.1 (2026-03-28) - Initial draft: IIR_Filter_Apply, Acoustic_ComputeBearing,
 *                     SpinScan_Execute, Homing_Navigate stubs defined.
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants & Macros ---------------------------------------------------- */

#define ACOUSTICS_MIC_COUNT          3       /**< Number of MAX4466 microphones */
#define ACOUSTICS_SAMPLE_RATE_HZ     8000    /**< ADC sampling rate in Hz */
#define ACOUSTICS_IIR_ORDER          4       /**< IIR filter order */
#define ACOUSTICS_BEARING_MIN       -180.0f  /**< Minimum bearing angle (degrees) */
#define ACOUSTICS_BEARING_MAX        180.0f  /**< Maximum bearing angle (degrees) */
#define ACOUSTICS_HIT_THRESHOLD      0.6f    /**< Minimum confidence to flag A_Hit */
#define ACOUSTICS_GRID_SIZE          64      /**< 2D occupancy grid dimension (NxN) */

/* -- Data Types ------------------------------------------------------------ */

/**
 * @brief Status codes returned by acoustics functions.
 */
typedef enum {
    ACOUSTICS_OK            =  0,
    ACOUSTICS_ERR_INIT      = -1,
    ACOUSTICS_ERR_TIMEOUT   = -2,
    ACOUSTICS_ERR_NO_HIT    = -3
} acoustics_status_t;

/**
 * @brief Result of a bearing computation.
 */
typedef struct {
    float    bearing_deg;   /**< Computed angle in degrees (-180 to +180) */
    bool     hit_detected;  /**< True if a distress call was detected */
    uint32_t timestamp_ms;  /**< Time of detection (ms since boot) */
} acoustics_result_t;

/**
 * @brief 2D occupancy grid populated by SpinScan_Execute.
 */
typedef struct {
    uint8_t  cells[ACOUSTICS_GRID_SIZE][ACOUSTICS_GRID_SIZE]; /**< 0=free, 1=obstacle */
    uint8_t  size;          /**< Grid dimension (cells per side) */
    float    resolution_cm; /**< Real-world size of each cell in cm */
} acoustics_grid_t;

/**
 * @brief Motor direction command output from Homing_Navigate.
 *        Aligned with Modül 1 UART format ('W', 'S', 'A', 'D', 'Q').
 */
typedef enum {
    ACOUSTICS_NAV_FORWARD  = 'W',
    ACOUSTICS_NAV_BACKWARD = 'S',
    ACOUSTICS_NAV_LEFT     = 'A',
    ACOUSTICS_NAV_RIGHT    = 'D',
    ACOUSTICS_NAV_STOP     = 'Q'
} acoustics_nav_cmd_t;

/* -- Public Functions ------------------------------------------------------ */

/**
 * @brief  Apply Software-based Digital IIR Filter to raw ADC microphone samples.
 *         Attenuates motor/gearbox noise to isolate distress calls.
 * @param  samples  Pointer to input sample buffer (raw ADC values)
 * @param  length   Number of samples in buffer
 * @param  out      Pointer to output buffer for filtered samples
 * @return ACOUSTICS_OK on success, negative error code otherwise
 */
acoustics_status_t acoustics_iir_filter_apply(const float *samples, uint16_t length, float *out);

/**
 * @brief  Compute the bearing angle to an acoustic source via phase difference
 *         across the microphone array.
 * @param  mic_buffers  Array of ACOUSTICS_MIC_COUNT filtered sample buffers
 * @param  length       Number of samples per buffer
 * @param  out          Pointer to acoustics_result_t to be filled
 * @return ACOUSTICS_OK if valid bearing computed, ACOUSTICS_ERR_NO_HIT otherwise
 */
acoustics_status_t acoustics_compute_bearing(const float *mic_buffers[], uint16_t length,
                                            acoustics_result_t *out);

/**
 * @brief  Execute a 360-degree Spin-Scan to populate the 2D occupancy grid.
 *         Robot rotates in place; ultrasonic readings sampled at each step.
 * @param  grid  Pointer to acoustics_grid_t to be populated
 * @return ACOUSTICS_OK on completion, negative error code otherwise
 */
acoustics_status_t acoustics_spinscan_execute(acoustics_grid_t *grid);

/**
 * @brief  Generate motor navigation commands to steer robot toward acoustic source.
 * @param  bearing_deg  Target bearing angle in degrees (-180 to +180)
 * @param  cmd          Pointer to acoustics_nav_cmd_t to be filled
 * @return ACOUSTICS_OK on success, negative error code otherwise
 */
acoustics_status_t acoustics_homing_navigate(float bearing_deg, acoustics_nav_cmd_t *cmd);

#endif /* MOD03_ACOUSTICS_IIR_H */
