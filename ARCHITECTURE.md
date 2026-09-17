# STM32F103 MODBUS IoT Architecture (15-Node System)

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32 Master                             │
│              (MODBUS RTU Master over RS485/UART)               │
│                  - Device discovery                             │
│                  - Sensor polling                               │
│                  - Configuration management                     │
└────────────────────────────────────────────────────────────────┘
                              │
                              │ RS485/UART
                              │
         ┌────────────────────┼────────────────────┐
         │                    │                    │
    ┌────▼─────┐         ┌────▼─────┐         ┌────▼─────┐
    │ STM32 #1  │ ... ... │ STM32 #N  │         │STM32 #15 │
    │ (Addr: 1) │         │ (Addr:N)  │         │(Addr:15) │
    │ ADC Light │         │ Temperature         │ RS485    │
    │ DS18B20   │         │ Button Count        │ Relay    │
    │ RF433     │         │ Motion PIR          │ Counter  │
    └───────────┘         └───────────┘         └──────────┘
      EEPROM                EEPROM              EEPROM
      Config #1             Config #N           Config #15
```

## Architecture Components

### 1. EEPROM Driver (`driver/eeprom.h/c`)
**Purpose**: Persistent storage for device configuration

- **Location**: Last 4 KB of 64 KB FLASH (0x0800F000–0x08010000)
- **Functionality**:
  - FLASH-based EEPROM emulation
  - Word-aligned 16-bit writes
  - Page erase capability
  - CRC not yet implemented (can be added)

**Interface**:
```c
EEPROM_read(offset, data, len)    // Read from EEPROM
EEPROM_write(offset, data, len)   // Write to EEPROM
EEPROM_erase(offset, len)         // Erase EEPROM section
```

### 2. Device Configuration/ID System (`driver/device_config.h/c`)
**Purpose**: Unique device identification and MODBUS configuration

**Stored in EEPROM**:
```c
typedef struct {
    uint32_t magic;              // 0xDEADBEEF
    uint8_t  device_id;          // Unique device ID (0-255)
    uint8_t  modbus_slave_addr;  // MODBUS RTU slave address (1-247)
    uint8_t  device_type;        // Device type / sensor bitmask
    uint8_t  fw_version;         // Firmware version
    uint32_t hw_serial;          // Hardware serial number
} DeviceConfig_t;
```

**Device Type Bitmask**:
```
Bit 0: ADC Light Sensor     (GL5537)
Bit 1: DS18B20 Temperature  (1-Wire)
Bit 2: Button / Digital In
Bit 3: RF433 Receiver       (433 MHz)
Bit 4: RS485 Interface      (MODBUS RTU)
Bit 5: IR Receiver          (38 kHz)
Bit 6: Relay Output         (GPIO control)
Bit 7: Motion Sensor        (PIR)
```

**Interface**:
```c
DeviceConfig_init()                                    // Initialize
DeviceConfig_get()                                     // Read current config
DeviceConfig_setDefaults(id, addr, type)             // Set defaults
DeviceConfig_loadFromEEPROM()                         // Load from storage
DeviceConfig_saveToEEPROM()                           // Save to storage
```

### 3. Device Registry Pattern (`driver/device_registry.h/c`)
**Purpose**: Flexible sensor-to-register mapping without hardcoding

**Each app defines a registry array**:
```c
static const RegistryEntry_t device_sensors[] = {
    { 0,    "DEVICE_ID",    REGISTRY_TYPE_RO, read_device_id,  NULL },
    { 1,    "FW_VERSION",   REGISTRY_TYPE_RO, read_fw_version, NULL },
    { 10,   "LIGHT_RAW",    REGISTRY_TYPE_RO, adc_read_raw,    NULL },
    { 11,   "LIGHT_MV",     REGISTRY_TYPE_RO, adc_read_mv,     NULL },
    { 20,   "TEMP_C10",     REGISTRY_TYPE_RO, read_temp,       NULL },
    { 30,   "SETPOINT",     REGISTRY_TYPE_RW, read_sp, write_sp },
    REGISTRY_END
};

DeviceRegistry_init(device_sensors);
```

**MODBUS Master can now**:
- Read any register via FC3 (Read Holding Registers)
- Write any RW register via FC16 (Write Multiple Registers)
- Registry auto-maps to MODBUS addresses

**Benefits**:
- No duplicate MODBUS code per app
- Add sensors = just update registry array
- Scales to 15 heterogeneous devices

### 4. MODBUS Slave Adapter (`driver/modbus_slave.h/c`)
**Purpose**: Auto-map Device Registry to MODBUS protocol

**Integration**:
```c
// Initialize with device config MODBUS address
ModbusSlave_init(&mb_config, cfg->modbus_slave_addr);

// Main loop
while (1) {
    ModbusSlave_process();
}
```

**Automatically handles**:
- MODBUS FC3 (Read Holding Registers) → calls DeviceRegistry_read()
- MODBUS FC16 (Write Multiple Registers) → calls DeviceRegistry_write()
- Response frame generation and sending

**Optional custom handlers** for advanced logic:
```c
ModbusSlave_setRegisterReadHandler(my_custom_read);
ModbusSlave_setRegisterWriteHandler(my_custom_write);
```

## Bootloader Architecture

### FLASH Memory Layout (64 KB)
```
0x08000000 ┌─────────────────┐
           │ Bootloader      │  4 KB (fixed, non-updateable)
           ├─────────────────┤ 0x08001000
           │ Application     │  56 KB (updateable)
           │ (linked at      │
           │  0x08001000)    │
           ├─────────────────┤ 0x0800F000
           │ Config/EEPROM   │  4 KB (persistent)
           │ (linked at      │
           │  0x0800F000)    │
           └─────────────────┘ 0x08010000 (end)
```

### Bootloader Responsibilities
1. Validate application at 0x08001000 (magic number / CRC)
2. Check firmware version compatibility
3. Relocate vector table to app location
4. Jump to application reset handler

### Application Linker Script (`stm32f103c8_flash_app.ld`)
```
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08001000, LENGTH = 56K  // Offset by bootloader
  RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 20K
}
```

### Vector Table Relocation (in app `startup_gcc.c`)
```c
#define VECT_TAB_OFFSET (0x1000U)  // 4 KB bootloader offset

void SystemInit(void)
{
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
    // ...
}
```

## Typical Device Implementation

### Example: Light Sensor + Temperature Node

**Hardware**:
- ADC on PA0 (GL5537 photoresistor)
- DS18B20 on PA4 (1-Wire)
- UART1 for MODBUS (PA9/PA10)

**Code**:
```c
#include "device_config.h"
#include "device_registry.h"
#include "modbus_slave.h"
#include "adc.h"
#include "ds18b20.h"

// Registry: map sensors to MODBUS addresses
static const RegistryEntry_t sensors[] = {
    { 0,  "DEVICE_ID",    REGISTRY_TYPE_RO, read_device_id,    NULL },
    { 1,  "FW_VERSION",   REGISTRY_TYPE_RO, read_fw_version,   NULL },
    { 10, "LIGHT_RAW",    REGISTRY_TYPE_RO, ADC_readRaw,       NULL },
    { 11, "LIGHT_MV",     REGISTRY_TYPE_RO, ADC_readMilliVolts,NULL },
    { 20, "TEMP_C10",     REGISTRY_TYPE_RO, DS18B20_readTempC10,NULL },
    REGISTRY_END
};

int main(void)
{
    SystemInit();
    SCB->VTOR = FLASH_BASE | 0x1000U;

    DEBUG_initTrace(SystemCoreClock);

    DeviceConfig_init();  // Load config from EEPROM
    ADC_init(&adc_cfg);
    DS18B20_init(&onewire_cfg);

    DeviceRegistry_init(sensors);

    const MODBUS_RTU_config_t mb_cfg = {
        .interframe_timeout_ms = 100,
        .max_registers_per_request = 100,
    };

    DeviceConfig_t *cfg = DeviceConfig_get();
    ModbusSlave_init(&mb_cfg, cfg->modbus_slave_addr);

    while (1) {
        ModbusSlave_process();
    }
}
```

## Multi-Device System Workflow

### Deployment (15 Nodes)
1. **Single bootloader binary** deployed to all nodes (4 KB FLASH)
2. **Single app binary** deployed to all nodes (56 KB FLASH)
3. **Unique config per device** stored in each node's EEPROM

### Configuration Phase
```c
// Factory configuration (per device):
DeviceConfig_t cfg = {
    .magic = 0xDEADBEEF,
    .device_id = 5,              // Unique per node
    .modbus_slave_addr = 10,     // Unique per node
    .device_type = 0x03,         // ADC + DS18B20
    .fw_version = 1,
    .hw_serial = 0x12345678
};
EEPROM_write(0, (uint8_t *)&cfg, sizeof(cfg));
```

### Runtime Operation
**ESP32 Master**:
```c
// Discover all devices
for (addr = 1; addr <= 15; addr++) {
    ModbusRead_HoldingReg(addr, 0, &device_id);  // Get DEVICE_ID
    printf("Found device %d at MODBUS addr %d\n", device_id, addr);
}

// Poll sensors
while (1) {
    for (each_device) {
        ModbusRead_HoldingReg(addr, 10, &light_raw);
        ModbusRead_HoldingReg(addr, 11, &light_mv);
        ModbusRead_HoldingReg(addr, 20, &temp_c10);
        // Process data
    }
}
```

**Each STM32F103 Slave**:
```c
// Loads config from EEPROM
// Sets MODBUS address to cfg->modbus_slave_addr
// Waits for FC3/FC16 from master
// Responds with register values from registry
```

## Feature Extensibility

### Adding a New Sensor Type
1. Create driver: `driver/my_sensor.h/c`
2. Add registry entry in device app:
   ```c
   { 40, "MY_SENSOR", REGISTRY_TYPE_RO, my_sensor_read, NULL }
   ```
3. Update `device_type` bitmask if feature optional
4. Recompile app (bootloader unchanged)

### Adding a New Function Code (e.g., FC4 Input Registers)
1. Extend `modbus_slave.c` with new handler
2. Map to additional registry table or custom callback
3. No breaking changes to existing registry

### Configuration Over MODBUS
1. Designate special register range (e.g., 0x100–0x1FF) for config
2. Implement custom write handler:
   ```c
   if (addr >= 0x100 && addr < 0x200) {
       DeviceConfig_t *cfg = DeviceConfig_get();
       cfg->modbus_slave_addr = value;
       DeviceConfig_saveToEEPROM();
       // Restart device to take effect
   }
   ```

## Summary: Architecture Pillars

| Component | Purpose | Scalability |
|-----------|---------|-------------|
| **EEPROM** | Persistent config storage | Fixed 4 KB (enough for all 15 devices separately) |
| **DeviceConfig** | Unique ID + MODBUS addr | Per-device: 12 bytes in EEPROM |
| **DeviceRegistry** | Sensor-to-register mapping | Per-app: registry array (extensible) |
| **ModbusSlave** | Protocol adapter | Protocol-agnostic: works with any registry |
| **Bootloader** | Non-updateable boot code | Single copy for all 15 nodes |
| **Application** | User logic | Single binary, config distinguishes behavior |

This design enables:
✅ **Homogeneity**: All nodes run same bootloader + app binaries
✅ **Heterogeneity**: Each node's config/registry makes it unique
✅ **Scalability**: 15+ nodes with minimal code duplication
✅ **Flexibility**: Sensors added without touching MODBUS code
✅ **Persistence**: Calibration, thresholds, addresses survive power-down
✅ **OTA Updates**: Bootloader validates app binary before jump
