# EEPROM Ovládač (STM32F103)

## Prehľad
Emulovaný EEPROM ovládač pre STM32F103 využívajúci poslednú 4 KB pamäte FLASH (strany 31-32 zo 64 KB zariadenia).

## Konfigurácia
- **Počiatočná adresa**: 0x08000000 + 60KB = 0x0800F000
- **Veľkosť**: 4 KB (dve strany po 2 KB)
- **Typické použitie**: Konfigurácia zariadenia, kalibračné údaje, perzistentné parametre

## API

### `void EEPROM_init(void)`
Inicializácia subsystému EEPROM. Volať raz pri spustení.

### `EEPROM_status_t EEPROM_read(uint16_t offset, uint8_t *data, uint16_t len)`
Čítanie bajtov z EEPROM.

**Parametre**:
- `offset`: Offset v bajtoch (0 až EEPROM_TOTAL_SIZE-1)
- `data`: Vyrovnávacia pamäť pre uloženie údajov
- `len`: Počet bajtov na čítanie

**Vracia**: `EEPROM_OK` pri úspechu, kód chyby v ostatných prípadoch

**Príklad**:
```c
uint8_t config_buf[32];
EEPROM_read(0, config_buf, sizeof(config_buf));
```

### `EEPROM_status_t EEPROM_write(uint16_t offset, const uint8_t *data, uint16_t len)`
Zápis bajtov do EEPROM (zarovnané slová, zápis po 16 bitoch).

**Parametre**:
- `offset`: Offset v bajtoch
- `data`: Údaje na zápis
- `len`: Počet bajtov na zápis

**Vracia**: `EEPROM_OK` pri úspechu

**Upozornenie**: Dĺžka musí byť párna. Údaje sa zapisujú ako 16-bitové slová.

**Príklad**:
```c
uint8_t config_buf[32] = {...};
EEPROM_write(0, config_buf, sizeof(config_buf));
```

### `EEPROM_status_t EEPROM_erase(uint16_t offset, uint16_t len)`
Vymazanie jednej alebo viacerých EEPROM strán (offset musí byť zarovnaný na stranu, dĺžka musí byť násobkom 2048).

## Poznámky k hardvéru
- Čas zápisu FLASH: ~30µs na 2 bajty (pri 72 MHz)
- Čas vymazania strany: ~20ms na stranu
- Odolnosť: ~10 000 cyklov vymazania typicky na stranu
- Pre časté zápisy implementujte vyvažovanie opotrebenia naprieč viacerými stranami

## Integrácia
1. Pridajte `#include "eeprom.h"` do aplikácie
2. Zavolajte `EEPROM_init()` v `main()`
3. Používajte `EEPROM_read()` / `EEPROM_write()` podľa potreby

## Stavy chýb
```c
EEPROM_OK              // Operácia úspešná
EEPROM_ERR_LOCKED      // Pamäť je uzamknutá
EEPROM_ERR_INVALID_ADDR // Neplatná adresa
EEPROM_ERR_WRITE_FAILED // Zápis zlyhал
```
