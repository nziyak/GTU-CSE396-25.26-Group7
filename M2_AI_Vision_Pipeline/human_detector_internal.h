#ifndef HUMAN_DETECTOR_INTERNAL_H
#define HUMAN_DETECTOR_INTERNAL_H

/**
 * @file    human_detector_internal.h
 * @brief   Internal human detection interface for YOLO-based person search
 * @author  Gabil Rahimli 230104004902
 * @date    2026-03-29
 * @version 0.1
 *
 * Changelog:
 *   v0.1 - Initial internal human detector API.
 */

#include <stdint.h>
#include "vision_internal_types.h"

/* -- Detector Return Codes ------------------------------------------------ */
typedef enum
{
    DETECTOR_INT_OK = 0,
    DETECTOR_INT_ERR_INIT = -1,
    DETECTOR_INT_ERR_MODEL = -2,
    DETECTOR_INT_ERR_INFER = -3,
    DETECTOR_INT_ERR_INVALID_ARG = -4
} detector_int_status_t;

/**
 * @brief  Initialize human detector backend.
 * @param  backend Selected backend for detection stage.
 * @return DETECTOR_INT_OK on success, error code otherwise.
 */
detector_int_status_t vision_detector_init(vision_backend_t backend);

/**
 * @brief  Detect human candidates in the given frame.
 * @param  frame Input frame.
 * @param  threshold Minimum confidence threshold.
 * @param  out_list Pointer to detection list output.
 * @return DETECTOR_INT_OK on success, error code otherwise.
 */
detector_int_status_t vision_detector_detect(const vision_frame_t *frame,
                                             float threshold,
                                             vision_detection_list_t *out_list);

/**
 * @brief  Select the best target candidate among detections.
 * @param  detections Input detection list.
 * @param  out_target Pointer to selected detection output.
 * @return DETECTOR_INT_OK on success, error code otherwise.
 */
detector_int_status_t vision_detector_select_best_target(
    const vision_detection_list_t *detections,
    vision_detection_t *out_target);

/**
 * @brief  Shutdown detector backend and free related resources.
 */
void vision_detector_shutdown(void);

#endif /* HUMAN_DETECTOR_INTERNAL_H */
