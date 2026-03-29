#ifndef MODULE_AI_VISION_H
#define MODULE_AI_VISION_H

/**
 * @file    ai_vision.h
 * @brief   AI & Vision Pipeline public interface for target detection and victim severity analysis
 * @author  Fatma Öztürk 230104004152
 * @date    2026-03-29
 * @version 0.3
 *
 * Changelog:
 *   v0.1 - Initial draft with YOLO and VLM interface.
 *   v0.2 - Aligned victim status enums with Unity data contracts.
 *   v0.3 - Cleaned public interface comments and finalized module-level API.
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants & Macros --------------------------------------------------- */

/** @brief Default FPS used during low-power exploration mode. */
#define VISION_DEFAULT_FPS      5

/** @brief Maximum FPS used during high-performance victim assessment mode. */
#define VISION_MAX_FPS          25

/** @brief Maximum allowed timeout in seconds for vision-language inference. */
#define VISION_VLM_TIMEOUT_SEC  5

/* -- Data Types ----------------------------------------------------------- */

/**
 * @brief Victim status classification produced by the AI & Vision module.
 */
typedef enum
{
    VISION_STAT_NONE     = 0, /**< No victim detected in the current frame. */
    VISION_STAT_STANDING = 1, /**< Victim is standing. */
    VISION_STAT_LYING    = 2, /**< Victim is lying down. */
    VISION_STAT_TRAPPED  = 3  /**< Victim appears trapped or critically positioned. */
} vision_status_t;

/**
 * @brief Pin priority / color mapping used by dashboard and Unity visualization.
 */
typedef enum
{
    VISION_PIN_GREEN  = 3, /**< Low-priority / safe-state target marker. */
    VISION_PIN_YELLOW = 2, /**< Medium-priority target marker. */
    VISION_PIN_RED    = 1  /**< High-priority / critical target marker. */
} vision_priority_t;

/**
 * @brief Final result produced after processing a frame.
 *
 * This structure is intended to be shared with higher-level modules such as
 * FSM, Web Dashboard, and Unity Digital Twin integration layers.
 */
typedef struct
{
    uint16_t          target_id;       /**< Unique ID for the detected target. */
    vision_status_t   status;          /**< Classified victim status. */
    vision_priority_t priority;        /**< Visualization / queue priority level. */
    float             confidence;      /**< Model confidence score in range [0.0, 1.0]. */
    uint16_t          bbox_area;       /**< Bounding-box area used for proximity tie-breaks. */
    bool              target_detected; /**< True if a valid target is detected in frame. */
} vision_result_t;

/* -- Public Functions ----------------------------------------------------- */

/**
 * @brief  Initialize the AI & Vision pipeline on the Raspberry Pi.
 *
 * This function prepares the required camera-side and model-side resources
 * needed for person detection and victim analysis.
 *
 * @return 0 on success, negative error code otherwise.
 */
int ai_vision_init(void);

/**
 * @brief  Set the current power mode of the vision pipeline.
 *
 * In low-power mode, the module is expected to operate near
 * VISION_DEFAULT_FPS. In high-performance mode, it may increase its runtime
 * FPS up to VISION_MAX_FPS for detailed victim assessment.
 *
 * @param  high_perf True to enable high-performance mode, false for default mode.
 */
void ai_vision_set_power_mode(bool high_perf);

/**
 * @brief  Pause the vision pipeline to free resources for other critical modules.
 *
 * This is primarily used during STT or other high-cost operations that require
 * temporary preemption of vision workloads.
 */
void ai_vision_preemptive_pause(void);

/**
 * @brief  Resume the vision pipeline after a temporary pause.
 */
void ai_vision_resume(void);

/**
 * @brief  Process the current frame and produce a victim analysis result.
 *
 * If a valid human target is detected, the output structure is filled with
 * status, priority, confidence, and proximity-related metadata.
 *
 * @param  out_result Pointer to caller-owned output result structure.
 * @return 0 if processing succeeds and a target result is produced,
 *         negative error code otherwise.
 */
int ai_vision_process_frame(vision_result_t *out_result);

/**
 * @brief  Shutdown the AI & Vision pipeline and release allocated resources.
 */
void ai_vision_shutdown(void);

#endif /* MODULE_AI_VISION_H */
