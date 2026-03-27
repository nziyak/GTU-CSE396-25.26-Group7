#ifndef UART_COMM_H
#define UART_COMM_H

/**
 * @file      uart_comm.h
 * @brief     UART Communication Interface between STM32 and Raspberry Pi 5
 * @author    Ziya 210104004027
 * @date      2026-03-27
 * @version   0.2
 * * Changelog:
 * v0.1 (2026-03-27) - Initial draft, structs for telemetry and commands.
 * v0.2 (2026-03-27) - Added acoustic angle variable to telemetry payload.
 */

#include <stdint.h>
#include <stdbool.h>

/* -- Constants & Macros ---------------------------------------------------- */
#define UART_BAUD_RATE        115200
#define UART_MAX_BUFFER_SIZE  128
#define UART_MSG_TERMINATOR   '\n'

/* -- Data Types ------------------------------------------------------------ */
/**
 * @brief Motor direction commands received from Pi 5
 */
typedef enum {
    UART_DIR_STOP     = 0,
    UART_DIR_FORWARD  = 1,
    UART_DIR_BACKWARD = 2,
    UART_DIR_LEFT     = 3,
    UART_DIR_RIGHT    = 4
} uart_direction_t;

/**
 * @brief Telemetry payload sent FROM STM32 TO Raspberry Pi
 */
typedef struct {
    float   temperature;      /**< DHT22 temperature reading in Celsius */
    bool    smoke_detected;   /**< MQ-2 digital threshold status */
    bool    is_stuck;         /**< MPU6050 stuck detection flag */
    float   acoustic_angle;   /**< IIR filtered acoustic bearing (-180 to 180) */
} uart_telemetry_t;

/**
 * @brief Command payload received FROM Raspberry Pi TO STM32
 */
typedef struct {
    uart_direction_t direction;  /**< Target motor direction */
    uint8_t          speed_pwm;  /**< Motor speed (0-255) */
    bool             buzzer_on;  /**< Trigger wake-up buzzer */
    bool             lights_on;  /**< Trigger SOS / Flashlight */
} uart_command_t;

/* -- Public Functions ------------------------------------------------------ */
/**
 * @brief  Initialises the UART peripheral and DMA buffers.
 * @return 0 on success, negative value on error.
 */
int8_t uart_comm_init(void);

/**
 * @brief  Serializes and transmits the telemetry struct to Pi 5 via UART.
 * @param  data The telemetry data populated by other STM32 modules.
 */
void uart_send_telemetry(const uart_telemetry_t* data);

/**
 * @brief  Checks the RX buffer for new commands from Pi 5 and parses them.
 * @param  out_cmd Pointer to caller-owned command struct to be populated.
 * @return True if a new valid command was received, False otherwise.
 */
bool uart_receive_command(uart_command_t* out_cmd);

#endif /* UART_COMM_H */