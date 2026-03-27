# MOD-01 Embedded & Hardware Module

**Purpose:** This module handles real-time hardware reflexes via the STM32 microcontroller, drives DC motors, polls environmental sensors, manages the Dual Powerbank setup (12V Decoy), and ensures UART communication with the Raspberry Pi.

**Authors:**
* Ziya [Öğrenci Nonu Yaz] (Primary - Power & UART)
* Ömer [Öğrenci Nonu Yaz] (Secondary - Motors & Stuck Detection)
* Gabil [Öğrenci Nonu Yaz] (Secondary - Env. Sensors)

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
  * **Description:** `Motor_SetSpeed`, `Stuck_Check` vb. eklenecek.
* **`TODO: [Gabil]`**
  * **Description:** `Sensor_ReadDHT22`, `Sensor_ReadMQ2` vb. eklenecek.