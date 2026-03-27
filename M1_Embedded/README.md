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
API SummaryFunctionParametersReturn ValueDescriptionpwr_initvoidpwr_status_tInitializes power GPIO pins.pwr_set_decoy_statebool enablevoidEnables/disables 12V output for motors.pwr_is_time_limit_exceededvoidboolChecks if 60-min RTH limit is hit.uart_comm_initvoidint8_tInitializes UART peripheral and DMA.uart_send_telemetryconst uart_telemetry_t* datavoidSends structured telemetry to Pi 5.uart_receive_commanduart_command_t* out_cmdboolParses incoming motor/light commands.TODO: [Ömer]......Motor_SetSpeed, Stuck_Check vb. eklenecek.TODO: [Gabil]......Sensor_ReadDHT22, Sensor_ReadMQ2 vb. eklenecek.Known Limitations and TODOSTODO: Define exact GPIO pins for L298N PWM channels.Limitation: UART DMA buffer size is strictly limited to 128 bytes; payload must not exceed this.Version Historyv0.2 (2026-03-27): Updated telemetry payload to include dead-reckoning variables.v0.1 (2026-03-27): Initial draft, defined power management and UART communication contracts.