# Vzor Device Registry

## Prehľad
Flexibilný mechanizmus na registráciu senzorů/ovládačů bez hardcodeingu MODBUS logiky.
Mapuje názvy senzorů a funkcie čítania/zápisu na adresy MODBUS registrov.

## Architektúra

Namiesto písania MODBUS handlérov pre každú aplikáciu poskytuje `DeviceRegistry_*`:

```c
// apps/test_adc_light/main.c
static uint16_t read_adc_raw(void) { return ADC_readRaw(); }
static uint16_t read_adc_mv(void)  { return ADC_readMilliVolts(); }

static const RegistryEntry_t registry[] = {
    { 0, "ADC_RAW",  REGISTRY_TYPE_RO, read_adc_raw,  NULL },
    { 1, "ADC_MV",   REGISTRY_TYPE_RO, read_adc_mv,   NULL },
    { 2, "TEMP_C10", REGISTRY_TYPE_RO, read_temp_c10, NULL },
    REGISTRY_END
};

int main(void)
{
    // ...
    DeviceRegistry_init(registry);
    ModbusSlave_init(&mb_cfg, device_cfg->modbus_slave_addr);
}
```

**MODBUS master teraz číta**:
- Register 0 → ADC_raw hodnota
- Register 1 → ADC napätie v mV
- Register 2 → Teplota v C×10

## Štruktúra

```c
typedef struct
{
    uint16_t address;           // MODBUS adresa registra
    const char *name;           // Opisný názov (na logging)
    RegistryType_t type;        // RO alebo RW
    DeviceRegistry_readFunc_t read;   // Funkcia čítania (vracia uint16_t)
    DeviceRegistry_writeFunc_t write; // Funkcia zápisu (NULL pre RO)
} RegistryEntry_t;
```

## API

### `void DeviceRegistry_init(const RegistryEntry_t *entries)`
Inicializácia registry s tabuľkou senzorů/registrov.

**Parametre**:
- `entries`: Pole RegistryEntry_t, ukončené makrom REGISTRY_END

**Príklad**:
```c
static const RegistryEntry_t device_sensors[] = {
    { 0, "Light", REGISTRY_TYPE_RO, read_light_level, NULL },
    { 1, "Setpoint", REGISTRY_TYPE_RW, read_setpoint, write_setpoint },
    REGISTRY_END
};

DeviceRegistry_init(device_sensors);
```

### `const RegistryEntry_t *DeviceRegistry_findByAddress(uint16_t addr)`
Vyhľadávanie registry položky podľa MODBUS adresy.

**Vracia**: Ukazovateľ na položku, alebo NULL, ak sa nenajde

### `uint16_t DeviceRegistry_read(uint16_t addr)`
Čítanie hodnoty z registry položky.

**Parametre**:
- `addr`: MODBUS adresa registra

**Vracia**: 16-bitová hodnota senzora

**Príklad**:
```c
uint16_t light_val = DeviceRegistry_read(0);  // Číta úroveň svetla
```

### `void DeviceRegistry_write(uint16_t addr, uint16_t value)`
Zápis hodnoty do registry položky (iba RW).

**Parametre**:
- `addr`: MODBUS adresa registra
- `value`: 16-bitová hodnota na zápis

**Príklad**:
```c
DeviceRegistry_write(1, 2500);  // Nastaví setpoint na 2500mV
```

## Výhody

1. **Bez boilerplate kódu**: Každá nová aplikácia len definuje svoju registry, bez MODBUS copy-paste
2. **Flexibilnosť**: Pridávanie/odoberanie senzorů úpravou len registry pola
3. **Bezpečnosť typov**: Funkcie čítania/zápisu zaručujú správnu manipuláciu s údajmi
4. **Scalability**: 15-uzlový systém používa rovnaký vzor pre každý uzol
5. **Debugging**: Názvy senzorů sú vytlačené v debug výstupe

## Príklad: Zariadenie s viacerými senzormi

```c
// Zariadenie so svetlom, teplotou a tlačítkom
static RegistryEntry_t light_registry[] = {
    { 0, "LightRaw", REGISTRY_TYPE_RO, ADC_readRaw, NULL },
    { 1, "LightMV", REGISTRY_TYPE_RO, ADC_readMilliVolts, NULL },
    { 2, "TempC10", REGISTRY_TYPE_RO, read_internal_temp, NULL },
    REGISTRY_END
};

static RegistryEntry_t button_registry[] = {
    { 0, "ButtonState", REGISTRY_TYPE_RO, button_read_state, NULL },
    { 1, "ClickCount", REGISTRY_TYPE_RO, button_read_clicks, NULL },
    REGISTRY_END
};

// Aplikácia si vyberá registry podľa device_type
if (cfg->device_type & 0x01) {
    DeviceRegistry_init(light_registry);
} else {
    DeviceRegistry_init(button_registry);
}
```

## Tok integrácie

```
MODBUS Master posiela FC3 (Read Regs) pre addr 0x0002
         ↓
ModbusSlave_init() nastaví callback na čítanie
         ↓
Callback volá DeviceRegistry_read(0x0002)
         ↓
Registry nájde položku pre addr 0x0002
         ↓
Volá ukazovateľ na funkciu čítania (napr. read_internal_temp())
         ↓
Vracia 253 (25.3°C)
         ↓
ModbusSlave zapakuje do odpovede FC3
         ↓
Posiela späť master-u
```
