#ifndef MODULE_AI_VISION_H
#define MODULE_AI_VISION_H

/**
 * @file    ai_vision.h
 * @brief   AI & Vision Pipeline - Target detection and severity analysis interface
 * @author  Fatma Öztürk 230104004152
 * @date    2026-03-28
 * @version 0.3 (Ceng_Integration)
 *
 * Changelog:
 * v0.1 - Initial draft with YOLO and VLM interface.
 * v0.2 - Aligned victim status enums with M5 Unity DataContracts.
 * v0.3 (Ceng_Integration) - Docx compliance revision:
 *   [R5] bbox_area(uint16) → vision_bbox_t struct (x,y,w,h)  (docx §3A)
 *   [R6] get_vision_results() function added                   (docx §3A)
 *   [R7] pause_vision_pipeline/resume_vision_pipeline alias   (docx §3B)
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants & Macros ---------------------------------------------------- */
#define VISION_DEFAULT_FPS      5       /**< Low-power exploration framerate       */
#define VISION_MAX_FPS          25      /**< High-power assessment framerate       */
#define VISION_VLM_TIMEOUT_SEC  5       /**< Max latency allowed for Edge VLM      */

/* -- Data Types ------------------------------------------------------------ */

/** @brief Victim status levels aligned with M5 Unity DataContracts.cs */
typedef enum {
    VISION_STAT_NONE     = 0, /**< No victim in sight                        */
    VISION_STAT_STANDING = 1, /**< Person standing (Low Priority)             */
    VISION_STAT_LYING    = 2, /**< Person lying down (Medium Priority)        */
    VISION_STAT_TRAPPED  = 3  /**< Person trapped (High Priority)             */
} vision_status_t;

/** @brief Mapping priority levels for Unity Map Pins */
typedef enum {
    VISION_PIN_GREEN     = 3, /**< Standing / Safe environment                */
    VISION_PIN_YELLOW    = 2, /**< Lying / Needs attention                    */
    VISION_PIN_RED       = 1  /**< Trapped / Critical emergency               */
} vision_priority_t;

/**
 * @brief [R5] Bounding box for a detected target.
 *        Docx §3A rule: TargetData(bbox=(x,y,w,h), ...)
 *
 *        NEW STRUCT — did not exist in the original (only bbox_area:uint16 existed).
 *        Reason: Docx §3A defines M2→M4 data format as
 *        "TargetData(bbox=(x,y,w,h), ...)".
 *        The M4 Python side TargetData dataclass uses the same (x,y,w,h)
 *        fields; the C side must match this structure.
 */
typedef struct {
    uint16_t x;     /**< Top-left X coordinate of bounding box */
    uint16_t y;     /**< Top-left Y coordinate of bounding box */
    uint16_t w;     /**< Width of bounding box                 */
    uint16_t h;     /**< Height of bounding box                */
} vision_bbox_t;

/**
 * @brief Results of the AI analysis for the Augmented Status Report.
 *        Docx §3A: TargetData(bbox=(x,y,w,h), status=VictimSeverity.TRAPPED,
 *                              confidence=0.85)
 *
 *        [R5] bbox_area → vision_bbox_t bbox change:
 *        Original: only bbox_area (uint16) existed — for proximity tie-breaker.
 *        New: full (x,y,w,h) struct — M4 needs bounding box coordinates to
 *        steer the robot toward the target. Area can be computed as w*h.
 */
typedef struct {
    uint16_t          target_id;   /**< ID for tracking multiple victims      */
    vision_bbox_t     bbox;        /**< (x,y,w,h) bounding box [R5]          */
    vision_status_t   status;      /**< Classified victim state               */
    vision_priority_t priority;    /**< Assigned pin color for Unity          */
    float             confidence;  /**< Model confidence score (0.0-1.0)      */
} vision_result_t;

/* -- Public Functions ------------------------------------------------------ */

/** @brief  Initializes YOLOv8 and VLM models on the Raspberry Pi 5. */
int ai_vision_init(void);

/** @brief  Adjusts FPS based on FSM state to prevent overheating. */
void ai_vision_set_power_mode(bool high_perf);

/** @brief  Pauses vision models to allow MOD-04 to run STT.
 *  @note   Prevents Out-Of-Memory (OOM) errors on the Pi 5. */
void ai_vision_preemptive_pause(void);

/** @brief  Resumes vision pipeline after STT processing is complete. */
void ai_vision_resume(void);

/** @brief  Analyzes the current frame to classify victim severity.
 *  @param  out_result Pointer to store the analysis.
 *  @return 0 if target is confirmed, -1 if no target found. */
int ai_vision_process_frame(vision_result_t *out_result);

/* -- Docx-mandated interface aliases --------------------------------------- */

/**
 * @brief  [R6] Docx §3A function: get_vision_results()
 *         "Status of the person seen by the YOLO or VLM model on camera
 *         and estimated distance."
 *
 *         NEW FUNCTION — did not exist in the original. Reason: Docx §3A
 *         defines this as the primary function through which M2 provides
 *         data to M4. Wraps ai_vision_process_frame but returns multiple
 *         results: multiple victims may be detected in a single frame.
 *
 * @param  out_results  Output array for detected targets
 * @param  max_results  Maximum capacity of out_results array
 * @param  out_count    Number of results actually written
 * @return 0 on success, -1 if no target found
 */
int get_vision_results(vision_result_t *out_results, uint8_t max_results,
                       uint8_t *out_count);

/**
 * @brief  [R7] Docx §3B aliases: pause_vision_pipeline / resume_vision_pipeline
 *
 *         Original names were ai_vision_preemptive_pause / ai_vision_resume.
 *         Docx §3B states "M2 Receiving Function: pause_vision_pipeline() and
 *         resume_vision_pipeline()". M4_MainFSM.py will call these names.
 *         C macros redirect to the existing functions.
 */
#define pause_vision_pipeline()   ai_vision_preemptive_pause()
#define resume_vision_pipeline()  ai_vision_resume()

#endif /* MODULE_AI_VISION_H */
