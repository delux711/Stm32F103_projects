# button

Ovládač tlačidiel s debounce logikou, detekciou single/double/triple kliknutia a long-press. Podporuje EXTI aj polling režim.

## Súbory
- `button.h` – verejné API, typy, makro `BUTTON_DEF`
- `button.c` – implementácia state machine

## Konfigurácia tlačidla

Tlačidlo sa popisuje štruktúrou `btn_t`. Pre pohodlné vytváranie slúži makro `BUTTON_DEF`:

```c
#define BUTTON_DEF(port, pin, single_cb, double_cb, triple_cb, long_cb)
```

Príklad:

```c
static volatile btn_t buttons[] = {
    BUTTON_DEF(GPIOA, 0, onSingle, onDouble, NULL, onLong),
    BUTTON_DEF(GPIOB, 1, onSingle, NULL,     NULL, NULL  ),
};
```

## Inicializácia

```c
BUTTON_init(buttons, 2u);
```

Nastaví GPIO piny ako vstupy s pull-up, nakonfiguruje EXTI linky a povolí príslušné prerušenia.

## Časové konštanty

| Konštanta | Hodnota | Popis |
|---|---|---|
| `DEBOUNCE_MS` | 20 ms | Debounce čas pri stlačení aj uvoľnení |
| `LONG_PRESS_MS` | 800 ms | Minimálna dĺžka long-press |
| `MULTICLICK_MS` | 400 ms | Max pauza medzi kliknutiami pre multi-click |

## API

### `BUTTON_init`
```c
void BUTTON_init(const volatile btn_t *button_configs, uint32_t count);
```
Inicializuje všetky tlačidlá, nastaví GPIO a EXTI.

---

### `BUTTON_process`
```c
void BUTTON_process(void);
```
Volať každú 1 ms (z `SysTick_Handler`). Riadi state machine debounce a multi-click logiku pre aktívne (polling) tlačidlá.

---

### `BUTTON_irqGlobalHandler`
```c
void BUTTON_irqGlobalHandler(void);
```
Volať zo všetkých `EXTIx_IRQHandler`. Prepína tlačidlá do polling režimu pri detekovaní hrany.

---

### `BUTTON_delayMs`
```c
void BUTTON_delayMs(uint32_t ms);
```
Pomocná čakacia funkcia (blokujúca).

## Zapojenie v `bsp_irq.c`

```c
void SysTick_Handler(void) {
    SYS_incrementMs();
    BUTTON_process();
}

void EXTI0_IRQHandler(void)  { BUTTON_irqGlobalHandler(); }
void EXTI1_IRQHandler(void)  { BUTTON_irqGlobalHandler(); }
// ... ďalšie EXTI linky podľa použitých pinov
```

## Stav tlačidla – `btnState_t`

```
BTN_IDLE → BTN_DEBOUNCE_PRESS → BTN_PRESSED → BTN_DEBOUNCE_RELEASE → BTN_IDLE
```

## Použitie (príklad)

```c
static void onSingle(void) { DEBUG_writeString("single\r\n"); }
static void onDouble(void) { DEBUG_writeString("double\r\n"); }
static void onLong(void)   { DEBUG_writeString("long\r\n");   }

static volatile btn_t btns[] = {
    BUTTON_DEF(GPIOA, 0, onSingle, onDouble, NULL, onLong),
};

BUTTON_init(btns, 1u);
```

## Poznámky
- Tlačidlo musí byť zapojené ako active-LOW (stlačené = GND).
- Pri long-press sa callback zavolá ihneď po dosiahnutí `LONG_PRESS_MS`, nie po uvoľnení.
- Single/double/triple sa rozlišujú po uplynutí `MULTICLICK_MS` od posledného kliknutia.
