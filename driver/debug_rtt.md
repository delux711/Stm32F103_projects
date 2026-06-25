# RTT Debugging with Log Levels (DEBUG, INFO, OK, WARN, ERROR)

## New: Debug Log Levels

RTT now supports 5 debug output levels with color-coded prefixes:

| Level | Prefix | Usage | Example |
|-------|--------|-------|---------|
| **DEBUG** | `[DEBUG]` | Development tracing | `rtt_debug("Reg addr=0x%04x\r\n", addr)` |
| **INFO** | `[INFO]` | Normal operation | `rtt_info("Counter: %lu\r\n", cnt)` |
| **OK** | `[OK]` | Success, initialization | `rtt_ok("Device ready\r\n")` |
| **WARN** | `[WARN]` | Warning | `rtt_warn("Timeout %u ms\r\n", time)` |
| **ERROR** | `[ERROR]` | Error condition | `rtt_error("Failed: %d\r\n", err)` |

## Header File: `debug_rtt_levels.h`

New header with all debug helper functions.

**Include:**
```c
#include "debug_rtt_levels.h"
```

**Available Functions:**
```c
rtt_printf(format, ...)   // No prefix
rtt_debug(format, ...)    // [DEBUG] prefix
rtt_info(format, ...)     // [INFO] prefix
rtt_ok(format, ...)       // [OK] prefix
rtt_warn(format, ...)     // [WARN] prefix
rtt_error(format, ...)    // [ERROR] prefix
```

## Usage Examples

### Initialization

```c
#include "debug_rtt_levels.h"

int main(void) {
    DEBUG_initTrace(SystemCoreClock);

    rtt_ok("System initialized\r\n");

    if (DeviceConfig_init() != 0) {
        rtt_error("Config init failed\r\n");
        return -1;
    }

    rtt_ok("Device configured\r\n");
}
```

### Reading Sensor Data

```c
while (1) {
    uint16_t temp = ADC_readInternalTempC10();
    rtt_info("Temperature: %d.%d C\r\n", temp/10, temp%10);

    if (temp > 8000) {  // 80°C
        rtt_warn("High temperature: %d C\r\n", temp/10);
    }
}
```

### MODBUS Debugging

```c
if (ModbusSlave_init(&cfg, slave_addr) != 0) {
    rtt_error("MODBUS init failed\r\n");
} else {
    rtt_ok("MODBUS slave ready at addr %d\r\n", slave_addr);
}

// In processing loop:
rtt_info("MODBUS request from addr %u\r\n", from_addr);
```

### Button Events

```c
void btn_callback(button_event_t evt) {
    switch(evt) {
        case BTN_SINGLE:
            rtt_info("Button: single click\r\n");
            break;
        case BTN_ERROR:
            rtt_error("Button: error state\r\n");
            break;
    }
}
```

## RTT Output in Ozone

**Expected Output:**
```
[OK] System initialized
[OK] Device configured
[INFO] Temperature: 25.3 C
[INFO] MODBUS request from addr 1
[DEBUG] Reg addr=0x0010 value=0x1234
[WARN] High temperature: 80 C
[OK] MODBUS slave ready at addr 1
```

## Applications with Debug Levels

All applications updated to use debug levels:

| Application | Element | Level |
|-------------|---------|-------|
| **test_modbus_slave** | Device ID, Address | OK |
| **test_modbus_slave** | FW Version | DEBUG |
| **test_modbus_slave** | Ready message | OK |
| **test_modbus_slave** | Counter | INFO |
| **test_adc_light** | Start | OK |
| **test_adc_light** | Sensor data | INFO |
| **test_button** | Click/Press events | INFO |
| **test_RS485_modbus** | Ready | OK |
| **driver/adc.c** | Debug info | DEBUG |
| **driver/device_config.c** | Errors | ERROR |

## Advantages of Log Levels

1. ✅ **Clarity** - See message type at a glance
2. ✅ **Filtering** - Ozone can filter by prefix
3. ✅ **Consistency** - Unified format across project
4. ✅ **Maintenance** - Easy to understand others' code
5. ✅ **Production** - Can be disabled on Release builds

## Viewing Output

### SEGGER Ozone

1. Open debug session
2. `Window` → `Real-Time Terminal`
3. Messages appear with prefixes

### J-Link RTT Client

```bash
JLinkRTTClient
```

Output with prefixes appears in real-time.

### Required Includes

```c
#include <stdio.h>     // For vsnprintf
#include <stdarg.h>    // For va_list, va_start, va_end
#include "debug.h"     // For DEBUG_writeString
```

### Usage

**Instead of:**
```c
printf("Device ID: %d\r\n", cfg->device_id);
printf("MODBUS Addr: %d\r\n", cfg->modbus_slave_addr);
printf("Counter: %lu\r\n", counter);
```

**Use:**
```c
rtt_printf("Device ID: %d\r\n", cfg->device_id);
rtt_printf("MODBUS Addr: %d\r\n", cfg->modbus_slave_addr);
rtt_printf("Counter: %lu\r\n", counter);
```

## How to View Output

### Via SEGGER Ozone

1. Open Ozone debug session
2. Go to `Window` → `Real-Time Terminal`
3. Run application
4. RTT output appears in window

### Via SEGGER J-Link RTT Client

```bash
# In terminal
JLinkRTTClient

# Expected output:
# =========================== MODBUS Slave (Device Registry) ===
# Device ID: 0
# MODBUS Addr: 1
# Device Type: 0x00
# FW Version: 1
# Ready - waiting for MODBUS requests
# Counter: 1
# Counter: 2
# ...
```

## Memory Optimization

Buffer size `256` bytes is sufficient for most messages. Can be adjusted:

```c
static char buffer[512];  // For longer messages
// or
static char buffer[128];  // For compact application
```

## Supported Formatting

Helper function supports all standard `printf` formats:

```c
rtt_printf("Int: %d\r\n", 42);           // Integer
rtt_printf("Hex: 0x%02x\r\n", 0xFF);    // Hexadecimal
rtt_printf("Float: %.2f\r\n", 3.14);    // Float (if FPU available)
rtt_printf("String: %s\r\n", "Hello");  // String
rtt_printf("Multiple: %d, %s, 0x%x\r\n", 10, "test", 255);
```

## Performance

- **vsnprintf()** - ~100-500 µs (depending on format length)
- **DEBUG_writeString()** - ~50-200 µs (depending on string length)
- **RTT transport** - Virtually latency-free (shared memory)

No blocking of main loop.

## Example: Test Modbus Slave

Full integration seen in `apps/test_modbus_slave/main.c`:

```c
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "debug.h"

static void rtt_printf(const char *format, ...)
{
    static char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    DEBUG_writeString(buffer);
}

int main(void)
{
    DEBUG_initTrace(SystemCoreClock);

    // ...

    rtt_printf("Device ID: %d\r\n", cfg->device_id);
    rtt_printf("MODBUS Addr: %d\r\n", cfg->modbus_slave_addr);

    while (1) {
        if ((now - last_ms) >= 1000) {
            counter++;
            rtt_printf("Counter: %lu\r\n", counter);
        }
    }
}
```

## Troubleshooting

### Output Not Appearing

- Check `DEBUG_initTrace()` is called in `main()`
- Check RTT is enabled in debug configuration (Ozone, KEIL)
- Check JTAG/SWD physical connection

### Buffer Overflow

If message is truncated:
```c
// Increase buffer
static char buffer[512];
```

### Formatting Error

If format doesn't display correctly:
```c
// Check correct format for data type
rtt_printf("Value: %u\r\n", unsigned_value);  // %u for unsigned
rtt_printf("Value: %d\r\n", signed_value);    // %d for signed
rtt_printf("Value: %lu\r\n", unsigned_long);  // %lu for unsigned long
```

## Alternatives (Without RTT)

If RTT is not available, you can use:

1. **UART/Serial printf** - Slower, but works without debugger
   ```c
   UART_printf("Message\r\n");
   ```

2. **LED Blinking** - For status signaling
   ```c
   if (error) LED_toggle(RED);
   ```

3. **CAN bus** - For communication with master
   ```c
   CAN_send_message(&msg);
   ```

But **RTT is recommended** - fastest and most flexible.
