# MODBUS Slave Adapter

## Overview
Wrapper around MODBUS RTU driver that auto-maps Device Registry to MODBUS function codes 3 (Read Holding Registers) and 16 (Write Multiple Registers).

## Design
Instead of writing MODBUS handlers for each app, `ModbusSlave_*` provides:
1. Simple initialization with registry-based callbacks
2. Automatic FC3/FC16 handling via device registry
3. Optional custom read/write handlers for advanced scenarios

## Architecture

```
UART RX → MODBUS_RTU_process()
            ↓ (calls registered callback)
         ModbusSlave_readHoldingReg()
            ↓
         DeviceRegistry_read(address)
            ↓
         Read function pointer
            ↓
         Return 16-bit value → MODBUS_RTU_process() → TX response
```

## API

### `void ModbusSlave_init(const MODBUS_RTU_config_t *config, uint8_t slave_addr)`
Initialize MODBUS slave with registry-based callbacks.

**Parameters**:
- `config`: MODBUS_RTU_config_t with interframe timeout, max registers
- `slave_addr`: MODBUS slave address (1-247)

**Example**:
```c
MODBUS_RTU_config_t mb_cfg = {
    .interframe_timeout_ms = 100,
    .max_registers_per_request = 100
};
ModbusSlave_init(&mb_cfg, 10);  // Slave address 10
```

### `void ModbusSlave_setRegisterReadHandler(ModbusSlave_regGetFunc_t handler)`
Set custom read handler (optional, registry is used by default).

**Parameters**:
- `handler`: Callback function with signature `int handler(uint16_t addr, uint16_t *value)`

### `void ModbusSlave_setRegisterWriteHandler(ModbusSlave_regSetFunc_t handler)`
Set custom write handler (optional, registry is used by default).

**Parameters**:
- `handler`: Callback function with signature `void handler(uint16_t addr, uint16_t value)`

### `void ModbusSlave_process(void)`
Process pending MODBUS requests. Call from main loop.

**Example**:
```c
while (1) {
    ModbusSlave_process();
    HAL_Delay(1);
}
```

## Typical Usage

**For simple apps (registry-only)**:
```c
int main(void)
{
    SystemInit();
    DeviceConfig_init();

    // Define registry
    static const RegistryEntry_t my_registry[] = {
        { 0, "Sensor1", REGISTRY_TYPE_RO, read_sensor1, NULL },
        { 1, "Sensor2", REGISTRY_TYPE_RO, read_sensor2, NULL },
        REGISTRY_END
    };

    DeviceRegistry_init(my_registry);

    // Initialize MODBUS
    MODBUS_RTU_config_t mb_cfg = { .interframe_timeout_ms = 100, .max_registers_per_request = 100 };
    ModbusSlave_init(&mb_cfg, 10);  // Use device config later: DeviceConfig_get()->modbus_slave_addr

    // Main loop
    while (1) {
        ModbusSlave_process();
    }
}
```

**For advanced apps (custom handlers)**:
```c
static int my_custom_read(uint16_t addr, uint16_t *value)
{
    // Custom logic per register
    if (addr == 100) {
        *value = get_calibration_factor();
    } else {
        return DeviceRegistry_read(addr);  // Fall through to registry
    }
    return 0;
}

int main(void)
{
    // ... setup registry ...
    ModbusSlave_init(&mb_cfg, 10);
    ModbusSlave_setRegisterReadHandler(my_custom_read);
}
```

## MODBUS Function Codes Supported

### FC3: Read Holding Registers
- Requests up to 125 registers at once
- Each register is 16 bits
- Device registry or custom handler provides values

**Example MODBUS frame**:
```
Request:  [10] [03] [0000] [0002] CRC  → Read 2 registers from addr 0
Response: [10] [03] [04] [0xHH] [0xLL] [0xHH] [0xLL] CRC
```

### FC16: Write Multiple Registers
- Writes up to 123 registers at once
- Calls write handler for each address
- Returns echo of request plus CRC

**Example MODBUS frame**:
```
Request:  [10] [10] [0064] [0001] [02] [0x1234] CRC  → Write 1 register to addr 0x64
Response: [10] [10] [0064] [0001] CRC
```

## Integration with Device Config

**At startup, use device config MODBUS address**:
```c
DeviceConfig_init();
DeviceConfig_t *cfg = DeviceConfig_get();
ModbusSlave_init(&mb_cfg, cfg->modbus_slave_addr);
```

**Runtime MODBUS address change**:
```c
// Master writes to special register (e.g., addr 0xF0) to change slave address
// App reads device_config, updates it, saves to EEPROM:
cfg->modbus_slave_addr = new_addr;
DeviceConfig_saveToEEPROM();
// Restart device to take effect
```

## Error Handling
- Invalid register addresses return 0 (or custom handler decides)
- RO registers reject writes silently
- Malformed MODBUS frames are ignored (CRC check in MODBUS_RTU)
