#ifndef CAMERA_INTERNAL_H
#define CAMERA_INTERNAL_H

/**
 * @file    camera_internal.h
 * @brief   Internal Pi Camera interface for AI & Vision Pipeline
 * @author  Gabil Rahimli 230104004902
 * @date    2026-03-29
 * @version 0.1
 *
 * Changelog:
 *   v0.1 - Initial internal camera control API.
 */

#include <stdint.h>
#include "vision_internal_types.h"

/* -- Camera Return Codes -------------------------------------------------- */
typedef enum
{
    CAMERA_INT_OK = 0,
    CAMERA_INT_ERR_INIT = -1,
    CAMERA_INT_ERR_START = -2,
    CAMERA_INT_ERR_CAPTURE = -3,
    CAMERA_INT_ERR_INVALID_ARG = -4
} camera_int_status_t;

/* -- Camera Configuration ------------------------------------------------- */
typedef struct
{
    uint16_t frame_width;
    uint16_t frame_height;
    uint8_t  fps;
} camera_int_config_t;

/**
 * @brief  Initialize Pi Camera with given configuration.
 * @param  config Pointer to camera configuration.
 * @return CAMERA_INT_OK on success, error code otherwise.
 */
camera_int_status_t vision_camera_init(const camera_int_config_t *config);

/**
 * @brief  Start camera stream.
 * @return CAMERA_INT_OK on success, error code otherwise.
 */
camera_int_status_t vision_camera_start(void);

/**
 * @brief  Capture one frame from camera.
 * @param  out_frame Pointer to caller-owned frame structure.
 * @return CAMERA_INT_OK on success, error code otherwise.
 */
camera_int_status_t vision_camera_capture_frame(vision_frame_t *out_frame);

/**
 * @brief  Update runtime camera FPS.
 * @param  fps New target FPS.
 * @return CAMERA_INT_OK on success, error code otherwise.
 */
camera_int_status_t vision_camera_set_fps(uint8_t fps);

/**
 * @brief  Release memory/resources associated with a captured frame.
 * @param  frame Pointer to frame structure.
 */
void vision_camera_release_frame(vision_frame_t *frame);

/**
 * @brief  Stop camera stream safely.
 */
void vision_camera_stop(void);

/**
 * @brief  Shutdown camera and release all related resources.
 */
void vision_camera_shutdown(void);

#endif /* CAMERA_INTERNAL_H */
