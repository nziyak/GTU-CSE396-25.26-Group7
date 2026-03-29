#ifndef UART_COMM_H
#define UART_COMM_H

/**
 * @file      uart_comm.h
 * @brief     UART Communication Interface between STM32 and Raspberry Pi 5
 * @author    Ziya 210104004027
 * @date      2026-03-27
 * @version   0.4 (Ceng_Integration)
 *
 * Changelog:
 * v0.1 (2026-03-27) - Initial draft, structs for telemetry and commands.
 * v0.2 (2026-03-27) - Added acoustic angle variable to telemetry payload.
 * v0.3 (2026-03-27) - Added imu yaw angle and ultrasonic distance sensors variables for mapping.
 * v0.4 (Ceng_Integration) - Docx compliance revision:
 *   [R1] direction enum int(0-4) → char('W','A','S','D','Q')   (docx §1B)
 *   [R2] buzzer_on + lights_on → single action_flag                (docx §1B)
 *   [R3] acoustic_angle separated from telemetry → uart_acoustic_t   (docx §4)
 *   [R4] UART_ReceiveCommand, Motor_SetState, uart_send_acoustic added (docx §1B, §4)
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants & Macros ---------------------------------------------------- */
#define UART_BAUD_RATE        115200
#define UART_MAX_BUFFER_SIZE  128
#define UART_MSG_TERMINATOR   '\n'

/* -- Data Types ------------------------------------------------------------ */

/**
 * @brief Motor direction commands received from Pi 5.
 *        [R1] Docx §1B rule: W=Forward, A=Left, S=Backward, D=Right, Q=Stop (char-based).
 *        Original: was int-based (0,1,2,3,4). Reason for change: M4 Python
 *        side sends serial_send_command() in "M:W:150|A:1\n" format;
 *        STM32 needs to map these directly as char values.
 */
typedef enum {
    UART_DIR_FORWARD  = 'W',   /* Original: UART_DIR_FORWARD = 1 */
    UART_DIR_LEFT     = 'A',   /* Original: UART_DIR_LEFT    = 3 */
    UART_DIR_BACKWARD = 'S',   /* Original: UART_DIR_BACKWARD= 2 */
    UART_DIR_RIGHT    = 'D',   /* Original: UART_DIR_RIGHT   = 4 */
    UART_DIR_STOP     = 'Q'    /* Original: UART_DIR_STOP    = 0 */
} uart_direction_t;

/**
 * @brief Telemetry payload sent FROM STM32 TO Raspberry Pi (M1 → M4).
 *        Docx §1A format: T:<temp>|S:<smoke>|ST:<stuck>\n
 *
 *        [R3] acoustic_angle was removed from this struct. Per Docx §4, acoustic
 *        data is sent as a separate UART appendix (|A_Hit:1|A_Ang:-45.0\n).
 *        Therefore a new uart_acoustic_t struct was created.
 *
 *        imu_yaw_angle and us_dist_* fields were added by Ziya;
 *        they are outside docx scope but required for navigation, so they were kept.
 */
typedef struct {
    float   temperature;      /**< DHT22 temperature reading in Celsius      */
    bool    smoke_detected;   /**< MQ-2 digital threshold status             */
    bool    is_stuck;         /**< MPU6050 stuck detection flag              */
    /* acoustic_angle REMOVED — [R3] now resides in uart_acoustic_t */
    float   imu_yaw_angle;    /**< Robot's current compass heading (z-axis)  */
    uint8_t us_dist_front;    /**< Front ultrasonic distance (cm)            */
    uint8_t us_dist_back;     /**< Back ultrasonic distance (cm)             */
    uint8_t us_dist_left;     /**< Left ultrasonic distance (cm)             */
    uint8_t us_dist_right;    /**< Right ultrasonic distance (cm)            */
} uart_telemetry_t;

/**
 * @brief [R3] Acoustic bearing payload (M3 → M4, separate UART appendix).
 *        Docx §4 rule: "|A_Hit:<0/1>|A_Ang:<angle>\n"
 *        Angle: -180.0 to +180.0 degrees.
 *
 *        NEW STRUCT — did not exist in the original file. Reason: Docx §4
 *        mandates that acoustic data be sent as a separate packet from
 *        telemetry. This struct will be parsed by M4 serial_read_acoustic().
 */
typedef struct {
    bool    audio_hit;        /**< True if distress call detected (A_Hit)    */
    float   audio_angle;      /**< Bearing angle in degrees (A_Ang)          */
} uart_acoustic_t;

/**
 * @brief Command payload received FROM Raspberry Pi TO STM32 (M4 → M1).
 *        Docx §1B format: M:<dir>:<speed>|A:<action_flag>\n
 *        Example: M:W:150|A:1\n
 *
 *        [R2] buzzer_on + lights_on → single action_flag.
 *        Reason for change: Docx §1B protocol defines only one "A:<flag>"
 *        field. A single bool instead of two simplifies UART parsing
 *        and ensures compliance with the docx format.
 */
typedef struct {
    uart_direction_t direction;    /**< Motor direction (W/A/S/D/Q)          */
    uint8_t          speed_pwm;    /**< Motor speed PWM (0-255)              */
    bool             action_flag;  /**< Buzzer+LED toggle (original: buzzer_on + lights_on were separate) */
} uart_command_t;

/* -- Public Functions ------------------------------------------------------ */

/**
 * @brief  Initialises the UART peripheral and DMA buffers.
 * @return 0 on success, negative value on error.
 */
int8_t uart_comm_init(void);

/**
 * @brief  Serializes and transmits the telemetry struct to Pi 5 via UART.
 *         Docx §1A name: UART_SendTelemetry(TelemetryData data)
 * @param  data Telemetry data populated by STM32 sensors.
 */
void uart_send_telemetry(const uart_telemetry_t* data);

/** @brief Docx-compliant alias for uart_send_telemetry. */
#define UART_SendTelemetry(data_ptr)  uart_send_telemetry(data_ptr)

/**
 * @brief  [R4] Serializes and transmits acoustic bearing to Pi 5 via UART.
 *         Docx §4 format: |A_Hit:<0/1>|A_Ang:<angle>\n
 *
 *         NEW FUNCTION — did not exist in the original. Reason: since uart_acoustic_t
 *         was defined as a separate struct, it requires its own send function.
 * @param  data Acoustic result from M3 IIR filter processing.
 */
void uart_send_acoustic(const uart_acoustic_t* data);

/**
 * @brief  Checks the RX buffer for new commands from Pi 5 and parses them.
 * @param  out_cmd Pointer to command struct to be populated.
 * @return True if a new valid command was received, False otherwise.
 */
bool uart_receive_command(uart_command_t* out_cmd);

/**
 * @brief  [R4] Docx §1B sub-function: reads command from raw buffer.
 *         Docx name: UART_ReceiveCommand(uint8_t* buffer)
 *
 *         NEW FUNCTION — did not exist in the original. Reason: Docx §1B
 *         states "incoming packet will be captured via UART Receive interrupt";
 *         this raw-buffer version is used inside the interrupt handler.
 * @param  buffer Raw UART RX byte buffer.
 */
void UART_ReceiveCommand(uint8_t* buffer);

/**
 * @brief  [R4] Applies parsed command to L298N H-Bridge via PWM.
 *         Docx §1B name: Motor_SetState(MotorDirection dir, uint8_t speed)
 *
 *         NEW FUNCTION — did not exist in the original. Reason: Docx §1B
 *         states "will parse the command and apply voltage to PWM pins".
 *         Called after uart_receive_command.
 * @param  dir   Motor direction (W/A/S/D/Q)
 * @param  speed PWM duty cycle (0-255)
 */
void Motor_SetState(uart_direction_t dir, uint8_t speed);

#endif /* UART_COMM_H */
