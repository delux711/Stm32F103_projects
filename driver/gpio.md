# gpio

Abstrakcia nad GPIO registrami STM32F103.

## Súbory
- `gpio.h` – verejné API
- `gpio.c` – implementácia

## API

### `GPIO_enableClock`
```c
void GPIO_enableClock(GPIO_TypeDef *port);
```
Zapne hodinový signál pre daný GPIO port cez register `RCC->APB2ENR`.
Podporované porty: `GPIOA`, `GPIOB`, `GPIOC`, `GPIOD`.

---

### `GPIO_configPin`
```c
void GPIO_configPin(GPIO_TypeDef *port, uint8_t pin, gpio_config_t config);
```
Nastaví konfiguráciu jedného pinu (pin 0–15) zápisom do `CRL` (piny 0–7) alebo `CRH` (piny 8–15).

#### `gpio_config_t` – dostupné hodnoty

| Hodnota | Popis |
|---|---|
| `GPIO_CFG_INPUT_ANALOG` | Analógový vstup |
| `GPIO_CFG_INPUT_FLOATING` | Vstup s floating impedanciou |
| `GPIO_CFG_INPUT_PULL` | Vstup s pull-up / pull-down |
| `GPIO_CFG_OUTPUT_PP_10MHZ` | Push-Pull výstup 10 MHz |
| `GPIO_CFG_OUTPUT_PP_2MHZ` | Push-Pull výstup 2 MHz |
| `GPIO_CFG_OUTPUT_PP_50MHZ` | Push-Pull výstup 50 MHz |
| `GPIO_CFG_OUTPUT_OD_10MHZ` | Open-Drain výstup 10 MHz |
| `GPIO_CFG_OUTPUT_OD_2MHZ` | Open-Drain výstup 2 MHz |
| `GPIO_CFG_OUTPUT_OD_50MHZ` | Open-Drain výstup 50 MHz |
| `GPIO_CFG_OUTPUT_AF_PP_*` | Alternate Function Push-Pull |
| `GPIO_CFG_OUTPUT_AF_OD_*` | Alternate Function Open-Drain |

## Použitie

```c
GPIO_enableClock(GPIOB);
GPIO_configPin(GPIOB, 7, GPIO_CFG_OUTPUT_OD_50MHZ);
GPIOB->BSRR = (1u << 7);   // nastav pin HIGH (open-drain → uvoľni)
GPIOB->BRR  = (1u << 7);   // nastav pin LOW  (ťahaj dolu)
```

## Poznámky
- Pull-up / pull-down pri `GPIO_CFG_INPUT_PULL` sa volí zápisom do `ODR` (1 = pull-up, 0 = pull-down).
- Pre open-drain výstupy je nutný externý pull-up rezistor.
