Implementovaný je nový samostatný Modbus RTU modul nad RS485 transportom.

Zmeny:
1. Pridané nové súbory:
- modbus_rtu.h
- modbus_rtu.c

2. Modul je navrhnutý ako slave nad RS485:
- registruje RX callback cez RS485_setRxCallback v modbus_rtu.c
- odpovede posiela cez RS485_send v modbus_rtu.c
- skladá RTU rámec z prijatých bajtov a ukončuje ho cez inter-frame timeout v modbus_rtu.c
- kontroluje CRC16 Modbus v modbus_rtu.c

3. Podporené funkcie:
- 0x03 Read Holding Registers v modbus_rtu.c
- 0x06 Write Single Register v modbus_rtu.c
- exception odpovede (0x81/0x83/0x86 podľa requestu) v modbus_rtu.c

4. Konfigurovateľné parametre v modbus_rtu.h:
- slave address
- interframe timeout
- max registers per request
- callback na čítanie holding registra
- callback na zápis holding registra
- user context pointer

Krátke použitie:
```c
#include "modbus_rtu.h"

static bool AppReadReg(void *ctx, uint16_t addr, uint16_t *value) { /* ... */ return true; }
static bool AppWriteReg(void *ctx, uint16_t addr, uint16_t value) { /* ... */ return true; }

MODBUS_RTU_config_t cfg = {
    .slave_address = 1u,
    .interframe_timeout_ms = 5u,
    .max_registers_per_request = 32u,
    .read_holding_reg = AppReadReg,
    .write_holding_reg = AppWriteReg,
    .context = 0
};

RS485_init(&rs485_config);
MODBUS_RTU_init(&cfg);

while (1)
{
    MODBUS_RTU_process();
    __WFI();
}
```

Poznámka:
- Tvoja existujúca appka s command modulom zostala bez zásahu.
- V Keil projekte treba pridať nový súbor modbus_rtu.c, ak sa nepridáva automaticky.

1. Ak chceš, v ďalšom kroku ti to rovno napojím do main.c namiesto command parsera.
2. Môžem hneď doplniť aj funkciu 0x10 Write Multiple Registers pre praktickejší Modbus slave.

Made changes.