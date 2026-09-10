# RTT Debugging so Úrovňami (DEBUG, INFO, OK, WARN, ERROR)

## Nové: Debug Úrovne

RTT teraz podporuje 5 úrovní debug výstupu s farebnými prefixami:

| Úroveň | Prefix | Použitie | Príklad |
|--------|--------|---------|---------|
| **DEBUG** | `[DEBUG]` | Vývoj. info, trace dát | `rtt_debug("Reg addr=0x%04x\r\n", addr)` |
| **INFO** | `[INFO]` | Normálna operácia | `rtt_info("Counter: %lu\r\n", cnt)` |
| **OK** | `[OK]` | Úspech, inicializácia | `rtt_ok("Device ready\r\n")` |
| **WARN** | `[WARN]` | Varovanie | `rtt_warn("Timeout %u ms\r\n", time)` |
| **ERROR** | `[ERROR]` | Chyba | `rtt_error("Failed: %d\r\n", err)` |

## Header Súbor: `debug_rtt_levels.h`

Nový header so všetkými debug helper funkciami.

**Include:**
```c
#include "debug_rtt_levels.h"
```

**Dostupné Funkcie:**
```c
rtt_printf(format, ...)   // Bez prefixu
rtt_debug(format, ...)    // [DEBUG] prefix
rtt_info(format, ...)     // [INFO] prefix
rtt_ok(format, ...)       // [OK] prefix
rtt_warn(format, ...)     // [WARN] prefix
rtt_error(format, ...)    // [ERROR] prefix
```

## Príklady Použitia

### Inicializácia

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

### Čítanie Dát

```c
while (1) {
    uint16_t temp = ADC_readInternalTempC10();
    rtt_info("Temperature: %d.%d C\r\n", temp/10, temp%10);

    if (temp > 8000) {  // 80°C
        rtt_warn("High temperature: %d C\r\n", temp/10);
    }
}
```

### MODBUS Debug

```c
if (ModbusSlave_init(&cfg, slave_addr) != 0) {
    rtt_error("MODBUS init failed\r\n");
} else {
    rtt_ok("MODBUS slave ready at addr %d\r\n", slave_addr);
}

// V procesingu:
rtt_info("MODBUS request from addr %u\r\n", from_addr);
```

### Buttonové Eventy

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

## RTT Výstup v Ozone

**Očakávaný Výstup:**
```
[OK] System initialized
[OK] Device configured
[INFO] Temperature: 25.3 C
[INFO] MODBUS request from addr 1
[DEBUG] Reg addr=0x0010 value=0x1234
[WARN] High temperature: 80 C
[OK] MODBUS slave ready at addr 1
```

## Aplikácie s Debug Úrovňami

Všetky aplikácie boli aktualizované na používanie debug úrovní:

| Aplikácia | Prvok | Úroveň |
|-----------|-------|--------|
| **test_modbus_slave** | Device ID, Addr | OK |
| **test_modbus_slave** | FW Version | DEBUG |
| **test_modbus_slave** | Ready message | OK |
| **test_modbus_slave** | Counter | INFO |
| **test_adc_light** | Start | OK |
| **test_adc_light** | Sensor data | INFO |
| **test_button** | Click/Press | INFO |
| **test_RS485_modbus** | Ready | OK |
| **driver/adc.c** | Debug info | DEBUG |
| **driver/device_config.c** | Errors | ERROR |

## Výhody Debug Úrovní

1. ✅ **Jasnosť** - Vidieť typ správy na prvý pohľad
2. ✅ **Filterovanie** - Ozone môže filtrovať podľa prefixu
3. ✅ **Konzistencia** - Jednotný formát v celom projekte
4. ✅ **Maintenance** - Ľahko pochopiť kód iných
5. ✅ **Production** - Môžu sa zakázať na Release

## Ako Vidieť Výstup

### SEGGER Ozone

1. Otvoriť debug session
2. `Window` → `Real-Time Terminal`
3. Hlášky sa objavujú s prefixami

### J-Link RTT Client

```bash
JLinkRTTClient
```

Výstup s prefixami sa objavuje v reálnom čase.

### Požadované Include-y

```c
#include <stdio.h>     // Pre vsnprintf
#include <stdarg.h>    // Pre va_list, va_start, va_end
#include "debug.h"     // Pre DEBUG_writeString
```

### Použitie

**Namesto:**
```c
printf("Device ID: %d\r\n", cfg->device_id);
printf("MODBUS Addr: %d\r\n", cfg->modbus_slave_addr);
printf("Counter: %lu\r\n", counter);
```

**Použi:**
```c
rtt_printf("Device ID: %d\r\n", cfg->device_id);
rtt_printf("MODBUS Addr: %d\r\n", cfg->modbus_slave_addr);
rtt_printf("Counter: %lu\r\n", counter);
```

## Ako Vidieť Výstup

### Cez SEGGER Ozone

1. Otvoriť Ozone debug session
2. Ísť do `Window` → `Real-Time Terminal`
3. Spustiť aplikáciu
4. RTT výstup sa objaví v okne

### Cez SEGGER J-Link RTT Client

```bash
# V termináli
JLinkRTTClient

# Očakávaný výstup:
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

Buffer veľkosť `256` bajtov je postačujúca pre väčšinu správ. Možno ju zmeniť:

```c
static char buffer[512];  // Pre dlhšie správy
// alebo
static char buffer[128];  // Pre kompaktnú aplikáciu
```

## Supports Formatting

Helper funkcia podporuje všetky štandardné `printf` formáty:

```c
rtt_printf("Int: %d\r\n", 42);           // Integer
rtt_printf("Hex: 0x%02x\r\n", 0xFF);    // Hexadecimal
rtt_printf("Float: %.2f\r\n", 3.14);    // Float (ak je FPU k dispozícii)
rtt_printf("String: %s\r\n", "Hello");  // String
rtt_printf("Multiple: %d, %s, 0x%x\r\n", 10, "test", 255);
```

## Performance

- **vsnprintf()** - ~100-500 µs (podľa dĺžky formátu)
- **DEBUG_writeString()** - ~50-200 µs (podľa dĺžky stringu)
- **RTT transport** - Prakticky bezlatencia (shared memory)

Bez blokovania hlavného cyklu.

## Príklad: Test Modbus Slave

Úplnú integraci vidíte v `apps/test_modbus_slave/main.c`:

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

### Výstup sa Neobjavuje

- Skontroluj, či `DEBUG_initTrace()` je volaný v `main()`
- Skontroluj, či je RTT povolený v debug konfigurácií (Ozone, KEIL)
- Skontroluj fyzické zapojenie JTAG/SWD

### Buffer Overflow

Ak sa správa skrátila:
```c
// Zväčšiť buffer
static char buffer[512];
```

### Formátovacia Chyba

Ak sa formát nezobrazuje správne:
```c
// Skontroluj korektný formát pre typ dát
rtt_printf("Value: %u\r\n", unsigned_value);  // %u pre unsigned
rtt_printf("Value: %d\r\n", signed_value);    // %d pre signed
rtt_printf("Value: %lu\r\n", unsigned_long);  // %lu pre unsigned long
```

## Alternatívy (Bez RTT)

Ak RTT nie je k dispozícií, môžeš používať:

1. **UART/Serial printf** - Pomalší, ale funguje bez debuggera
   ```c
   UART_printf("Message\r\n");
   ```

2. **LED Blinking** - Na signalizovanie štátu
   ```c
   if (error) LED_toggle(RED);
   ```

3. **CAN bus** - Na komunikáciu s mastrom
   ```c
   CAN_send_message(&msg);
   ```

Ale **RTT je odporúčané** - najrýchlejší a najflexibilnejší.
