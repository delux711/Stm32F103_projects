# systick

Jednoduchý milisekundový časovač na báze SysTick prerušenia.

## Súbory
- `systick.h` – verejné API
- `systick.c` – implementácia

## Inicializácia

Časovač nevyžaduje vlastnú init funkciu. Pred použitím je potrebné:

1. Nastaviť SysTick tak, aby generoval prerušenie každú 1 ms:
   ```c
   SysTick_Config(SystemCoreClock / 1000u);
   ```
2. Volať `SYS_incrementMs()` z handlera `SysTick_Handler` (zvyčajne v `bsp_irq.c`):
   ```c
   void SysTick_Handler(void) {
       SYS_incrementMs();
   }
   ```

## API

### `SYS_getMs`
```c
uint32_t SYS_getMs(void);
```
Vráti aktuálny čas v milisekundách od štartu.

---

### `SYS_incrementMs`
```c
void SYS_incrementMs(void);
```
Inkrementuje interný čítač. Volať výhradne z `SysTick_Handler`.

---

### `SYS_delayMs`
```c
void SYS_delayMs(uint32_t ms);
```
Blokujúce čakanie v milisekundách.
Interne volá `__WFI()` pre šetrenie CPU počas čakania.

## Použitie

```c
SysTick_Config(SystemCoreClock / 1000u);

uint32_t start = SYS_getMs();
// ... niečo urobiť ...
if ((SYS_getMs() - start) > 500u) {
    // uplynulo viac ako 500 ms
}

SYS_delayMs(1000u);  // čakaj 1 sekundu
```

## Poznámky
- Čítač pretečie po ~49 dňoch (32-bitový uint32).
- `SYS_delayMs` nie je vhodné volať z prerušenia.
