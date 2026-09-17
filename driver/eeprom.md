# EEPROM Driver (STM32F103)

## Overview
Emulated EEPROM driver for STM32F103, using last 4KB of FLASH memory (pages 31-32 of 64KB device).

## Configuration
- **Start Address**: 0x08000000 + 60KB = 0x0800F000
- **Size**: 4KB (two 2KB pages)
- **Typical Use**: Device configuration, calibration data, persistent parameters

## API

### `void EEPROM_init(void)`
Initialize EEPROM subsystem. Call once at startup.

### `EEPROM_status_t EEPROM_read(uint16_t offset, uint8_t *data, uint16_t len)`
Read bytes from EEPROM.

**Parameters**:
- `offset`: Byte offset (0 to EEPROM_TOTAL_SIZE-1)
- `data`: Buffer to store read data
- `len`: Number of bytes to read

**Returns**: `EEPROM_OK` on success, error code otherwise

**Example**:
```c
uint8_t config_buf[32];
EEPROM_read(0, config_buf, sizeof(config_buf));
```

### `EEPROM_status_t EEPROM_write(uint16_t offset, const uint8_t *data, uint16_t len)`
Write bytes to EEPROM (word-aligned, 16-bit writes).

**Parameters**:
- `offset`: Byte offset
- `data`: Data to write
- `len`: Number of bytes to write

**Returns**: `EEPROM_OK` on success

**Warning**: Length must be even. Data is written as 16-bit words.

**Example**:
```c
uint8_t config_buf[32] = {...};
EEPROM_write(0, config_buf, sizeof(config_buf));
```

### `EEPROM_status_t EEPROM_erase(uint16_t offset, uint16_t len)`
Erase one or more EEPROM pages (offset must be page-aligned, length must be multiple of 2048).

## Hardware Notes
- FLASH write time: ~30µs per 2 bytes (at 72 MHz)
- Page erase time: ~20ms per page
- Endurance: ~10,000 erase cycles typical per page
- For frequent writes, implement wear-leveling across multiple pages

## Integration
1. Add `#include "eeprom.h"` to app
2. Call `EEPROM_init()` in `main()`
3. Use `EEPROM_read()` / `EEPROM_write()` as needed
