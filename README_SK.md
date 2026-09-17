# STM32F103 MODBUS IoT Systém - Dokumentácia

**Jazyk**: [🇸🇰 Slovenčina (SK)](#sk-slovenčina) | [🇬🇧 English (EN)](#en-english)

---

## 🇸🇰 Slovenčina

# STM32F103 MODBUS IoT Systém - Úvod

Komplexný systém na vytvorenie distribuovaného MODBUS IoT ekosystému s 15 nezávislými STM32F103 uzlami komunikujúcimi s ESP32 master cez MODBUS RTU.

## 📦 Čo Je Zahrnuté

### Štyri Kľúčové Komponenty:

1. **EEPROM Driver** (`driver/eeprom_SK.md`)
   - Perzistentné úložisko na poslednú 4 KB FLASH
   - Konfigurácia zariadenia (ID, MODBUS adresa, verzia FW)

2. **Device Configuration System** (`driver/device_config_SK.md`)
   - Unikátna identifikácia zariadenia
   - MODBUS slave adresa na zariadenie
   - Uloženie v EEPROM

3. **Device Registry Pattern** (`driver/device_registry_SK.md`)
   - Flexibilné mapovanie senzorů na MODBUS registery
   - Žiadny duplikovaný kód na aplikáciu
   - Ľahké pridávanie nových senzorů

4. **MODBUS Slave Adapter** (`driver/modbus_slave_SK.md`)
   - Automatické mapovanie DeviceRegistry → MODBUS FC3/FC16
   - Podpora read/write operácií
   - Bez hardcodeingu logiky

### Doplnkové:

- **ADC Ovládač** (`driver/adc_SK.md`) - GL5537 fotoresistor + interný teplotný senzor
- **Bootloader Architektúra** (`BOOTLOADER_DESIGN_SK.md`) - FLASH rozdelenie, OTA updaty
- **Testovacia Aplikácia** (`apps/test_modbus_slave/`) - Kompletnú integráciu

## 🚀 Rýchly Štart

### 1. Prečítaj Dokumentáciu

**Nováčikovi**:
- Začni s [`ARCHITECTURE_SK.md`](ARCHITECTURE_SK.md)
- Potom čítaj [`BOOTLOADER_DESIGN_SK.md`](BOOTLOADER_DESIGN_SK.md)

**Vývoj**:
- [`driver/device_registry_SK.md`](driver/device_registry_SK.md) - Ako definovať registry
- [`driver/modbus_slave_SK.md`](driver/modbus_slave_SK.md) - Ako inicializovať MODBUS

**Nasadenie**:
- [`IMPLEMENTATION_SUMMARY_SK.md`](IMPLEMENTATION_SUMMARY_SK.md) - Deployment guide

### 2. Skontroluj Príklady

```bash
# Prečítaj príklad aplikácie
cat apps/test_modbus_slave/main.c

# Prečítaj príklad registry
grep -A 20 "static const RegistryEntry_t" apps/test_modbus_slave/main.c
```

### 3. Skompiluj

```bash
# Build test_modbus_slave (FLASH recommended)
make APP=test_modbus_slave f

# Výstup: build/test_modbus_slave_flash/test_modbus_slave.elf
```

### 4. Flashuj

```bash
# Bootloader
openocd -c "program bootloader.elf 0x08000000 verify"

# Aplikácia
openocd -c "program build/test_modbus_slave_flash/test_modbus_slave.elf 0x08001000 verify"
```

## 📚 Dokumentácia

### Slovenské Verzie (SK)
| Súbor | Obsah |
|-------|-------|
| [`ARCHITECTURE_SK.md`](ARCHITECTURE_SK.md) | Celý systémový prehľad a návrh |
| [`BOOTLOADER_DESIGN_SK.md`](BOOTLOADER_DESIGN_SK.md) | Bootloader, FLASH rozdelenie, OTA |
| [`IMPLEMENTATION_SUMMARY_SK.md`](IMPLEMENTATION_SUMMARY_SK.md) | Súhrn komponentov a deploy guide |
| [`driver/eeprom_SK.md`](driver/eeprom_SK.md) | EEPROM API |
| [`driver/device_config_SK.md`](driver/device_config_SK.md) | Device config API |
| [`driver/device_registry_SK.md`](driver/device_registry_SK.md) | Registry vzor API |
| [`driver/modbus_slave_SK.md`](driver/modbus_slave_SK.md) | MODBUS slave API |
| [`driver/adc_SK.md`](driver/adc_SK.md) | ADC ovládač API |

### Anglické Verzie (EN)
| Súbor | Obsah |
|-------|-------|
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | System architecture and design |
| [`BOOTLOADER_DESIGN.md`](BOOTLOADER_DESIGN.md) | Bootloader, FLASH layout, OTA |
| [`IMPLEMENTATION_SUMMARY.md`](IMPLEMENTATION_SUMMARY.md) | Component summary and deploy guide |
| [`driver/eeprom.md`](driver/eeprom.md) | EEPROM API |
| [`driver/device_config.md`](driver/device_config.md) | Device config API |
| [`driver/device_registry.md`](driver/device_registry.md) | Registry pattern API |
| [`driver/modbus_slave.md`](driver/modbus_slave.md) | MODBUS slave API |
| [`driver/adc.md`](driver/adc.md) | ADC driver API |

## 💡 Príklady Použitia

### Príklad 1: Jednoduché Čítanie Senzora

```c
#include "device_registry.h"
#include "modbus_slave.h"

// 1. Definuj registry
static uint16_t read_temp(void) {
    return DS18B20_readTempC10();
}

static const RegistryEntry_t my_registry[] = {
    { 0, "TEMP_C10", REGISTRY_TYPE_RO, read_temp, NULL },
    REGISTRY_END
};

// 2. Inicializuj
int main(void) {
    SystemInit();
    DeviceConfig_init();
    DeviceRegistry_init(my_registry);

    MODBUS_RTU_config_t cfg = { .interframe_timeout_ms = 100 };
    ModbusSlave_init(&cfg, 10);  // Slave addr 10

    while (1) {
        ModbusSlave_process();
    }
}

// 3. MODBUS master čita: ReadReg(addr=10, reg=0) → dostane teplotu
```

### Príklad 2: Read-Write Register (Setpoint)

```c
static uint16_t setpoint = 2500;

static uint16_t read_setpoint(void) {
    return setpoint;
}

static void write_setpoint(uint16_t value) {
    setpoint = value;
    // Ulož do EEPROM
    DeviceConfig_t *cfg = DeviceConfig_get();
    cfg->modbus_slave_addr = value;
    DeviceConfig_saveToEEPROM();
}

static const RegistryEntry_t registry[] = {
    { 0, "SETPOINT", REGISTRY_TYPE_RW, read_setpoint, write_setpoint },
    REGISTRY_END
};
```

### Príklad 3: Viacerí Senzori na Jednom Uzle

```c
static const RegistryEntry_t multi_sensor_registry[] = {
    // Svetlo
    { 0, "LIGHT_RAW",  REGISTRY_TYPE_RO, ADC_readRaw,          NULL },
    { 1, "LIGHT_MV",   REGISTRY_TYPE_RO, ADC_readMilliVolts,   NULL },

    // Teplota
    { 10, "TEMP_C10",  REGISTRY_TYPE_RO, read_internal_temp,   NULL },

    // Konfigurácia
    { 20, "DEVICE_ID", REGISTRY_TYPE_RO, read_device_id,       NULL },
    { 21, "FW_VER",    REGISTRY_TYPE_RO, read_fw_version,      NULL },

    REGISTRY_END
};
```

## 🔗 Nasadenie 15-Uzlov

1. **Príprava** (na factory):
   - Každému STM32F103 prirad unikátny device_id (1–15)
   - Prirad unikátnu MODBUS adresu (1–15)
   - Ulož do EEPROM (DeviceConfig)

2. **Build**:
   - Jeden bootloader binár
   - Jeden app binár

3. **Flash**:
   - Aplikuj na všetkých 15 uzloch
   - Bootloader → 0x08000000
   - App → 0x08001000

4. **Runtime**:
   - ESP32 master objaví uzly cez DeviceRegistry
   - Master čita/píše do registrov podľa potreby
   - Každý uzol má svoju konfiguráciu v EEPROM

## ✨ Klíčové Výhody

| Výhoda | Popis |
|--------|-------|
| **Bez Duplikácie** | Jeden app binár pre všetky uzly |
| **Flexibilný** | Registry vzor → žiadny hardcoding |
| **Scalable** | Ľahko pridať nové senzory |
| **Perzistentný** | Konfigurácia prežije restart |
| **Bezpečný** | Bootloader fixný, app updateable |

## 🐛 Troubleshooting

### Debug: Ako Vidieť Hlášky cez RTT

V aplikáciách sa nepoužíva `printf()` (nepodporovaný bez libc). Namiesto toho:

1. **Definuj helper funkciu** - Pozri [`driver/debug_rtt_SK.md`](driver/debug_rtt_SK.md)
   ```c
   static void rtt_printf(const char *format, ...)
   {
       static char buffer[256];
       va_list args;
       va_start(args, format);
       vsnprintf(buffer, sizeof(buffer), format, args);
       va_end(args);
       DEBUG_writeString(buffer);
   }
   ```

2. **Použi v main()**
   ```c
   rtt_printf("Device ID: %d\r\n", cfg->device_id);
   rtt_printf("Counter: %lu\r\n", counter);
   ```

3. **Pozri výstup** - Cez SEGGER Ozone: `Window` → `Real-Time Terminal`

Príklad: [`apps/test_modbus_slave/main.c`](apps/test_modbus_slave/main.c)

### Build Error: "RAM overflowed"

Keď zostaviť `test_modbus_slave` s RAM (`make APP=test_modbus_slave r`):
- RAM je 4 KB, ale potrebuje ~4.7 KB
- **Riešenie**: Použiť FLASH build: `make APP=test_modbus_slave f`

### MODBUS Master nedostáva odpoveď

- Skontroluj, či je slave adresa v `DeviceConfig_get()->modbus_slave_addr` správna
- Skontroluj `DeviceRegistry_init()` je volaný pred `ModbusSlave_init()`
- Skontroluj fyzické RS485 zapojenie

### Zmena konfigurácie sa neuloží po reštarte

- Zavolaj `DeviceConfig_saveToEEPROM()` po zmene
- Skontroluj, či EEPROM zápis vrátil `EEPROM_OK`

## 📞 Pomoc

- Čítaj [`ARCHITECTURE_SK.md`](ARCHITECTURE_SK.md) pre prehlad
- Čítaj príslušný `*_SK.md` súbor pre detaily
- Pozri príklad v [`apps/test_modbus_slave/main.c`](apps/test_modbus_slave/main.c)

---

## 🇬🇧 English

# STM32F103 MODBUS IoT System - Introduction

A comprehensive system for creating a distributed MODBUS IoT ecosystem with 15 independent STM32F103 nodes communicating with an ESP32 master via MODBUS RTU.

## 📦 What's Included

### Four Core Components:

1. **EEPROM Driver** (`driver/eeprom.md`)
   - Persistent storage in last 4 KB of FLASH
   - Device configuration (ID, MODBUS address, FW version)

2. **Device Configuration System** (`driver/device_config.md`)
   - Unique device identification
   - MODBUS slave address per device
   - Storage in EEPROM

3. **Device Registry Pattern** (`driver/device_registry.md`)
   - Flexible sensor-to-MODBUS register mapping
   - No code duplication per application
   - Easy to add new sensors

4. **MODBUS Slave Adapter** (`driver/modbus_slave.md`)
   - Automatic mapping of DeviceRegistry → MODBUS FC3/FC16
   - Support for read/write operations
   - No hardcoded logic

### Additional:

- **ADC Driver** (`driver/adc.md`) - GL5537 light sensor + internal temp sensor
- **Bootloader Architecture** (`BOOTLOADER_DESIGN.md`) - FLASH layout, OTA updates
- **Test Application** (`apps/test_modbus_slave/`) - Full integration example

## 🚀 Quick Start

### 1. Read Documentation

**New users**:
- Start with [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Then read [`BOOTLOADER_DESIGN.md`](BOOTLOADER_DESIGN.md)

**Development**:
- [`driver/device_registry.md`](driver/device_registry.md) - How to define registry
- [`driver/modbus_slave.md`](driver/modbus_slave.md) - How to initialize MODBUS

**Deployment**:
- [`IMPLEMENTATION_SUMMARY.md`](IMPLEMENTATION_SUMMARY.md) - Deployment guide

### 2. Check Examples

```bash
# Read example application
cat apps/test_modbus_slave/main.c

# Check registry example
grep -A 20 "static const RegistryEntry_t" apps/test_modbus_slave/main.c
```

### 3. Compile

```bash
# Build test_modbus_slave (FLASH recommended)
make APP=test_modbus_slave f

# Output: build/test_modbus_slave_flash/test_modbus_slave.elf
```

### 4. Flash

```bash
# Bootloader
openocd -c "program bootloader.elf 0x08000000 verify"

# Application
openocd -c "program build/test_modbus_slave_flash/test_modbus_slave.elf 0x08001000 verify"
```

## 📚 Documentation

See SK (Slovenčina) section above for full documentation list in both languages.

---

**Created**: June 2026 | **Target**: STM32F103C8 | **Compiler**: GCC arm-none-eabi 11.3.1
