# Architektúra Bootloaderu STM32F103

## Rozdelenie pamäte FLASH (64 KB)

```
0x08000000 ┌─────────────────────────┐
           │  BOOTLOADER             │
           │  (Nemodifikovateľný)    │
           │                         │
           │  Veľkosť: 4 KB – 8 KB   │
           ├─────────────────────────┤ 0x08001000 alebo 0x08002000
           │  APLIKÁCIA (RW)         │
           │  (Modifikovateľná)      │
           │                         │
           │  Veľkosť: 48 KB – 52 KB │
           │                         │
           │  Obsahuje:              │
           │  - Užívateľský kód      │
           │  - Ovládače             │
           │  - MODBUS stack         │
           ├─────────────────────────┤ 0x0800F000
           │  CONFIG/EEPROM          │
           │  (Perzistentné úložisko)│
           │                         │
           │  Veľkosť: 4 KB          │
           │                         │
           │  Obsahuje:              │
           │  - DeviceConfig         │
           │  - Kalibračné údaje     │
           │  - MODBUS nastavenia    │
           └─────────────────────────┘ 0x08010000 (koniec)
```

## Typická konfigurácia

### Možnosť 1: 4 KB Bootloader (ODPORÚČANÉ)
- **Bootloader**: 0x08000000 – 0x08001000 (4 KB)
- **Aplikácia**: 0x08001000 – 0x0800F000 (56 KB)
- **Config/EEPROM**: 0x0800F000 – 0x08010000 (4 KB)

**Výhody**:
- Maximálny priestor pre aplikáciu
- Jednoduché zarovnanie (hranice strán)

### Možnosť 2: 8 KB Bootloader
- **Bootloader**: 0x08000000 – 0x08002000 (8 KB)
- **Aplikácia**: 0x08002000 – 0x0800F000 (52 KB)
- **Config/EEPROM**: 0x0800F000 – 0x08010000 (4 KB)

**Použitie keď**: Bootloader potrebuje pokročilé funkcie (CRC, podpisovanie, atď.)

## Nastavenie Linker Scriptu

### Pre Aplikáciu (Loadovateľná na 0x08001000)

**stm32f103c8_flash_app.ld**:
```linker
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08001000, LENGTH = 56K
  RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
  /* ... regulárne sekcie, ale FLASH origin je 0x08001000 ... */
}
```

**Kľúčové zmeny zo standartného**:
- Zmeniť `ORIGIN = 0x08001000` (nie 0x08000000)
- Znížiť `LENGTH = 56K` (nie 64K)
- Vektorová tabuľka musí byť preusmerovaná za behu alebo fixná v bootloaderi

### Preusmerenie Vektorovej Tabuľky

**V bootloaderi**:
```c
// stm32f103c8_flash.ld (Bootloader script, ponechat predvolené 0x08000000)
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 4K
  // ...
}
```

**V aplikácii startup_gcc.c**:
```c
#define VECT_TAB_OFFSET  (0x1000U)  // 4 KB bootloader offset

void SystemInit(void)
{
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;  // Preusmeruj vektorovú tabuľku
    // ... zvyšok init
}
```

## Zodpovednosti Bootloaderu

1. **Validácia**: Skontrolovať, či existuje platná aplikácia na 0x08001000
2. **Verifikácia**: Voliteľná CRC/podpisová kontrola aplikačného binárneho súboru
3. **Kontrola Verzie**: Porovnať verziu bootloaderu s verziou FW aplikácie
4. **Skáknutie na Aplikáciu**: Nastaviť zásobník a skočiť na reset handler aplikácie
5. **Fallback**: Ak aplikácia neplatná, ostať v bootloaderi alebo vstúpiť do režimu obnovy

## Zodpovednosti Aplikácie

1. **Device Config**: Načítať MODBUS adresu a device ID z EEPROM
2. **Inicializácia**: Nastaviť všetky periférie
3. **Hlavný Cyklus**: Spustiť MODBUS slave, senzory, atď.

## Zostavovanie pre každú Komponentu

### Preložiť Bootloader
```bash
# Zmeniť root Makefile na výber bootloader linker scriptu
make APP=bootloader LINKER=stm32f103c8_flash.ld
```

### Preložiť Aplikáciu
```bash
# Aplikácia používa app-špecifický linker script s offsetom
make APP=test_adc_light LINKER=stm32f103c8_flash_app.ld
```

### Procedúra Flašovania
```bash
# Flashovať bootloader na 0x08000000
openocd -c "program bootloader.elf 0x08000000 verify"

# Flashovať aplikáciu na 0x08001000
openocd -c "program app.elf 0x08001000 verify"
```

## Register Unikátneho ID Zariadenia (Unique per STM32)

Každý STM32F103C8 má unikátne 96-bitové ID na 0x1FFFF7E0–E0B:
```c
#define UID_BASE  (0x1FFFF7E0)
#define DEVID0    (*(uint32_t *)(UID_BASE + 0x00))
#define DEVID1    (*(uint32_t *)(UID_BASE + 0x04))
#define DEVID2    (*(uint32_t *)(UID_BASE + 0x08))
```

**Prípad použitia**:
- Bootloader číta UID pri spustení
- Porovnáva s uloženou hodnotou v DeviceConfig
- V prípade nezhody môže byť zariadenie v nesprávnom slote (bezpečnostná kontrola)

## Integrácia s Device Config

**Lokácia Config EEPROM**: Posledná 4 KB FLASH (0x0800F000)

**Pri spustení aplikácie**:
```c
int main(void)
{
    SystemInit();
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;  // Preusmeruj vektory

    DeviceConfig_init();  // Načítaj z EEPROM
    DeviceConfig_t *cfg = DeviceConfig_get();

    // Použi cfg->modbus_slave_addr, cfg->device_id, atď.
}
```

## Stratégia Aktualizácie viacerých Zariadení (15 uzlov)

1. **Všetky zariadenia spúšťajú rovnaký bootloader kód** (fixný, v prvých 4 KB)
2. **Každé zariadenie má unikátny DeviceConfig v EEPROM**
3. **Aplikačné binárne súbory môžu byť identické** (config ich rozlišuje)
4. **Over-the-air aktualizácia cez MODBUS**:
   - ESP32 posiela nový aplikačný binár každému uzlu v chunkoch
   - Uzol ukladá do aplikačnej sekcie (0x08001000)
   - Uzol sa reštartuje, bootloader validuje
   - Aplikácia sa spúšťa s device-špecifickým configom z EEPROM

## Príklad Bootloader Vstupu

```c
// V bootloaderi (stm32f103c8_flash.ld)
typedef void (*AppFunc_t)(void);

void bootloader_main(void)
{
    // Validuj aplikáciu na 0x08001000
    uint32_t app_vector = *(uint32_t *)(0x08001000);
    if (app_vector == 0xFFFFFFFF || app_vector == 0x00000000) {
        // Žiadna platná aplikácia, ostani v bootloaderi
        while (1) { /* recovery mode */ }
    }

    // Skočí na aplikáciu
    AppFunc_t jump_to_app = (AppFunc_t)*(uint32_t *)(0x08001004);
    __set_MSP(*(uint32_t *)(0x08001000));
    jump_to_app();
}
```

## Poznámky pre 15-uzlový Systém

- **Jeden bootloader binár** pre všetky uzly (OTA aktualizácia bootloaderu, len ak je potrebná)
- **Jeden app binár** sa dá použiť na všetkých uzloch (konfiguračné súbory sa líšia)
- **Perzistencia Konfigurácie** cez DeviceConfig v poslednej 4 KB EEPROM
- **Pridelenie Slave Adresy** môže byť:
  - Predprogramované v továrni
  - Dynamicky pridelené master pri prvom spustení
  - Modifikované cez MODBUS register zápis + reštart
