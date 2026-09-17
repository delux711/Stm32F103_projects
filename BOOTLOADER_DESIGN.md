# STM32F103 Bootloader Architecture

## FLASH Memory Layout (64 KB)

```
0x08000000 ┌─────────────────────────┐
           │  BOOTLOADER             │
           │  (Non-updateable)       │
           │                         │
           │  Size: 4 KB – 8 KB      │
           ├─────────────────────────┤ 0x08001000 or 0x08002000
           │  APPLICATION (RW)       │
           │  (Updateable)           │
           │                         │
           │  Size: 48 KB – 52 KB    │
           │                         │
           │  Contains:              │
           │  - User code            │
           │  - Drivers              │
           │  - MODBUS stack         │
           ├─────────────────────────┤ 0x0800F000
           │  CONFIG/EEPROM          │
           │  (Persistent storage)   │
           │                         │
           │  Size: 4 KB             │
           │                         │
           │  Contains:              │
           │  - DeviceConfig         │
           │  - Calibration data     │
           │  - MODBUS settings      │
           └─────────────────────────┘ 0x08010000 (end)
```

## Typical Configuration

### Option 1: 4 KB Bootloader (RECOMMENDED)
- **Bootloader**: 0x08000000 – 0x08001000 (4 KB)
- **Application**: 0x08001000 – 0x0800F000 (56 KB)
- **Config/EEPROM**: 0x0800F000 – 0x08010000 (4 KB)

**Advantages**:
- Maximum space for application
- Simple alignment (page boundaries)

### Option 2: 8 KB Bootloader
- **Bootloader**: 0x08000000 – 0x08002000 (8 KB)
- **Application**: 0x08002000 – 0x0800F000 (52 KB)
- **Config/EEPROM**: 0x0800F000 – 0x08010000 (4 KB)

**Use when**: Bootloader needs advanced features (CRC, versioning, etc.)

## Linker Script Setup

### For Application (Loadable at 0x08001000)

**stm32f103c8_flash_app.ld**:
```linker
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08001000, LENGTH = 56K
  RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
  /* ... regular sections, but FLASH origin is 0x08001000 ... */
}
```

**Key changes from standard**:
- Change `ORIGIN = 0x08001000` (not 0x08000000)
- Reduce `LENGTH = 56K` (not 64K)
- Vector table must be re-located at runtime or fixed in bootloader

### Vector Table Re-location

**In bootloader**:
```c
// stm32f103c8_flash.ld (Bootloader script, keep default 0x08000000)
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 4K
  // ...
}
```

**In application startup_gcc.c**:
```c
#define VECT_TAB_OFFSET  (0x1000U)  // 4 KB bootloader offset

void SystemInit(void)
{
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;  // Relocate vector table
    // ... rest of init
}
```

## Bootloader Responsibilities

1. **Validation**: Check if valid app exists at 0x08001000
2. **Verification**: Optional CRC/signature check of application binary
3. **Version Check**: Compare bootloader version with app FW version
4. **Jump to App**: Set stack pointer and jump to app reset handler
5. **Fallback**: If app invalid, stay in bootloader or enter recovery mode

## Application Responsibilities

1. **Device Config**: Load MODBUS address and device ID from EEPROM
2. **Initialization**: Setup all peripherals
3. **Main Loop**: Run MODBUS slave, sensors, etc.

## Building for Each Component

### Compile Bootloader
```bash
# Modify root Makefile to select bootloader linker script
make APP=bootloader LINKER=stm32f103c8_flash.ld
```

### Compile Application
```bash
# Application uses app-specific linker script with offset
make APP=test_adc_light LINKER=stm32f103c8_flash_app.ld
```

### Flash Procedure
```bash
# Flash bootloader at 0x08000000
openocd -c "program bootloader.elf 0x08000000 verify"

# Flash application at 0x08001000
openocd -c "program app.elf 0x08001000 verify"
```

## Device ID Register (Unique per STM32)

Each STM32F103C8 has a unique 96-bit ID at 0x1FFFF7E0–E0B:
```c
#define UID_BASE  (0x1FFFF7E0)
#define DEVID0    (*(uint32_t *)(UID_BASE + 0x00))
#define DEVID1    (*(uint32_t *)(UID_BASE + 0x04))
#define DEVID2    (*(uint32_t *)(UID_BASE + 0x08))
```

**Use case**:
- Bootloader reads UID at startup
- Compares with stored value in DeviceConfig
- If mismatch, device may be in wrong slot (safety check)

## Integration with Device Config

**DeviceConfig EEPROM location**: Last 4 KB FLASH (0x0800F000)

**At app startup**:
```c
int main(void)
{
    SystemInit();
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;  // Re-locate vectors

    DeviceConfig_init();  // Load from EEPROM
    DeviceConfig_t *cfg = DeviceConfig_get();

    // Use cfg->modbus_slave_addr, cfg->device_id, etc.
}
```

## Multi-Device Update Strategy (15 nodes)

1. **All devices run same bootloader code** (fixed, in first 4 KB)
2. **Each device has unique DeviceConfig in EEPROM**
3. **App binaries can be identical** (config distinguishes them)
4. **Over-the-air update via MODBUS**:
   - ESP32 sends new app binary to each node in chunks
   - Node stores in app section (0x08001000)
   - Node restarts, bootloader validates
   - App starts with device-specific config from EEPROM

## Example Bootloader Entry

```c
// In bootloader (stm32f103c8_flash.ld)
typedef void (*AppFunc_t)(void);

void bootloader_main(void)
{
    // Validate app at 0x08001000
    uint32_t app_vector = *(uint32_t *)(0x08001000);
    if (app_vector == 0xFFFFFFFF || app_vector == 0x00000000) {
        // No valid app, stay in bootloader
        while (1) { /* recovery mode */ }
    }

    // Jump to app
    AppFunc_t jump_to_app = (AppFunc_t)*(uint32_t *)(0x08001004);
    __set_MSP(*(uint32_t *)(0x08001000));
    jump_to_app();
}
```

## Notes for 15-Node System

- **Single bootloader binary** for all nodes (OTA update via bootloader section only if needed)
- **Single app binary** can be used on all nodes (config files differ)
- **Config persistence** via DeviceConfig in last 4 KB EEPROM
- **Slave address assignment** can be:
  - Pre-programmed in factory
  - Assigned dynamically by master on first power-up
  - Modified via MODBUS register write + restart
