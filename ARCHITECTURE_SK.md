# Architektúra STM32F103 MODBUS IoT (15-uzlový Systém)

## Prehľad Systému

```
┌─────────────────────────────────────────────────────────────────┐
│                        ESP32 Master                             │
│              (MODBUS RTU Master cez RS485/UART)                │
│                  - Objavovanie zariadení                        │
│                  - Polling senzorů                              │
│                  - Správa konfigurácií                          │
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

## Architekturálne Komponenty

### 1. EEPROM Ovládač (`driver/eeprom.h/c`)
**Účel**: Perzistentné úložisko pre konfigúráciu zariadenia

- **Lokácia**: Posledná 4 KB z 64 KB FLASH (0x0800F000–0x08010000)
- **Funkcionalita**:
  - FLASH-based EEPROM emulácia
  - Zarovnané zápisy po 16-bitoch
  - Schopnosť vymazania strán
  - CRC ešte nie je implementovaný (možno pridať)

**Rozhranie**:
```c
EEPROM_read(offset, data, len)    // Čítanie z EEPROM
EEPROM_write(offset, data, len)   // Zápis do EEPROM
EEPROM_erase(offset, len)         // Vymazanie sekcie EEPROM
```

### 2. Systém Konfigurácie/ID Zariadenia (`driver/device_config.h/c`)
**Účel**: Unikátna identifikácia zariadenia a konfigurácia MODBUS

**Uložené v EEPROM**:
```c
typedef struct {
    uint32_t magic;              // 0xDEADBEEF
    uint8_t  device_id;          // Unikátne ID zariadenia (0-255)
    uint8_t  modbus_slave_addr;  // MODBUS RTU slave adresa (1-247)
    uint8_t  device_type;        // Typ zariadenia / maska senzora
    uint8_t  fw_version;         // Verzia firmware
    uint32_t hw_serial;          // Sériové číslo hardvéru
} DeviceConfig_t;
```

**Maska typu zariadenia**:
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

**Rozhranie**:
```c
DeviceConfig_init()                                    // Inicializácia
DeviceConfig_get()                                     // Čítanie aktuálnej konfig
DeviceConfig_setDefaults(id, addr, type)             // Nastavenie predvolení
DeviceConfig_loadFromEEPROM()                         // Načítanie z úložiska
DeviceConfig_saveToEEPROM()                           // Uloženie do úložiska
```

### 3. Vzor Device Registry (`driver/device_registry.h/c`)
**Účel**: Flexibilné mapovanie senzorů na registry bez hardcodeingu

**Každá aplikácia definuje registry pole**:
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

**MODBUS Master teraz môže**:
- Čítať ľubovoľný register cez FC3 (Read Holding Registers)
- Písať ľubovoľný RW register cez FC16 (Write Multiple Registers)
- Registry sa automaticky mapuje na MODBUS adresy

**Výhody**:
- Žiadny duplikovaný MODBUS kód na aplikáciu
- Pridávanie senzorů = iba aktualizácia registry pola
- Škálovateľnosť na 15 heterogénnych zariadení

### 4. MODBUS Slave Adaptér (`driver/modbus_slave.h/c`)
**Účel**: Automatické mapovanie Device Registry na MODBUS protokol

**Integrácia**:
```c
// Inicializácia s MODBUS adresou zariadenia z konfig
ModbusSlave_init(&mb_config, cfg->modbus_slave_addr);

// Hlavný cyklus
while (1) {
    ModbusSlave_process();
}
```

**Automaticky zvláda**:
- MODBUS FC3 (Read Holding Registers) → volá DeviceRegistry_read()
- MODBUS FC16 (Write Multiple Registers) → volá DeviceRegistry_write()
- Generovanie a odosielanie rámca odpovede

**Voliteľné vlastné handléry** pre pokročilú logiku:
```c
ModbusSlave_setRegisterReadHandler(my_custom_read);
ModbusSlave_setRegisterWriteHandler(my_custom_write);
```

## Architektúra Bootloaderu

### Rozdelenie Pamäte FLASH (64 KB)
```
0x08000000 ┌─────────────────┐
           │ Bootloader      │  4 KB (fixný, nemodifikovateľný)
           ├─────────────────┤ 0x08001000
           │ Aplikácia       │  56 KB (modifikovateľná)
           │ (linkovaná na   │
           │  0x08001000)    │
           ├─────────────────┤ 0x0800F000
           │ Config/EEPROM   │  4 KB (perzistentný)
           │ (linkovaná na   │
           │  0x0800F000)    │
           └─────────────────┘ 0x08010000 (koniec)
```

### Zodpovednosti Bootloaderu
1. Validovať aplikáciu na 0x08001000 (magic number / CRC)
2. Skontrolovať kompatibilitu verzie firmware
3. Preusmerovať vektorovú tabuľku na umiestnenie aplikácie
4. Skočiť na aplikačný reset handler

### Linker Script Aplikácie (`stm32f103c8_flash_app.ld`)
```
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08001000, LENGTH = 56K  // Offset bootloaderom
  RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 20K
}
```

### Preusmerenie Vektorovej Tabuľky (v app `startup_gcc.c`)
```c
#define VECT_TAB_OFFSET (0x1000U)  // 4 KB bootloader offset

void SystemInit(void)
{
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
    // ...
}
```

## Typická Implementácia Zariadenia

### Príklad: Uzol so Svetelným a Teplotným Senzorom

**Hardvér**:
- ADC na PA0 (GL5537 fotoresistor)
- DS18B20 na PA4 (1-Wire)
- UART1 pre MODBUS (PA9/PA10)

**Kód**:
```c
#include "device_config.h"
#include "device_registry.h"
#include "modbus_slave.h"
#include "adc.h"
#include "ds18b20.h"

// Registry: mapovanie senzorů na MODBUS adresy
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

    DeviceConfig_init();  // Načítaj config z EEPROM
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

## Workflow Systému s viacerými Zariadeniami

### Nasadenie (15 Uzlov)
1. **Jediný bootloader binár** nasadený na všetkých uzloch (4 KB FLASH)
2. **Jediný app binár** nasadený na všetkých uzloch (56 KB FLASH)
3. **Unikátna konfigurácia na zariadenie** uložená v EEPROM každého uzla

### Fáza Konfigurácie
```c
// Továrenská konfigurácia (na zariadenie):
DeviceConfig_t cfg = {
    .magic = 0xDEADBEEF,
    .device_id = 5,              // Unikátne na uzol
    .modbus_slave_addr = 10,     // Unikátne na uzol
    .device_type = 0x03,         // ADC + DS18B20
    .fw_version = 1,
    .hw_serial = 0x12345678
};
EEPROM_write(0, (uint8_t *)&cfg, sizeof(cfg));
```

### Operácia za Behu
**ESP32 Master**:
```c
// Objavovanie všetkých zariadení
for (addr = 1; addr <= 15; addr++) {
    ModbusRead_HoldingReg(addr, 0, &device_id);  // Získaj DEVICE_ID
    printf("Nájdené zariadenie %d na MODBUS addr %d\n", device_id, addr);
}

// Polling senzorů
while (1) {
    for (each_device) {
        ModbusRead_HoldingReg(addr, 10, &light_raw);
        ModbusRead_HoldingReg(addr, 11, &light_mv);
        ModbusRead_HoldingReg(addr, 20, &temp_c10);
        // Spracovanie údajov
    }
}
```

**Každý STM32F103 Slave**:
```c
// Načítá config z EEPROM
// Nastaví MODBUS adresu na cfg->modbus_slave_addr
// Čaká na FC3/FC16 od master-u
// Odpovedá s hodnotami registrov z registry
```

## Rozšírivosť Funkcií

### Pridávanie Nového Typu Senzora
1. Vytvor ovládač: `driver/my_sensor.h/c`
2. Pridaj registry položku v app zariadenia:
   ```c
   { 40, "MY_SENSOR", REGISTRY_TYPE_RO, my_sensor_read, NULL }
   ```
3. Aktualizuj `device_type` masku ak je funkcia voliteľná
4. Preložiť app (bootloader sa nemení)

### Pridávanie Nového Funkčného Kódu (napr. FC4 Input Registers)
1. Rozšír `modbus_slave.c` o nový handler
2. Mapuj na ďalšiu registry tabuľku alebo vlastný callback
3. Žiadne breaking zmeny na existujúcej registry

### Konfigurácia Cez MODBUS
1. Designuj rozsah špecifických registrov (napr. 0x100–0x1FF) pre config
2. Implementuj vlastný write handler:
   ```c
   if (addr >= 0x100 && addr < 0x200) {
       DeviceConfig_t *cfg = DeviceConfig_get();
       cfg->modbus_slave_addr = value;
       DeviceConfig_saveToEEPROM();
       // Reštartuj zariadenie na aplikáciu zmeny
   }
   ```

## Súhrn: Piliere Architektúry

| Komponenta | Účel | Škálovateľnosť |
|-----------|------|----------------|
| **EEPROM** | Perzistentné úložisko konfig | Fixné 4 KB (postačuje pre všetkých 15 zariadení oddelene) |
| **DeviceConfig** | Unikátne ID + MODBUS addr | Na zariadenie: 12 bajtov v EEPROM |
| **DeviceRegistry** | Mapovanie senzor-registra | Na aplikáciu: registry pole (rozšírivateľné) |
| **ModbusSlave** | Adaptér protokolu | Protokol-agnostické: funguje s ľubovoľnou registry |
| **Bootloader** | Nemodifikovateľný boot kód | Jeden kópiu pre všetkých 15 uzlov |
| **Aplikácia** | Logika užívateľa | Jeden binár, konfigurácia odlišuje správanie |

Tento dizajn umožňuje:
✅ **Homogenita**: Všetky uzly spúšťajú rovnaké bootloader + app binárne súbory
✅ **Heterogenita**: Konfigurácia/registry každého uzla ho robí unikátnym
✅ **Škálovateľnosť**: 15+ uzlov s minimálnym duplikáciou kódu
✅ **Flexibilnosť**: Senzory pridané bez dotýkania MODBUS kódu
✅ **Perzistencia**: Kalibrácia, prahy, adresy prežijú výpadok napájania
✅ **OTA Aktualizácie**: Bootloader validuje app binár pred skokom
