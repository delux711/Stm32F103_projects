# Device Configuration/ID System

## Overview
Persistent storage for device identity, firmware versioning, and MODBUS slave address.
Stored in EEPROM (first 16 bytes).

## Structure
```c
typedef struct
{
    uint32_t magic;                    // 0xDEADBEEF
    uint8_t  device_id;                // Unique device ID (0-255)
    uint8_t  modbus_slave_addr;        // MODBUS RTU slave address (1-247)
    uint8_t  device_type;              // Device type bitmask (sensor config)
    uint8_t  fw_version;               // Firmware version
    uint32_t hw_serial;                // Hardware serial number
} DeviceConfig_t;
```

## Magic Number
`DEVICE_CONFIG_MAGIC = 0xDEADBEEF` - validates EEPROM is correctly initialized

## Device Type Bitmask Example
```
Bit 0: ADC Light Sensor (GL5537)
Bit 1: DS18B20 Temperature Sensor
Bit 2: Button Input
Bit 3: RF433 Receiver
Bit 4: RS485 Interface
Bit 5: IR Receiver
Bit 6-7: Reserved
```

## API

### `void DeviceConfig_init(void)`
Initialize device config system and EEPROM.

### `void DeviceConfig_setDefaults(uint8_t device_id, uint8_t modbus_addr, uint8_t dev_type)`
Set default configuration (used if EEPROM is blank).

**Example**:
```c
DeviceConfig_setDefaults(5, 10, 0x03);  // Device 5, MODBUS addr 10, ADC+DS18B20
```

### `DeviceConfig_t *DeviceConfig_get(void)`
Get current device configuration (loads from EEPROM if not already loaded).

**Returns**: Pointer to current config structure

**Example**:
```c
DeviceConfig_t *cfg = DeviceConfig_get();
printf("MODBUS Address: %d\r\n", cfg->modbus_slave_addr);
```

### `int DeviceConfig_loadFromEEPROM(void)`
Load configuration from EEPROM. Returns 0 if valid, -1 if EEPROM is blank (loads defaults).

### `int DeviceConfig_saveToEEPROM(void)`
Save current configuration to EEPROM. Returns 0 on success, -1 on failure.

**Example - Change slave address at runtime**:
```c
DeviceConfig_t *cfg = DeviceConfig_get();
cfg->modbus_slave_addr = 11;
DeviceConfig_saveToEEPROM();
```

## Usage Pattern

**At startup**:
```c
int main(void)
{
    SystemInit();
    DEBUG_initTrace(SystemCoreClock);

    DeviceConfig_init();
    DeviceConfig_t *cfg = DeviceConfig_get();

    printf("Device %d, MODBUS Addr %d\r\n", cfg->device_id, cfg->modbus_slave_addr);

    // ... rest of initialization
}
```

**In multi-device system**:
```c
// Each STM32F103 has unique DeviceConfig in EEPROM
// ESP32 master reads device IDs via MODBUS register 0
// Routes commands to correct slave based on device_id
```

## Integration with Device Registry
Device type bitmask determines which sensors are active:
- Registry reads device_type
- Filters registry entries based on enabled sensors
- Reduces MODBUS register table for device capabilities
