# debug

Pomocný modul pre ladenie cez SEGGER RTT a SWO ITM, plus ovládanie debug LED (PC13).

## Súbory
- `debug.h` – verejné API (vrátane inline funkcií)
- `debug.c` – inicializácia SWO a SEGGER RTT

## Inicializácia

```c
DEBUG_initTrace(SystemCoreClock);
```

Nastaví:
- LED pin **PC13** ako výstup
- SWO pin **PB3** ako Alternate Function Push-Pull 50 MHz (JTAG vypnutý, SWD zachovaný)
- SEGGER RTT (cez knižnicu `SEGGER_RTT`)

## API

### Výstup textu

| Funkcia | Popis |
|---|---|
| `DEBUG_writeString(const char *s)` | Odošle reťazec cez SEGGER RTT kanál 0 |
| `DEBUG_writeChar(char c)` | Odošle jeden znak cez SEGGER RTT |
| `DEBUG_sendChar(ch, channel)` | Odošle bajt cez ITM port (inline, kontroluje debugger) |
| `DEBUG_sendString(str, channel)` | Odošle reťazec cez ITM port (inline, kontroluje debugger) |

> `DEBUG_sendChar` / `DEBUG_sendString` kontrolujú pred odoslaním, či je debugger pripojený a ITM povolený.
> `DEBUG_sendCharInternal` – nekontroluje, posiela vždy (nízka úroveň).

---

### LED (PC13)

| Funkcia | Popis |
|---|---|
| `DEBUG_ledPinToggle()` | Prepne LED (inline) |
| `DEBUG_ledPinOn()` | Zapne LED – `BS13` |
| `DEBUG_ledPinOff()` | Vypne LED – `BR13` |
| `DEBUG_setPin(bool on)` | Nastaví LED podľa hodnoty |
| `DEBUG_getPin()` | Vráti aktuálny stav pinu PC13 |

## Použitie

```c
DEBUG_initTrace(SystemCoreClock);

DEBUG_writeString("Start\r\n");
DEBUG_ledPinOn();
SYS_delayMs(500u);
DEBUG_ledPinOff();

// ITM cez Ozone / Keil debugger
DEBUG_sendString("Hello ITM\r\n", 0);
```

## Poznámky
- SEGGER RTT nevyžaduje fyzické prepojenie SWO – funguje cez SWDIO/SWDCLK.
- ITM (`DEBUG_sendChar`, `DEBUG_sendString`) vyžaduje aktívne debugger spojenie.
- LED je aktívna v LOW (PC13 na Blue Pill je invertovaná).
