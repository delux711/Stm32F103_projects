# MODBUS Slave Test Application

## Overview
Demonstrates integration of Device Configuration, Device Registry, and MODBUS Slave adapter.

Device exposes 4 registers via MODBUS RTU:
- **Register 0**: Device ID (read-only)
- **Register 1**: Firmware Version (read-only)
- **Register 10**: Counter (read-write)
- **Register 11**: Counter High (read-only, for 32-bit value)

## Hardware
- STM32F103C8 + RS485 transceiver (e.g. MAX3485)
- USART1 remapped to:
  - TX: PB6
  - RX: PB7
  - DE/RE (direction): PB8
- Baudrate: 9600 8N1

Connect USB-RS485 master to A/B lines and use slave address from EEPROM (default `1`).

## EEPROM Configuration
Device reads from EEPROM on startup:
- **Default Device ID**: 0
- **Default MODBUS Slave Address**: 1
- **Default Device Type**: 0
- **FW Version**: 1

To modify:
```c
DeviceConfig_t *cfg = DeviceConfig_get();
cfg->device_id = 5;
cfg->modbus_slave_addr = 10;
DeviceConfig_saveToEEPROM();
```

## Registry Pattern
Shows how to:
1. Define read-only sensor values
2. Define read-write configuration registers
3. Terminate registry with `REGISTRY_END`
4. Automatically map to MODBUS FC3 (Read) and FC16 (Write)

## Build
```bash
# RAM build (for debugging)
make APP=test_modbus_slave r

# FLASH build (recommended)
make APP=test_modbus_slave f
```

## Running
Expected output on RTT/UART:
```
=== MODBUS Slave (Device Registry) ===
Device ID: 0
MODBUS Addr: 1
Device Type: 0x00
FW Version: 1
Ready - waiting for MODBUS requests
Counter: 1
Counter: 2
...
```

## Quick Start (just run)
1. Build and flash:
```bash
make APP=test_modbus_slave f
```
2. Start Modbus master tool (QModMaster / Modbus Poll / mbpoll):
  - RTU, 9600, 8N1
  - Slave ID: `1`
3. Read register `10` (FC3) repeatedly and watch value increment every second.

## MODBUS Testing
Using Modbus master simulator:
```
Read Register 0 (Device ID):
  Request:  [01][03][0000][0001] CRC
  Response: [01][03][02][0000] CRC

Read Register 10 (Counter):
  Request:  [01][03][000A][0001] CRC
  Response: [01][03][02][xxxx] CRC  (value increments each second)

Write Register 10 (Counter):
  Request:  [01][10][000A][0001][02][1234] CRC
  Response: [01][10][000A][0001] CRC
```

## Integration with 15-Node System
1. Flash identical application to all 15 STM32F103 nodes
2. Configure each with unique device ID + MODBUS address in EEPROM
3. Each node exposes registers according to its registry
4. ESP32 master can discover all nodes via register 0 (Device ID)
5. Enable/disable features per device type bitmask (DeviceConfig.device_type)
