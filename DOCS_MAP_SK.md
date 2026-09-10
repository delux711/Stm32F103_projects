# 📚 Mapa Dokumentácie STM32F103 MODBUS IoT

## 🇸🇰 Slovenčina (SK)

### 🎯 Miesta Začiatku (Čítaj v Tomto Poradí)

| Krok | Dokument | Obsah |
|------|----------|-------|
| 1️⃣ | [README_SK.md](README_SK.md) | Úvod, rýchly štart, príklady |
| 2️⃣ | [ARCHITECTURE_SK.md](ARCHITECTURE_SK.md) | Celkový systém, 15-uzlový design |
| 3️⃣ | [BOOTLOADER_DESIGN_SK.md](BOOTLOADER_DESIGN_SK.md) | FLASH rozdelenie, OTA aktualizácie |
| 4️⃣ | [IMPLEMENTATION_SUMMARY_SK.md](IMPLEMENTATION_SUMMARY_SK.md) | Suhrn komponentov, deployment |

### 🔧 Technické API Dokumentácie

| Komponenta | Opis | Súbor |
|-----------|-------|--------|
| **EEPROM** | Perzistentné úložisko | [eeprom_SK.md](driver/eeprom_SK.md) |
| **Device Config** | Device ID + MODBUS adresa | [device_config_SK.md](driver/device_config_SK.md) |
| **Device Registry** | Sensor mapping vzor | [device_registry_SK.md](driver/device_registry_SK.md) |
| **MODBUS Slave** | MODBUS adapter | [modbus_slave_SK.md](driver/modbus_slave_SK.md) |
| **ADC Driver** | Light sensor + interná teplota | [adc_SK.md](driver/adc_SK.md) |
| **RTT Debug** | Printf cez SEGGER RTT | [debug_rtt_SK.md](driver/debug_rtt_SK.md) |

### 📝 Príklady a Prílohy

| Typ | Umiestnenie | Popis |
|-----|-------------|-------|
| **Test Aplikácia** | `apps/test_modbus_slave/` | Kompletnú integráciu |
| **RTT Debug Impl** | `apps/test_modbus_slave/RTT_DEBUG_IMPLEMENTATION.md` | RTTprintf implementácia |
| **Linker Script** | `bsp/STM32F103C8/stm32f103c8_flash_app.ld` | App offset (0x08001000) |

---

## 🇬🇧 English (EN)

### 🎯 Entry Points (Read in This Order)

| Step | Document | Content |
|------|----------|---------|
| 1️⃣ | [README_SK.md](README_SK.md) (scroll to EN section) | Introduction, quick start, examples |
| 2️⃣ | [ARCHITECTURE.md](ARCHITECTURE.md) | Overall system, 15-node design |
| 3️⃣ | [BOOTLOADER_DESIGN.md](BOOTLOADER_DESIGN.md) | FLASH layout, OTA updates |
| 4️⃣ | [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md) | Component summary, deployment |

### 🔧 Technical API Documentation

| Component | Description | File |
|-----------|-------------|------|
| **EEPROM** | Persistent storage | [eeprom.md](driver/eeprom.md) |
| **Device Config** | Device ID + MODBUS address | [device_config.md](driver/device_config.md) |
| **Device Registry** | Sensor mapping pattern | [device_registry.md](driver/device_registry.md) |
| **MODBUS Slave** | MODBUS adapter | [modbus_slave.md](driver/modbus_slave.md) |
| **ADC Driver** | Light sensor + internal temp | [adc.md](driver/adc.md) |
| **RTT Debug** | Printf via SEGGER RTT | [debug_rtt.md](driver/debug_rtt.md) |

### 📝 Examples and Attachments

| Type | Location | Description |
|------|----------|-------------|
| **Test Application** | `apps/test_modbus_slave/` | Full integration example |
| **RTT Debug Impl** | `apps/test_modbus_slave/RTT_DEBUG_IMPLEMENTATION.md` | RTTprintf implementation |
| **Linker Script** | `bsp/STM32F103C8/stm32f103c8_flash_app.ld` | App offset (0x08001000) |

---

## 🗂️ Súborová Štruktúra Dokumentácie

```
📦 STM32F103_projects/
├── 📄 README_SK.md ......................... Hlavný úvod (SK + EN)
├── 📄 ARCHITECTURE_SK.md .................. Systémový prehľad (SK)
├── 📄 ARCHITECTURE.md ..................... System overview (EN)
├── 📄 BOOTLOADER_DESIGN_SK.md ............ Bootloader návrh (SK)
├── 📄 BOOTLOADER_DESIGN.md ............... Bootloader design (EN)
├── 📄 IMPLEMENTATION_SUMMARY_SK.md ....... Implementačný súhrn (SK)
├── 📄 IMPLEMENTATION_SUMMARY.md .......... Implementation summary (EN)
│
├── 📁 driver/
│   ├── 📄 eeprom.md ....................... EEPROM API (EN)
│   ├── 📄 eeprom_SK.md .................... EEPROM API (SK)
│   ├── 📄 eeprom.h / eeprom.c ............ EEPROM implementation
│   │
│   ├── 📄 device_config.md ............... Device Config API (EN)
│   ├── 📄 device_config_SK.md ............ Device Config API (SK)
│   ├── 📄 device_config.h / device_config.c
│   │
│   ├── 📄 device_registry.md ............. Registry Pattern API (EN)
│   ├── 📄 device_registry_SK.md .......... Registry Pattern API (SK)
│   ├── 📄 device_registry.h / device_registry.c
│   │
│   ├── 📄 modbus_slave.md ................ MODBUS Slave API (EN)
│   ├── 📄 modbus_slave_SK.md ............. MODBUS Slave API (SK)
│   ├── 📄 modbus_slave.h / modbus_slave.c
│   │
│   ├── 📄 adc.md .......................... ADC Driver API (EN)
│   ├── 📄 adc_SK.md ....................... ADC Driver API (SK)
│   └── 📄 adc.h / adc.c .................. ADC implementation
│
├── 📁 apps/
│   └── 📁 test_modbus_slave/
│       ├── 📄 README.md ................... Test app documentation
│       ├── 📄 main.c ..................... Full integration example
│       ├── 📄 app_config.h
│       └── 📄 Makefile
│
└── 📁 bsp/STM32F103C8/
    └── 📄 stm32f103c8_flash_app.ld ....... App linker script (0x08001000)
```

---

## 🎯 Odporučené Čítanie podľa Role

### 👨‍💼 Manažér / Product Owner
1. [README_SK.md](README_SK.md) - Úvod
2. [ARCHITECTURE_SK.md](ARCHITECTURE_SK.md) - Sekcí "System Overview"
3. [IMPLEMENTATION_SUMMARY_SK.md](IMPLEMENTATION_SUMMARY_SK.md) - Sekcí "Features"

### 👨‍💻 Embedded Developer (Vývoj Nového Uzla)
1. [README_SK.md](README_SK.md) - Rýchly Štart
2. [device_registry_SK.md](driver/device_registry_SK.md) - Ako definovať senzory
3. [modbus_slave_SK.md](driver/modbus_slave_SK.md) - Inicializácia MODBUS
4. [apps/test_modbus_slave/main.c](apps/test_modbus_slave/main.c) - Príklad

### 👨‍🔧 DevOps / Deployment Engineer
1. [BOOTLOADER_DESIGN_SK.md](BOOTLOADER_DESIGN_SK.md) - FLASH rozdelenie
2. [IMPLEMENTATION_SUMMARY_SK.md](IMPLEMENTATION_SUMMARY_SK.md) - Deployment Krok 1-4
3. [device_config_SK.md](driver/device_config_SK.md) - Konfigurácia zariadenia

### 🔬 Systémový Architekt
1. [ARCHITECTURE_SK.md](ARCHITECTURE_SK.md) - Kompletnú
2. [BOOTLOADER_DESIGN_SK.md](BOOTLOADER_DESIGN_SK.md) - Bootloader sekcí
3. Všetky API dokumentácie (`*_SK.md`)

### 🐛 QA / Tester
1. [README_SK.md](README_SK.md) - Príklady
2. [apps/test_modbus_slave/README.md](apps/test_modbus_slave/README.md) - Test app usage
3. [IMPLEMENTATION_SUMMARY_SK.md](IMPLEMENTATION_SUMMARY_SK.md) - Build & Status sekcí

---

## 🔍 Ako Nájsť Čo Hľadáš

| Čo Hľadáš | Kde To Nájsť |
|-----------|--------------|
| Ako čítať senzor cez MODBUS? | [device_registry_SK.md](driver/device_registry_SK.md) - "Príklad: Zariadenie s viacerými senzormi" |
| Ako nastaviť slave adresu? | [device_config_SK.md](driver/device_config_SK.md) - "API" sekcí |
| Ako flashovať zariadenie? | [BOOTLOADER_DESIGN_SK.md](BOOTLOADER_DESIGN_SK.md) - "Procedúra Flašovania" |
| Ako aktualizovať firmware OTA? | [ARCHITECTURE_SK.md](ARCHITECTURE_SK.md) - "Multi-Device Update Strategy" |
| Ako pridat nový senzor? | [IMPLEMENTATION_SUMMARY_SK.md](IMPLEMENTATION_SUMMARY_SK.md) - "Workflow: Pridávanie Nového Senzora" |
| Aké sú FLASH limity? | [BOOTLOADER_DESIGN_SK.md](BOOTLOADER_DESIGN_SK.md) - "Rozdelenie Pamäte FLASH" |
| Ako skompilovat? | [README_SK.md](README_SK.md) - "Rýchly Štart: Skompiluj" |

---

## ✅ Kontrolný Zoznam pre Nasadenie

- [ ] Prečítaný [BOOTLOADER_DESIGN_SK.md](BOOTLOADER_DESIGN_SK.md)
- [ ] Pochopený FLASH layout (0x08000000, 0x08001000, 0x0800F000)
- [ ] Prípravená DeviceConfig pre každý uzol (device_id, modbus_slave_addr)
- [ ] Skompilovaná test_modbus_slave app
- [ ] Flashovaný bootloader na všetkých 15 uzloch
- [ ] Flashovaná app na všetkých 15 uzloch
- [ ] Testovaná MODBUS komunikácia (FC3, FC16)
- [ ] Overená perzistencia konfigurácie (EEPROM)

---

## 📞 Rýchla Pomoc

**Problém**: Build zlyhá

→ Čítaj: [IMPLEMENTATION_SUMMARY_SK.md](IMPLEMENTATION_SUMMARY_SK.md) - "Build Status" sekcí

**Problém**: MODBUS master nedostáva odpoveď

→ Čítaj: [README_SK.md](README_SK.md) - "Troubleshooting" sekcí

**Problém**: EEPROM konfigurácia sa neuloží

→ Čítaj: [device_config_SK.md](driver/device_config_SK.md) - "Típové chyby"

**Problém**: Chcem pridať nový senzor

→ Čítaj: [device_registry_SK.md](driver/device_registry_SK.md) - "Príklady"

---

## 🎓 Vzdelávací Plán

**Deň 1: Základy**
- Prečítaj [README_SK.md](README_SK.md)
- Skompiluj [test_modbus_slave](apps/test_modbus_slave/)
- Vizualizuj FLASH layout z [BOOTLOADER_DESIGN_SK.md](BOOTLOADER_DESIGN_SK.md)

**Deň 2: Komponenty**
- Prečítaj [device_registry_SK.md](driver/device_registry_SK.md)
- Prečítaj [modbus_slave_SK.md](driver/modbus_slave_SK.md)
- Porozumej registry mapovaniu

**Deň 3: Nasadenie**
- Prečítaj [BOOTLOADER_DESIGN_SK.md](BOOTLOADER_DESIGN_SK.md) úplne
- Prečítaj [IMPLEMENTATION_SUMMARY_SK.md](IMPLEMENTATION_SUMMARY_SK.md) - "Deployment"
- Príprav DeviceConfig pre prvý uzol

**Deň 4: Optimizácia**
- Prečítaj [ARCHITECTURE_SK.md](ARCHITECTURE_SK.md) - "Feature Extensibility"
- Navrhni registry pre svoju aplikáciu
- Príprav pre 15-uzlové nasadenie

---

**Posledná aktualizácia**: Jún 2026
**Cieľ**: STM32F103C8
**Compiler**: GCC arm-none-eabi 11.3.1
**Status**: ✅ Production Ready
