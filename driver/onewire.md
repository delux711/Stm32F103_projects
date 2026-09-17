# onewire

Bitbang implementácia 1-Wire komunikácie pre STM32F103. Časovanie je realizované cez DWT cycle counter pre presnú mikrosekundovú oneskorenie.

## Súbory
- `onewire.h` – verejné API, typy
- `onewire.c` – implementácia

## Zapojenie

| Signál | Popis |
|---|---|
| Dátová linka | Jeden GPIO pin konfigurovaný ako **open-drain výstup** |
| Pull-up | Externý rezistor **4,7 kΩ** na VCC (3,3 V) |

## Konfigurácia

```c
typedef struct {
    GPIO_TypeDef *port;        // GPIO port, napr. GPIOB
    uint8_t       pin;         // číslo pinu 0–15
    uint32_t      cpu_freq_hz; // frekvencia CPU v Hz, napr. 72000000
} OW_config_t;
```

## API

### `OW_init`
```c
void OW_init(const OW_config_t *config);
```
Povolí hodiny GPIO, nakonfiguruje pin ako open-drain výstup 50 MHz a spustí DWT cycle counter.

---

### `OW_reset`
```c
OW_status_t OW_reset(void);
```
Vyšle reset pulz (500 µs LOW) a čaká na presence pulse.

| Návratová hodnota | Popis |
|---|---|
| `OW_OK` | Aspoň jedno zariadenie odpovedalo |
| `OW_NO_PRESENCE` | Žiadne zariadenie nedetegované |

---

### `OW_writeByte` / `OW_readByte`
```c
void    OW_writeByte(uint8_t byte);
uint8_t OW_readByte(void);
```
Zapíše / prečíta jeden bajt (LSB first).

---

### `OW_writeBit` / `OW_readBit`
```c
void    OW_writeBit(uint8_t bit);
uint8_t OW_readBit(void);
```
Zápis / čítanie jedného bitu (nízkoúrovňové).

---

### `OW_skipROM`
```c
void OW_skipROM(void);
```
Odošle príkaz `0xCC` – adresuje všetky zariadenia naraz (vhodné pri jedinom zariadení na zbernici).

---

### `OW_matchROM`
```c
void OW_matchROM(const OW_rom_t *rom);
```
Odošle príkaz `0x55` + 8-bajtový ROM kód – adresuje konkrétne zariadenie.

---

### `OW_readROM`
```c
OW_status_t OW_readROM(OW_rom_t *rom);
```
Prečíta 64-bitový ROM kód (príkaz `0x33`). Použiteľné len pri jedinom zariadení na zbernici. Overuje CRC.

---

### `OW_searchROM`
```c
OW_status_t OW_searchROM(OW_rom_t *found, uint8_t max_count, uint8_t *count);
```
Vyhľadá všetky zariadenia na zbernici (Maxim AN 187 algoritmus).

| Parameter | Popis |
|---|---|
| `found` | Pole pre uloženie nájdených ROM kódov |
| `max_count` | Kapacita poľa |
| `count` | Výstup – počet nájdených zariadení |

---

### `OW_crc8`
```c
uint8_t OW_crc8(const uint8_t *data, uint8_t len);
```
Dallas/Maxim CRC-8 (poly `0x31`, init `0x00`). Správny 8-bajtový ROM kód vrátane CRC bajtu dáva výsledok `0x00`.

## Stavové kódy – `OW_status_t`

| Hodnota | Popis |
|---|---|
| `OW_OK` | Operácia úspešná |
| `OW_NO_PRESENCE` | Žiadne zariadenie neodpovedalo |
| `OW_CRC_ERROR` | Chyba CRC |

## ROM kód – `OW_rom_t`

```c
#define OW_ROM_SIZE 8u

typedef struct {
    uint8_t bytes[OW_ROM_SIZE];  // [0]=family code, [1..6]=serial, [7]=CRC
} OW_rom_t;
```

## Použitie – DS18B20 teplota

```c
static const OW_config_t ow_cfg = {
    .port        = GPIOB,
    .pin         = 7u,
    .cpu_freq_hz = 72000000u
};

OW_init(&ow_cfg);

// Spusti konverziu
OW_reset();
OW_skipROM();
OW_writeByte(0x44);           // Convert T

SYS_delayMs(750u);            // čakaj na 12-bit konverziu

// Prečítaj scratchpad
OW_reset();
OW_skipROM();
OW_writeByte(0xBE);           // Read Scratchpad

uint8_t sp[9];
for (uint8_t i = 0; i < 9; i++) sp[i] = OW_readByte();

if (OW_crc8(sp, 9) == 0u) {
    int16_t raw  = (int16_t)(((uint16_t)sp[1] << 8) | sp[0]);
    float   temp = raw / 16.0f;  // °C
}
```

## Časovanie slotov (podľa Maxim AN 126)

| Operácia | Čas LOW | Čas HIGH / celkovo |
|---|---|---|
| Reset | 500 µs | uvoľni, sample po 70 µs, čakaj 410 µs |
| Write 1 | 6 µs | 64 µs |
| Write 0 | 60 µs | 10 µs |
| Read | 6 µs | sample po ďalších 9 µs, tail 55 µs |

## Poznámky
- DWT counter musí byť spustený pred prvým volaním (`OW_init` to zabezpečí automaticky).
- Funkcie nie sú thread-safe – nevolať z prerušenia súčasne s hlavnou slučkou.
- Pri viacerých zariadeniach na zbernici použiť `OW_searchROM` + `OW_matchROM`.
