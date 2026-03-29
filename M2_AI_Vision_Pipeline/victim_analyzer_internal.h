#ifndef VICTIM_ANALYZER_INTERNAL_H
#define VICTIM_ANALYZER_INTERNAL_H

/**
 * @file    victim_analyzer_internal.h
 * @brief   Internal victim severity analysis interface
 * @author  Fatma Öztürk 230104004152
 * @date    2026-03-29
 * @version 0.1
 *
 * Changelog:
 *   v0.1 - Initial internal analyzer API for standing / lying / trapped classification.
 */

#include "ai_vision.h"
#include "vision_internal_types.h"
#include "human_detector_internal.h"

/* -- Analyzer Return Codes ------------------------------------------------ */
typedef enum
{
    ANALYZER_INT_OK = 0,
    ANALYZER_INT_ERR_INIT = -1,
    ANALYZER_INT_ERR_MODEL = -2,
    ANALYZER_INT_ERR_INFER = -3,
    ANALYZER_INT_ERR_INVALID_ARG = -4
} analyzer_int_status_t;

/**
 * @brief  Initialize victim severity analysis backend.
 * @param  backend Selected backend for severity analysis.
 * @return ANALYZER_INT_OK on success, error code otherwise.
 */
analyzer_int_status_t vision_analyzer_init(vision_backend_t backend);

/**
 * @brief  Classify a detected target as standing, lying, or trapped.
 * @param  frame Full camera frame.
 * @param  target Selected detection candidate.
 * @param  out_result Pointer to public result output.
 * @return ANALYZER_INT_OK on success, error code otherwise.
 */
analyzer_int_status_t vision_analyzer_classify(const vision_frame_t *frame,
                                               const vision_detection_t *target,
                                               vision_result_t *out_result);

/**
 * @brief  Map victim status to public pin color.
 * @param  status Victim severity/status.
 * @return Corresponding pin color for dashboard / digital twin usage.
 */
vision_priority_t vision_analyzer_map_priority(vision_status_t status);

/**
 * @brief  Shutdown victim analyzer and release model resources.
 */
void vision_analyzer_shutdown(void);

#endif /* VICTIM_ANALYZER_INTERNAL_H */
