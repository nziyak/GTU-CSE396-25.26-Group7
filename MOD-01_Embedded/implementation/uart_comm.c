/**
 * @file    uart_comm.c
 * @brief   UART Communication Driver — STM32 ↔ Raspberry Pi 5
 * @author  Ziya 210104004027
 * @date    2026-05-16
 * @version 0.2
 *
 * Changelog:
 *   v0.1 - Header draft only.
 *   v0.2 - Full implementation: DMA-based UART TX, interrupt-based RX with
 *          ring buffer, telemetry serialisation, and command parsing.
 *
 * Protocol:
 *   TX (STM32 → Pi):  ASCII key-value pairs terminated by '\n'
 *     Example: "T:25.3|S:1|K:0|AA:-45.2|YW:12.5|UF:30|UB:40|UL:25|UR:35\n"
 *
 *   RX (Pi → STM32):  ASCII key-value pairs terminated by '\n'
 *     Example: "D:1|P:180|BZ:0|LT:1\n"
 *       D  = direction (0-4, maps to uart_direction_t)
 *       P  = speed PWM (0-255)
 *       BZ = buzzer on/off
 *       LT = lights on/off
 *
 * Hardware:
 *   - USART1 @ 115200 baud, 8N1
 *   - TX = PA9  (USART1_TX)
 *   - RX = PA10 (USART1_RX)
 *   - DMA is used for TX to avoid blocking.
 *   - RX uses byte-by-byte interrupt into a ring buffer.
 */

#include "uart_comm.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ===== External HAL Handle ============================================== */

extern UART_HandleTypeDef huart1;

/* ===== Ring Buffer for RX =============================================== */

#define RX_RING_SIZE  256   /* Must be power of 2 */

static volatile uint8_t  rx_ring[RX_RING_SIZE];
static volatile uint16_t rx_head = 0;   /* ISR writes here */
static volatile uint16_t rx_tail = 0;   /* main-loop reads here */

/* Single-byte DMA/IT target */
static uint8_t rx_byte;

/* Line buffer for assembling a complete command string */
static char    cmd_line[UART_MAX_BUFFER_SIZE];
static uint8_t cmd_line_idx = 0;

/* TX buffer (used by sprintf, sent via DMA) */
static char tx_buffer[UART_MAX_BUFFER_SIZE];

/* ===== Module State ===================================================== */

static bool g_initialized = false;

/* ===== Internal Helpers ================================================= */

/**
 * @brief  Push one byte into the ring buffer (called from ISR).
 */
static inline void ring_push(uint8_t byte)
{
    uint16_t next = (rx_head + 1) & (RX_RING_SIZE - 1);
    if (next != rx_tail) {          /* Drop byte on overflow */
        rx_ring[rx_head] = byte;
        rx_head = next;
    }
}

/**
 * @brief  Pop one byte from the ring buffer (called from main loop).
 * @return True if a byte was available, False if buffer empty.
 */
static inline bool ring_pop(uint8_t *out)
{
    if (rx_tail == rx_head) {
        return false;
    }
    *out = rx_ring[rx_tail];
    rx_tail = (rx_tail + 1) & (RX_RING_SIZE - 1);
    return true;
}

/**
 * @brief  Parse a key-value token like "D:1" or "BZ:0".
 *         Writes into the appropriate field of out_cmd.
 * @return True if the key was recognised, False otherwise.
 */
static bool parse_token(const char *token, uart_command_t *out_cmd)
{
    const char *colon = strchr(token, ':');
    if (colon == NULL) {
        return false;
    }

    /* Key length */
    size_t key_len = (size_t)(colon - token);
    const char *val_str = colon + 1;
    int val = atoi(val_str);

    if (key_len == 1 && token[0] == 'D') {
        /* Direction */
        if (val >= 0 && val <= 4) {
            out_cmd->direction = (uart_direction_t)val;
        }
        return true;
    }
    if (key_len == 1 && token[0] == 'P') {
        /* Speed PWM */
        out_cmd->speed_pwm = (uint8_t)(val > 255 ? 255 : (val < 0 ? 0 : val));
        return true;
    }
    if (key_len == 2 && token[0] == 'B' && token[1] == 'Z') {
        /* Buzzer */
        out_cmd->buzzer_on = (val != 0);
        return true;
    }
    if (key_len == 2 && token[0] == 'L' && token[1] == 'T') {
        /* Lights */
        out_cmd->lights_on = (val != 0);
        return true;
    }

    return false;  /* Unknown key — ignore */
}

/**
 * @brief  Parse a complete newline-terminated command line.
 *         Format: "D:1|P:180|BZ:0|LT:1"
 * @return True if at least one valid field was parsed, False otherwise.
 */
static bool parse_command_line(const char *line, uart_command_t *out_cmd)
{
    /* Set defaults */
    out_cmd->direction = UART_DIR_STOP;
    out_cmd->speed_pwm = 0;
    out_cmd->buzzer_on = false;
    out_cmd->lights_on = false;

    /* Work on a mutable copy */
    char buf[UART_MAX_BUFFER_SIZE];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    bool any_parsed = false;
    char *saveptr = NULL;
    char *token = strtok_r(buf, "|", &saveptr);

    while (token != NULL) {
        /* Skip leading whitespace */
        while (*token == ' ') token++;

        if (parse_token(token, out_cmd)) {
            any_parsed = true;
        }
        token = strtok_r(NULL, "|", &saveptr);
    }

    return any_parsed;
}

/* ===== UART RX Interrupt Callback ======================================= */

/**
 * @brief  HAL UART Rx complete callback — pushes received byte into ring buffer
 *         and re-arms the interrupt for the next byte.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        ring_push(rx_byte);
        /* Re-arm single-byte interrupt reception */
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/* ===== Public API ======================================================= */

/**
 * @brief  Initialises the UART peripheral and starts interrupt-based RX.
 *
 * Prerequisites: MX_USART2_UART_Init() must have been called by CubeMX
 * generated code before this function.
 *
 * @return 0 on success, negative value on error.
 */
int8_t uart_comm_init(void)
{
    /* Reset ring buffer state */
    rx_head = 0;
    rx_tail = 0;
    cmd_line_idx = 0;
    memset(cmd_line, 0, sizeof(cmd_line));

    /* Verify UART handle */
    if (huart1.Instance == NULL) {
        return -1;
    }

    /* Start byte-by-byte interrupt reception */
    if (HAL_UART_Receive_IT(&huart1, &rx_byte, 1) != HAL_OK) {
        return -1;
    }

    g_initialized = true;
    return 0;
}

/**
 * @brief  Serializes and transmits the telemetry struct to Pi 5 via UART.
 *
 * Output format (ASCII, newline-terminated):
 *   "T:<temp>|S:<smoke>|K:<stuck>|AA:<acoustic_angle>|YW:<yaw>|UF:<front>|UB:<back>|UL:<left>|UR:<right>\n"
 *
 * Field key mapping:
 *   T   = temperature (°C, 1 decimal)
 *   S   = smoke detected (0/1)
 *   K   = is stuck (0/1)
 *   AA  = acoustic angle (-180.0 to 180.0)
 *   YW  = IMU yaw angle (degrees)
 *   UF  = ultrasonic front (cm)
 *   UB  = ultrasonic back (cm)
 *   UL  = ultrasonic left (cm)
 *   UR  = ultrasonic right (cm)
 *
 * @param  data  The telemetry data populated by other STM32 modules.
 */
void uart_send_telemetry(const uart_telemetry_t *data)
{
    if (!g_initialized || data == NULL) {
        return;
    }

    int len = snprintf(tx_buffer, sizeof(tx_buffer),
        "T:%.1f|S:%d|K:%d|AA:%.1f|YW:%.1f|UF:%u|UB:%u|UL:%u|UR:%u\n",
        data->temperature,
        data->smoke_detected ? 1 : 0,
        data->is_stuck ? 1 : 0,
        data->acoustic_angle,
        data->imu_yaw_angle,
        (unsigned)data->us_dist_front,
        (unsigned)data->us_dist_back,
        (unsigned)data->us_dist_left,
        (unsigned)data->us_dist_right
    );

    if (len > 0 && len < (int)sizeof(tx_buffer)) {
        /* Blocking transmit with 100ms timeout.
         * For higher throughput, switch to HAL_UART_Transmit_DMA. */
        HAL_UART_Transmit(&huart1, (uint8_t *)tx_buffer, (uint16_t)len, 100);
    }
}

/**
 * @brief  Checks the RX ring buffer for new bytes, assembles a complete
 *         command line (terminated by '\n'), and parses it.
 *
 * This function is non-blocking and should be called from the main loop.
 *
 * @param  out_cmd  Pointer to caller-owned command struct to be populated.
 * @return True if a new valid command was received and parsed, False otherwise.
 */
bool uart_receive_command(uart_command_t *out_cmd)
{
    if (!g_initialized || out_cmd == NULL) {
        return false;
    }

    uint8_t byte;
    while (ring_pop(&byte)) {
        if (byte == UART_MSG_TERMINATOR) {
            /* End of line — null-terminate and parse */
            cmd_line[cmd_line_idx] = '\0';

            bool ok = false;
            if (cmd_line_idx > 0) {
                ok = parse_command_line(cmd_line, out_cmd);
            }

            cmd_line_idx = 0;
            if (ok) {
                return true;
            }
        } else {
            /* Accumulate byte into line buffer */
            if (cmd_line_idx < (UART_MAX_BUFFER_SIZE - 1)) {
                cmd_line[cmd_line_idx++] = (char)byte;
            } else {
                /* Line too long — discard and reset */
                cmd_line_idx = 0;
            }
        }
    }

    return false;
}
