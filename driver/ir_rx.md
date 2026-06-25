# ir_rx

Ovládač pre príjem infračervených príkazov z demodulovaných prijímačov,
napríklad **TSOP4838**, TSOP38238, VS1838B alebo HX1838.

Driver dekóduje protokol **NEC** pomocou EXTI prerušení a časovania v mikrosekundách.

## Súbory
- `ir_rx.h` - verejné API a dátové štruktúry
- `ir_rx.c` - implementácia dekódera NEC

## Podporovaný protokol

NEC rámec (32 bitov, LSB first):

```
[ address 8b ][ ~address 8b ][ command 8b ][ ~command 8b ]
```

Použité časovanie medzi zostupnými hranami (falling-edge to falling-edge):

| Interval | Typický čas |
|---|---:|
| Header | 13 500 us |
| Bit 0 | 1 125 us |
| Bit 1 | 2 250 us |
| Repeat | 11 250 us |

## Hardware (TSOP4838)

Príklad zapojenia na STM32F103:

```
TSOP4838 VCC  -> 3.3 V
TSOP4838 GND  -> GND
TSOP4838 OUT  -> PA1 (EXTI1)
```

Odporúčané odrušenie pri senzore:
- 100 nF medzi VCC a GND (čo najbližšie k senzoru)
- voliteľne 4.7 uF paralelne k 100 nF

Výstup TSOP je v kľude HIGH, pri prijatí IR burstu ide do LOW.

## Konfigurácia

```c
static const IR_RX_config_t ir_cfg = {
    .port           = GPIOA,
    .pin            = 1u,
    .exti_line      = 1u,
    .timer          = TIM2,
    .timer_rcc_reg  = (volatile uint32_t *)&RCC->APB1ENR,
    .timer_rcc_bit  = RCC_APB1ENR_TIM2EN,
    .timer_clock_hz = 72000000u
};
```

## API

### `IR_RX_init`
```c
void IR_RX_init(const IR_RX_config_t *config);
```
- GPIO nastaví na vstup s pull-up
- nastaví EXTI na zostupnú hranu
- spustí voľne bežiaci timer na 1 MHz

### `IR_RX_irqGlobalHandler`
```c
void IR_RX_irqGlobalHandler(void);
```
Volá sa z EXTI IRQ handlerov cez `bsp_irq.c`.

### `IR_RX_readFrame`
```c
uint8_t IR_RX_readFrame(IR_RX_frame_t *out_frame);
```
Vráti 1, ak je pripravený nový NEC rámec.

### `IR_RX_readRepeat`
```c
uint8_t IR_RX_readRepeat(void);
```
Vráti 1, ak bol prijatý repeat rámec (držané tlačidlo).

## Príklad použitia

```c
IR_RX_frame_t frame;

IR_RX_init(&ir_cfg);

while (1) {
    if (IR_RX_readFrame(&frame)) {
        if (frame.valid) {
            // frame.address a frame.command obsahujú dekódovaný príkaz
        }
    }

    if (IR_RX_readRepeat()) {
        // držané tlačidlo
    }
}
```

## Poznámky
- Driver je pripravený pre demodulované prijímače 38 kHz (TSOP4838).
- Aktuálne dekóduje NEC timing; iné protokoly nie sú implementované.
- Pri silnom rušení môže vzniknúť nevalidný rámec (`frame.valid == 0`).