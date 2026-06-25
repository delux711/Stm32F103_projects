# ir_tx

Ovládač pre infračervený vysielač – protokol **NEC** s 38 kHz nosnou frekvenciou.
Určený primárne na ovládanie LED svetelných pásov a RGB kontrolérov; ako bonus podporuje kódy pre LG a Samsung televízory.

## Súbory
- `ir_tx.h` – verejné API, konfiguračná štruktúra, preddefinované kódy
- `ir_tx.c` – implementácia

## Protokol – NEC

```
Carrier: 38 kHz, duty cycle 1/3
```

Štruktúra rámca (32 bitov, LSB first):

```
[ address 8b ][ ~address 8b ][ command 8b ][ ~command 8b ]
```

| Signál | Trvanie |
|---|---|
| Leading mark (carrier ON) | 9 000 µs |
| Leading space (carrier OFF) | 4 500 µs |
| Bit mark | 562 µs |
| Bit-0 space | 562 µs |
| Bit-1 space | 1 687 µs |
| Stop mark | 562 µs |
| **Celý rámec** | **≈ 67.5 ms** |

Repeat rámec (kláves držaný, každých ~110 ms):

```
9 000 µs mark  +  2 250 µs space  +  562 µs mark
```

## Hardware

```
STM32 PA6 ──[33 Ω]──► IR LED ──► GND
```

Pre väčší dosah (≥ 5 m) – tranzistorový budič:

```
PA6 ──[1 kΩ]──► NPN báza (BC337)
                NPN kolektor ──[IR LED]──► VCC 3.3 V
                NPN emitor  ──► GND
```

TIM3 CH1 je mapovaný na **PA6** bez AFIO remapu.

## Konfigurácia

```c
typedef struct {
    GPIO_TypeDef      *port;             // GPIO port IR LED pinu
    uint8_t            pin;              // pin 0–15
    TIM_TypeDef       *timer;            // napr. TIM3
    uint8_t            channel;          // 1–4
    volatile uint32_t *timer_rcc_reg;    // &RCC->APB1ENR alebo &RCC->APB2ENR
    uint32_t           timer_rcc_bit;    // napr. RCC_APB1ENR_TIM3EN
    uint32_t           afio_remap_mask;  // 0 = žiadny remap
    uint32_t           afio_remap_val;   // 0 = žiadny remap
    uint32_t           cpu_freq_hz;      // frekvencia CPU = frekvencia timera
} IR_TX_config_t;
```

> **Poznámka ku clock tree:** TIM2/TIM3/TIM4 sú na APB1. Pri systémovej frekvencii 72 MHz a delení APB1/2 = 36 MHz platí, že timer clock = 2 × APB1 = **72 MHz**. `cpu_freq_hz` teda zadaj vždy ako `SystemCoreClock` (72 000 000).

## API

### `IR_TX_init`
```c
void IR_TX_init(const IR_TX_config_t *config);
```
- Povolí GPIO a timer hodiny
- Nakonfiguruje pin ako AF Push-Pull 50 MHz
- Nastaví timer PSC=0, ARR = cpu_freq / 38 000 − 1
- Spustí PWM v PWM mode 1 (carrier OFF = CCR=0)
- Inicializuje DWT cycle counter

---

### `IR_TX_sendNEC`
```c
void IR_TX_sendNEC(uint8_t address, uint8_t command);
```
Odošle kompletný 32-bitový NEC rámec. **Blokujúce** ≈ 67.5 ms.

---

### `IR_TX_sendNECRepeat`
```c
void IR_TX_sendNECRepeat(void);
```
Odošle repeat rámec (kláves držaný). **Blokujúce** ≈ 11.25 ms.
Volať každých ~110 ms pokiaľ je kláves stlačený.

## Preddefinované kódy

### LED pásy – 44-key RGB kontrolér (`IR_ADDR_LED_STRIP = 0x00`)

| Define | Kód | Funkcia |
|---|---|---|
| `IR_CMD_LED_POWER` | `0x40` | Zapnutie / vypnutie |
| `IR_CMD_LED_BRIGHT_UP` | `0x5C` | Jas + |
| `IR_CMD_LED_BRIGHT_DOWN` | `0x5D` | Jas − |
| `IR_CMD_LED_RED` | `0x58` | Červená |
| `IR_CMD_LED_GREEN` | `0x18` | Zelená |
| `IR_CMD_LED_BLUE` | `0x08` | Modrá |
| `IR_CMD_LED_WHITE` | `0x48` | Biela |
| `IR_CMD_LED_FLASH` | `0x41` | Flash efekt |
| `IR_CMD_LED_STROBE` | `0x42` | Strobe efekt |
| `IR_CMD_LED_FADE` | `0x43` | Fade efekt |
| `IR_CMD_LED_SMOOTH` | `0x44` | Plynulý prechod farieb |

> Lacné čínske 44-key diaľkové ovládače LED pásov väčšinou používajú práve adresu **0x00** s týmito kódmi. Ak tvoj kontrolér nereaguje, zmeraj signál zo svojho originálneho diaľkového ovládača IR prijímačom (napr. TSOP38238) a zapíš vlastné kódy.

### LG TV – bonus (`IR_ADDR_LG_TV = 0x04`)

| Define | Kód | Funkcia |
|---|---|---|
| `IR_CMD_LG_POWER` | `0x08` | Zapnutie / vypnutie |
| `IR_CMD_LG_VOL_UP` | `0x02` | Hlasitosť + |
| `IR_CMD_LG_VOL_DOWN` | `0x03` | Hlasitosť − |
| `IR_CMD_LG_MUTE` | `0x09` | Stíšenie |
| `IR_CMD_LG_CH_UP` | `0x00` | Kanál + |
| `IR_CMD_LG_CH_DOWN` | `0x01` | Kanál − |

### Samsung TV – bonus (`IR_ADDR_SAMSUNG_TV = 0x07`)

| Define | Kód | Funkcia |
|---|---|---|
| `IR_CMD_SAMSUNG_POWER` | `0x02` | Zapnutie / vypnutie |
| `IR_CMD_SAMSUNG_VOL_UP` | `0x07` | Hlasitosť + |
| `IR_CMD_SAMSUNG_VOL_DOWN` | `0x0B` | Hlasitosť − |
| `IR_CMD_SAMSUNG_MUTE` | `0x0F` | Stíšenie |
| `IR_CMD_SAMSUNG_CH_UP` | `0x12` | Kanál + |
| `IR_CMD_SAMSUNG_CH_DOWN` | `0x10` | Kanál − |

## Použitie

```c
static const IR_TX_config_t ir_cfg = {
    .port            = GPIOA,
    .pin             = 6u,
    .timer           = TIM3,
    .channel         = 1u,
    .timer_rcc_reg   = (volatile uint32_t *)&RCC->APB1ENR,
    .timer_rcc_bit   = RCC_APB1ENR_TIM3EN,
    .afio_remap_mask = 0u,
    .afio_remap_val  = 0u,
    .cpu_freq_hz     = 72000000u
};

IR_TX_init(&ir_cfg);

// Zapni LED pás
IR_TX_sendNEC(IR_ADDR_LED_STRIP, IR_CMD_LED_POWER);
SYS_delayMs(200u);

// Nastav bielu farbu
IR_TX_sendNEC(IR_ADDR_LED_STRIP, IR_CMD_LED_WHITE);
SYS_delayMs(200u);

// LG TV – zvýš hlasitosť (so simuláciou držania klávesu)
IR_TX_sendNEC(IR_ADDR_LG_TV, IR_CMD_LG_VOL_UP);
SYS_delayMs(40u);  // medzera do prvého repeat
for (uint8_t i = 0; i < 5; i++) {
    IR_TX_sendNECRepeat();
    SYS_delayMs(98u);  // 11.25 ms repeat + ~99 ms = ~110 ms celkovo
}
```

## Mapovanie timera na pin (STM32F103)

| Timer | Channel | Pin (no remap) | Pin (partial) | Pin (full) |
|---|---|---|---|---|
| TIM2 | CH1 | PA0 | PA15 | – |
| TIM2 | CH2 | PA1 | PB3 | – |
| TIM3 | CH1 | **PA6** | PB4 | PC6 |
| TIM3 | CH2 | PA7 | PB5 | PC7 |
| TIM4 | CH1 | PB6 | – | PD12 |
| TIM4 | CH2 | PB7 | – | PD13 |

## Poznámky
- `IR_TX_sendNEC` a `IR_TX_sendNECRepeat` sú **blokujúce** (busy-wait cez DWT). Nevolať z prerušenia.
- Nosná 38 kHz je generovaná hardvérom (PWM), CPU nie je zaťažené počas carrier pulse trénov.
- Príjem IR signálu (dekódovanie) tento driver **nepodporuje** – na to použiť samostatný IR RX modul (napr. TSOP38238) na iný pin.
