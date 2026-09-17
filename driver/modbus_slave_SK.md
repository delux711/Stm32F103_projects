# MODBUS Slave Adaptér

## Prehľad
Wrapper okolo MODBUS RTU ovládača, ktorý automaticky mapuje Device Registry na MODBUS funkčné kódy 3 (Čítanie držiacich registrov) a 16 (Zápis viacerých registrov).

## Dizajn
Namiesto písania MODBUS handlérov pre každú aplikáciu poskytuje `ModbusSlave_*`:
1. Jednoduchú inicializáciu s registry-based callbackmi
2. Automatické zvládnutie FC3/FC16 cez device registry
3. Voliteľné vlastné handléry na čítanie/zápis pre pokročilé scenáre

## Architektúra

```
UART RX → MODBUS_RTU_process()
            ↓ (volá registrovaný callback)
         ModbusSlave_readHoldingReg()
            ↓
         DeviceRegistry_read(adresa)
            ↓
         Ukazovateľ na funkciu čítania
            ↓
         Vracia 16-bitovú hodnotu → MODBUS_RTU_process() → TX odpoveď
```

## API

### `void ModbusSlave_init(const MODBUS_RTU_config_t *config, uint8_t slave_addr)`
Inicializácia MODBUS slave s registry-based callbackmi.

**Parametre**:
- `config`: MODBUS_RTU_config_t s interframe timeoutom, max registrami
- `slave_addr`: MODBUS slave adresa (1-247)

**Príklad**:
```c
MODBUS_RTU_config_t mb_cfg = {
    .interframe_timeout_ms = 100,
    .max_registers_per_request = 100
};
ModbusSlave_init(&mb_cfg, 10);  // Slave adresa 10
```

### `void ModbusSlave_setRegisterReadHandler(ModbusSlave_regGetFunc_t handler)`
Nastavenie vlastného handloru čítania (voliteľné, registry sa používa predvolene).

**Parametre**:
- `handler`: Callback funkcia so signaturou `int handler(uint16_t addr, uint16_t *value)`

### `void ModbusSlave_setRegisterWriteHandler(ModbusSlave_regSetFunc_t handler)`
Nastavenie vlastného handloru zápisu (voliteľné, registry sa používa predvolene).

**Parametre**:
- `handler`: Callback funkcia so signaturou `void handler(uint16_t addr, uint16_t value)`

### `void ModbusSlave_process(void)`
Spracovanie čakajúcich MODBUS požiadaviek. Volať z hlavného cyklu.

**Príklad**:
```c
while (1) {
    ModbusSlave_process();
    HAL_Delay(1);
}
```

## Typické použitie

**Pre jednoduché aplikácie (iba registry)**:
```c
int main(void)
{
    SystemInit();
    DeviceConfig_init();

    // Definujte registry
    static const RegistryEntry_t my_registry[] = {
        { 0, "Sensor1", REGISTRY_TYPE_RO, read_sensor1, NULL },
        { 1, "Sensor2", REGISTRY_TYPE_RO, read_sensor2, NULL },
        REGISTRY_END
    };

    DeviceRegistry_init(my_registry);

    // Inicializujte MODBUS
    MODBUS_RTU_config_t mb_cfg = { .interframe_timeout_ms = 100, .max_registers_per_request = 100 };
    ModbusSlave_init(&mb_cfg, 10);

    // Hlavný cyklus
    while (1) {
        ModbusSlave_process();
    }
}
```

**Pre pokročilé aplikácie (vlastné handléry)**:
```c
static int my_custom_read(uint16_t addr, uint16_t *value)
{
    // Vlastná logika na register
    if (addr == 100) {
        *value = get_calibration_factor();
    } else {
        return DeviceRegistry_read(addr);  // Prejdite na registry
    }
    return 0;
}

int main(void)
{
    // ... nastaviť registry ...
    ModbusSlave_init(&mb_cfg, 10);
    ModbusSlave_setRegisterReadHandler(my_custom_read);
}
```

## Podporované MODBUS funkčné kódy

### FC3: Čítanie držiacich registrov
- Požiadavka až 125 registrov naraz
- Každý register je 16 bitov
- Device registry alebo vlastný handler poskytuje hodnoty

**Príklad MODBUS rámca**:
```
Požiadavka:  [10] [03] [0000] [0002] CRC  → Čítanie 2 registrov z adresy 0
Odpoveď:     [10] [03] [04] [0xHH] [0xLL] [0xHH] [0xLL] CRC
```

### FC16: Zápis viacerých registrov
- Zápis až 123 registrov naraz
- Volá handler zápisu pre každú adresu
- Vracia echo požiadavky plus CRC

**Príklad MODBUS rámca**:
```
Požiadavka:  [10] [10] [0064] [0001] [02] [0x1234] CRC  → Zápis 1 registra na addr 0x64
Odpoveď:     [10] [10] [0064] [0001] CRC
```

## Integrácia s Device Config

**Pri spustení použite MODBUS adresu z device config**:
```c
DeviceConfig_init();
DeviceConfig_t *cfg = DeviceConfig_get();
ModbusSlave_init(&mb_cfg, cfg->modbus_slave_addr);
```

**Zmena MODBUS adresy za runtime**:
```c
// Master píše do špecifického registra (napr. addr 0xF0) na zmenu slave adresy
// Aplikácia číta device_config, aktualizuje ho, uloží do EEPROM:
cfg->modbus_slave_addr = new_addr;
DeviceConfig_saveToEEPROM();
// Reštartujte zariadenie, aby sa zmena prejavila
```

## Spracovanie chýb
- Neplatné adresy registrov vrátia 0 (alebo vlastný handler rozhodne)
- RO registre odmietnu zápisy ticho
- Zle formované MODBUS rámce sú ignorované (CRC check v MODBUS_RTU)
