# MOD-01 Embedded & Hardware Module

**Purpose:** This module handles real-time hardware reflexes via the STM32 microcontroller, drives DC motors, polls environmental sensors, manages the Dual Powerbank setup (12V Decoy), and ensures UART communication with the Raspberry Pi.

**Authors:**
* Ziya [Student ID TBD] (Primary - Power & UART)
* Ömer [Student ID TBD] (Secondary - Motors & Stuck Detection)
* Gabil [Student ID TBD] (Secondary - Env. Sensors)

**Dependencies:** * STM32 HAL Library
* Standard C libraries (`<stdint.h>`, `<stdbool.h>`)

### Quick-Start Integration Example

```c
#include "pwr_management.h"
#include "uart_comm.h"

int main(void) {
    // Initialize power management and UART
    pwr_init();
    uart_comm_init();
    
    // Enable 12V decoy for motors
    pwr_set_decoy_state(true);

    // Send telemetry to Pi 5 (Mapping values included)
    // Temp, Smoke, Stuck, Acoustic_Angle, Yaw_Angle, US_Front, US_Left, US_Right
    uart_telemetry_t my_data = {24.5, false, false, -45.0, 90.0, 150, 45, 60};
    uart_send_telemetry(&my_data);
    
    while(1) {
        // Failsafe check
        if(pwr_is_time_limit_exceeded()) {
            // Trigger RTH Protocol
        }
    }
}
```
### API Summary

* **`pwr_status_t pwr_init(void)`**
  * **Description:** Initializes power GPIO pins.
* **`void pwr_set_decoy_state(bool enable)`**
  * **Description:** Enables/disables 12V output for motors.
* **`bool pwr_is_time_limit_exceeded(void)`**
  * **Description:** Checks if 60-min RTH limit is hit.
* **`int8_t uart_comm_init(void)`**
  * **Description:** Initializes UART peripheral and DMA.
* **`void uart_send_telemetry(const uart_telemetry_t* data)`**
  * **Description:** Sends structured telemetry to Pi 5.
* **`bool uart_receive_command(uart_command_t* out_cmd)`**
  * **Description:** Parses incoming motor/light commands.
* **`TODO: [Ömer]`**
  * **Description:** `Motor_SetSpeed`, `Stuck_Check` etc. to be added.
* **`TODO: [Gabil]`**
  * **Description:** `Sensor_ReadDHT22`, `Sensor_ReadMQ2` etc. to be added.

## Interface Integration Updates

The following structural changes were applied to the files in this folder as part of the `Ceng_Integration` docx compliance revision:

- **[R1] Direction enum changed from int to char** (`uart_comm.h`): `UART_DIR_FORWARD=1` → `UART_DIR_FORWARD='W'`, etc. Docx §1B mandates `M:W:150|A:1\n` wire format; char-based enum lets STM32 map received characters directly without a lookup table.
- **[R2] Merged `buzzer_on` + `lights_on` into single `action_flag`** (`uart_comm.h`): Docx §1B protocol defines only one `A:<flag>` field in the UART command packet.
- **[R3] Separated `acoustic_angle` from `uart_telemetry_t` into new `uart_acoustic_t` struct** (`uart_comm.h`): Docx §4 requires acoustic data (`|A_Hit:<0/1>|A_Ang:<angle>\n`) to be a separate UART appendix packet, not embedded in telemetry.
- **[R4] Added three new functions** (`uart_comm.h`): `uart_send_acoustic()`, `UART_ReceiveCommand()`, and `Motor_SetState()` — mandated by docx §1B and §4.
- **`pwr_management.h`**: No changes required — already compliant.