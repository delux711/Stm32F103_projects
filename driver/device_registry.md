# Device Registry Pattern

## Overview
Flexible mechanism to register sensors/drivers without hardcoding MODBUS logic.
Maps sensor names and read/write functions to MODBUS register addresses.

## Architecture
Instead of hardcoding 150 lines of MODBUS FC3/FC16 handlers per app, define:

```c
// apps/test_adc_light/main.c
static uint16_t read_adc_raw(void) { return ADC_readRaw(); }
static uint16_t read_adc_mv(void)  { return ADC_readMilliVolts(); }

static const RegistryEntry_t registry[] = {
    { 0, "ADC_RAW",  REGISTRY_TYPE_RO, read_adc_raw,  NULL },
    { 1, "ADC_MV",   REGISTRY_TYPE_RO, read_adc_mv,   NULL },
    { 2, "TEMP_C10", REGISTRY_TYPE_RO, read_temp_c10, NULL },
    REGISTRY_END
};

int main(void)
{
    // ...
    DeviceRegistry_init(registry);
    ModbusSlave_init(&mb_cfg, device_cfg->modbus_slave_addr);
}
```

**MODBUS master now reads**:
- Register 0 → 0xADC_raw value
- Register 1 → ADC voltage in mV
- Register 2 → Temperature in C×10

## Structure

```c
typedef struct
{
    uint16_t address;           // MODBUS register address
    const char *name;           // Descriptive name (for logging)
    RegistryType_t type;        // RO or RW
    DeviceRegistry_readFunc_t read;   // Read function (returns uint16_t)
    DeviceRegistry_writeFunc_t write; // Write function (NULL for RO)
} RegistryEntry_t;
```

## API

### `void DeviceRegistry_init(const RegistryEntry_t *entries)`
Initialize registry with sensor/register table.

**Parameters**:
- `entries`: Array of RegistryEntry_t, terminated with REGISTRY_END macro

**Example**:
```c
static const RegistryEntry_t device_sensors[] = {
    { 0, "Light", REGISTRY_TYPE_RO, read_light_level, NULL },
    { 1, "Setpoint", REGISTRY_TYPE_RW, read_setpoint, write_setpoint },
    REGISTRY_END
};

DeviceRegistry_init(device_sensors);
```

### `const RegistryEntry_t *DeviceRegistry_findByAddress(uint16_t addr)`
Look up registry entry by MODBUS address.

**Returns**: Pointer to entry, or NULL if not found

### `uint16_t DeviceRegistry_read(uint16_t addr)`
Read value from registry entry.

**Parameters**:
- `addr`: MODBUS register address

**Returns**: 16-bit sensor value

**Example**:
```c
uint16_t light_val = DeviceRegistry_read(0);  // Reads light level
```

### `void DeviceRegistry_write(uint16_t addr, uint16_t value)`
Write value to registry entry (RW only).

**Parameters**:
- `addr`: MODBUS register address
- `value`: 16-bit value to write

**Example**:
```c
DeviceRegistry_write(1, 2500);  // Sets setpoint to 2500mV
```

## Benefits

1. **No Boilerplate**: Each new app just defines its registry, no MODBUS copy-paste
2. **Flexible**: Add/remove sensors by editing registry array only
3. **Type-Safe**: Read/write functions ensure proper data handling
4. **Scalable**: 15-node system uses same pattern for each node
5. **Debugging**: Sensor names printed in debug output

## Example: Multi-Sensor Device

```c
// Device with light, temp, and button
static RegistryEntry_t light_registry[] = {
    { 0, "LightRaw", REGISTRY_TYPE_RO, ADC_readRaw, NULL },
    { 1, "LightMV", REGISTRY_TYPE_RO, ADC_readMilliVolts, NULL },
    { 2, "TempC10", REGISTRY_TYPE_RO, read_internal_temp, NULL },
    REGISTRY_END
};

static RegistryEntry_t button_registry[] = {
    { 0, "ButtonState", REGISTRY_TYPE_RO, button_read_state, NULL },
    { 1, "ClickCount", REGISTRY_TYPE_RO, button_read_clicks, NULL },
    REGISTRY_END
};

// App selects which registry based on device_type
if (cfg->device_type & 0x01) {
    DeviceRegistry_init(light_registry);
} else {
    DeviceRegistry_init(button_registry);
}
```

## Integration Flow

```
MODBUS Master sends FC3 (Read Regs) for addr 0x0002
         ↓
ModbusSlave_init() set read callback
         ↓
Callback calls DeviceRegistry_read(0x0002)
         ↓
Registry finds entry for addr 0x0002
         ↓
Calls read function pointer (e.g., read_internal_temp())
         ↓
Returns 253 (25.3°C)
         ↓
ModbusSlave wraps in MODBUS FC3 response
         ↓
Sends back to master
```
