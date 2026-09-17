# ADC Ovládač - GL5537 Fotoresistor a Interný Teplotný Senzor

## Prehľad
ADC driver pre STM32F103 s podporou svetelného senzora GL5537 (fotoresistor) a interného teplotného senzora MCU.

## Hardware konfigurácia
- **ADC1 kanál 0 (PA0)**: GL5537 fotoresistor s delič napätia
- **ADC1 kanál 16**: Interný teplotný senzor STM32F103
- **Referenčné napätie**: 3.3V (interný VREF)

## Schéma zapojenia
```
PA0 ──┬─── ADC1_IN0
      │
      ├── GL5537 (3.3V strana) ── VREF
      │
      └── 10k R ── GND
```

## Štruktúra konfigurácie
```c
typedef struct
{
    ADC_TypeDef *adc;
    uint8_t channel;
    uint16_t sample_time;
    uint16_t vref_mv;
    uint16_t dark_max;
    uint16_t bright_min;
} ADC_config_t;
```

## API

### `void ADC_init(const ADC_config_t *config)`
Inicializácia ADC a GPIO pre analógový vstup.

**Príklad**:
```c
const ADC_config_t adc_cfg = {
    .adc = ADC1,
    .channel = 0,
    .sample_time = 55,
    .vref_mv = 3300,
    .dark_max = 1400,
    .bright_min = 2800
};
ADC_init(&adc_cfg);
```

### `uint16_t ADC_readRaw(void)`
Čítanie raw 12-bitovej ADC hodnoty (0–4095).

**Príklad**:
```c
uint16_t raw = ADC_readRaw();  // Vracia 0–4095
```

### `uint16_t ADC_readAverage(uint16_t samples)`
Čítanie priemeru špecifikovaného počtu vzoriek.

**Príklad**:
```c
uint16_t avg = ADC_readAverage(10);  // Priemer 10 vzoriek
```

### `uint16_t ADC_readMilliVolts(void)`
Čítanie konvertovanej hodnoty v mV (s Single Point Calibration).

**Vracia**: Napätie v mV (0–3300)

**Príklad**:
```c
uint16_t mv = ADC_readMilliVolts();  // Vracia 0–3300 mV
```

### `uint16_t ADC_readMilliVoltsChannel(uint8_t channel, uint8_t sample_time)`
Čítanie napätia z ľubovoľného kanála s vlastným sample time.

### `ADC_lightLevel_t ADC_classifyRaw(uint16_t raw)`
Klasifikácia svetelnej úrovne na základe raw ADC hodnoty.

**Vracia**:
```c
typedef enum {
    ADC_LIGHT_DARK = 0,
    ADC_LIGHT_TWILIGHT = 1,
    ADC_LIGHT_BRIGHT = 2
} ADC_lightLevel_t;
```

**Prahy** (konfigurovateľné):
- Ak raw < dark_max → DARK
- Ak raw medzi dark_max a bright_min → TWILIGHT
- Ak raw > bright_min → BRIGHT

**Príklad**:
```c
uint16_t raw = ADC_readRaw();
ADC_lightLevel_t light = ADC_classifyRaw(raw);

switch (light) {
    case ADC_LIGHT_DARK:     printf("Tmavé\r\n"); break;
    case ADC_LIGHT_TWILIGHT: printf("Šero\r\n"); break;
    case ADC_LIGHT_BRIGHT:   printf("Svetlé\r\n"); break;
}
```

## Interný teplotný senzor

### `uint16_t ADC_readInternalTempRaw(void)`
Čítanie raw ADC hodnoty interného teplotného senzora (kanál 16).

**Poznámka**: TSVREFE bit sa zapne a vypne automaticky počas čítania (efektívny spotreba energie).

### `int16_t ADC_readInternalTempC10(void)`
Čítanie internej teploty v jednotkách C×10 (napr. 253 = 25.3°C).

**Kalibrácia** (typické STM32F103 hodnoty):
- V25 = 1430 mV
- Sklon = 4.3 mV/°C

**Príklad**:
```c
int16_t temp_c10 = ADC_readInternalTempC10();
printf("Teplota: %d.%d°C\r\n", temp_c10 / 10, temp_c10 % 10);
// Ak temp_c10 = 253, výstup: "Teplota: 25.3°C"
```

## Konfigurácia v app_config.h
```c
#define DRIVER_ADC_USE  1   // Povolí ADC driver
```

## Príklad - Test aplikácia (test_adc_light)

```c
#include "adc.h"
#include "systick.h"
#include "debug.h"

int main(void)
{
    SystemInit();
    SysTick_Config(SystemCoreClock / 1000);
    DEBUG_initTrace(SystemCoreClock);

    const ADC_config_t cfg = {
        .adc = ADC1,
        .channel = 0,
        .sample_time = 55,
        .vref_mv = 3300,
        .dark_max = 1400,
        .bright_min = 2800
    };

    ADC_init(&cfg);

    while (1) {
        uint16_t raw = ADC_readRaw();
        uint16_t mv = ADC_readMilliVolts();
        ADC_lightLevel_t light = ADC_classifyRaw(raw);
        int16_t temp = ADC_readInternalTempC10();

        printf("RAW: %d, mV: %d, Light: %d, Temp: %d.%d°C\r\n",
               raw, mv, light, temp / 10, temp % 10);

        HAL_Delay(1000);  // Každú sekundu
    }
}
```

## Memory Usage
- FLASH: ~1.5 KB (kód)
- RAM: ~100 B (globálne premenné)

## Integrácia s Device Registry

```c
// Samozrejme mapovať ADC do MODBUS registrov:
static const RegistryEntry_t adc_registry[] = {
    { 0, "LIGHT_RAW",  REGISTRY_TYPE_RO, ADC_readRaw,          NULL },
    { 1, "LIGHT_MV",   REGISTRY_TYPE_RO, ADC_readMilliVolts,   NULL },
    { 2, "LIGHT_LEVEL",REGISTRY_TYPE_RO, (uint16_t(*)(void))ADC_classifyRaw, NULL },
    { 3, "TEMP_C10",   REGISTRY_TYPE_RO, (uint16_t(*)(void))ADC_readInternalTempC10, NULL },
    REGISTRY_END
};

DeviceRegistry_init(adc_registry);
```
