# Konfigurácia Zariadenia a Systém ID

## Prehľad
Perzistentné úložisko pre identitu zariadenia, správu verzií firmware a MODBUS adresu slave.
Uložené v EEPROM (prvých 16 bajtov).

## Štruktúra
```c
typedef struct
{
    uint32_t magic;                    // 0xDEADBEEF
    uint8_t  device_id;                // Unikátne ID zariadenia (0-255)
    uint8_t  modbus_slave_addr;        // MODBUS RTU slave adresa (1-247)
    uint8_t  device_type;              // Maska typu zariadenia (konfig senzora)
    uint8_t  fw_version;               // Verzia firmware
    uint32_t hw_serial;                // Sériové číslo hardvéru
} DeviceConfig_t;
```

## Magic číslo
`DEVICE_CONFIG_MAGIC = 0xDEADBEEF` - validuje, že EEPROM je správne inicializované

## Maska typu zariadenia - príklad
```
Bit 0: ADC Light Sensor (GL5537)
Bit 1: DS18B20 Temperature Sensor
Bit 2: Button Input
Bit 3: RF433 Receiver
Bit 4: RS485 Interface
Bit 5: IR Receiver
Bit 6-7: Rezervované
```

## API

### `void DeviceConfig_init(void)`
Inicializácia systému konfigurácie zariadenia a EEPROM.

### `void DeviceConfig_setDefaults(uint8_t device_id, uint8_t modbus_addr, uint8_t dev_type)`
Nastavenie predvolených konfigurácií (používa sa, ak je EEPROM prázdny).

**Príklad**:
```c
DeviceConfig_setDefaults(5, 10, 0x03);  // Zariadenie 5, MODBUS addr 10, ADC+DS18B20
```

### `DeviceConfig_t *DeviceConfig_get(void)`
Získanie aktuálnej konfigurácie zariadenia (načítanie z EEPROM, ak ešte nie je načítané).

**Vracia**: Ukazovateľ na štruktúru aktuálnej konfigurácie

**Príklad**:
```c
DeviceConfig_t *cfg = DeviceConfig_get();
printf("MODBUS Adresa: %d\r\n", cfg->modbus_slave_addr);
```

### `int DeviceConfig_loadFromEEPROM(void)`
Načítanie konfigurácie z EEPROM. Vracia 0, ak je platná, -1 ak je EEPROM prázdny (načítajú sa predvolené hodnoty).

### `int DeviceConfig_saveToEEPROM(void)`
Uloženie aktuálnej konfigurácie do EEPROM. Vracia 0 pri úspechu, -1 pri zlyhání.

**Príklad - zmena slave adresy počas behu**:
```c
DeviceConfig_t *cfg = DeviceConfig_get();
cfg->modbus_slave_addr = 11;
DeviceConfig_saveToEEPROM();
```

## Vzor použitia

**Pri spustení**:
```c
int main(void)
{
    SystemInit();
    DEBUG_initTrace(SystemCoreClock);

    DeviceConfig_init();
    DeviceConfig_t *cfg = DeviceConfig_get();

    printf("Zariadenie %d, MODBUS Addr %d\r\n", cfg->device_id, cfg->modbus_slave_addr);

    // ... zvyšok inicializácie
}
```

**V systéme s viacerými zariadeniami**:
```c
// Každý STM32F103 má unikátnu DeviceConfig v EEPROM
// ESP32 master číta device IDs cez MODBUS register 0
// Smeruje príkazy na správny slave podľa device_id
```

## Integrácia s Device Registry
Maska typu zariadenia určuje, ktoré senzory sú aktívne:
- Registry číta device_type
- Filtruje registry položky na základe povolených senzorů
- Zmenšuje MODBUS register tabuľku pre možnosti zariadenia
