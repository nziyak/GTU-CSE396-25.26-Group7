#ifndef MODULE_AI_VISION_H
#define MODULE_AI_VISION_H

/**
 * @file    ai_vision.h
 * @brief   AI & Vision Pipeline - Target detection and severity analysis interface
 * @author  Fatma Öztürk 230104004152
 * @date    2026-03-28
 * @version 0.2
 * * Changelog:
 * v0.1 - Initial draft with YOLO and VLM interface.
 * v0.2 - Aligned victim status enums with M5 Unity DataContracts.
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants & Macros ---------------------------------------------------- */
#define VISION_DEFAULT_FPS      5       /**< Low-power exploration framerate  */
#define VISION_MAX_FPS          25      /**< High-power assessment framerate [cite: 113] */
#define VISION_VLM_TIMEOUT_SEC  5       /**< Max latency allowed for Edge VLM  */

/* -- Data Types ------------------------------------------------------------ */

/** * @brief Victim status levels aligned with M5 Unity DataContracts.cs [cite: 157]
 */
typedef enum {
    VISION_STAT_NONE     = 0, /**< No victim in sight */
    VISION_STAT_STANDING = 1, /**< Person standing (Low Priority) [cite: 125] */
    VISION_STAT_LYING    = 2, /**< Person lying down (Medium Priority) [cite: 121] */
    VISION_STAT_TRAPPED  = 3  /**< Person trapped (High Priority) [cite: 121] */
} vision_status_t;

/** * @brief Mapping priority levels for Unity Map Pins [cite: 123, 125]
 */
typedef enum {
    VISION_PIN_GREEN     = 3, /**< Standing / Safe environment */
    VISION_PIN_YELLOW    = 2, /**< Lying / Needs attention */
    VISION_PIN_RED       = 1  /**< Trapped / Critical emergency */
} vision_priority_t;

/** * @brief Results of the AI analysis for the Augmented Status Report [cite: 127]
 */
typedef struct {
    uint16_t          target_id;      /**< ID for tracking multiple victims [cite: 120] */
    vision_status_t   status;         /**< Classified victim state */
    vision_priority_t priority;       /**< Assigned pin color for Unity [cite: 157] */
    float             confidence;     /**< Model confidence score (0.0-1.0) */
    uint16_t          bbox_area;      /**< Used for proximity tie-breaker  */
} vision_result_t;

/* -- Public Functions ------------------------------------------------------ */

/** * @brief  Initializes YOLOv8 and VLM models on the Raspberry Pi 5.
 * @return 0 on success, negative error code otherwise.
 */
int ai_vision_init(void);

/** * @brief  Adjusts FPS based on FSM state to prevent overheating[cite: 111, 112].
 * @param  high_perf If true, boosts FPS to VISION_MAX_FPS.
 */
void ai_vision_set_power_mode(bool high_perf);

/** * @brief  Pauses vision models to allow MOD-04 to run STT[cite: 133, 147].
 * @note   Prevents Out-Of-Memory (OOM) errors on the Pi 5.
 */
void ai_vision_preemptive_pause(void);

/** * @brief  Resumes vision pipeline after STT processing is complete[cite: 103].
 */
void ai_vision_resume(void);

/** * @brief  Analyzes the current frame to classify victim severity.
 * @param  out_result Pointer to store the analysis (Status, Priority, etc.).
 * @return 0 if target is confirmed, -1 if no target found.
 */
int ai_vision_process_frame(vision_result_t *out_result);

#endif /* MODULE_AI_VISION_H */