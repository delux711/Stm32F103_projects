# adc

Driver pre interny ADC (STM32F103) s pomocnou klasifikaciou osvetlenia pre fotoresistor (napr. GL5537).

## Subory
- adc.h - verejne API
- adc.c - implementacia

## Co robi
- inicializuje ADC1/ADC2 kanal
- cita surovu 12-bit hodnotu (0..4095)
- cita priemer z viacerych vzoriek
- prepocita hodnotu na mV
- klasifikuje uroven osvetlenia: tma / sero / svetlo
- cita interny teplotny senzor MCU (ADC channel 16)

## Zapojenie GL5537
Typicky delic napatia:
- GL5537 -> 3.3V
- rezistor (napr. 10k) -> GND
- stred delica -> ADC pin (napr. PA0 = ADC1_IN0)

Poznamka: Podla orientacie delica moze pri svetle hodnota rast alebo klesat.
Prahy dark_max/bright_min nastav podla realneho merania.

## Zakladna konfiguracia
```c
#include "adc.h"

static const ADC_config_t adc_cfg = {
    .adc = ADC1,
    .port = GPIOA,
    .pin = 0u,
    .channel = 0u,
    .sample_time = 2u,
    .dark_max = 1300u,
    .bright_min = 2800u,
    .vref_mv = 3300u
};

ADC_init(&adc_cfg);
```

## Pouzitie
```c
uint16_t raw = ADC_readAverage(8u);
ADC_lightLevel_t level = ADC_classifyRaw(raw);

if (level == ADC_LIGHT_DARK) {
    // tma
} else if (level == ADC_LIGHT_TWILIGHT) {
    // sero
} else {
    // svetlo
}
```

## API
- ADC_init
- ADC_readRaw
- ADC_readAverage
- ADC_readMilliVolts
- ADC_readMilliVoltsChannel
- ADC_setThresholds
- ADC_classifyRaw
- ADC_readLightLevel
- ADC_lightLevelToString
- ADC_readInternalTempRaw
- ADC_readInternalTempC10

## Napatie na definovanom vstupe
```c
// napr. kanal ADC1_IN5, sample time 55.5 cyklov (0x2)
uint16_t vin_mv = ADC_readMilliVoltsChannel(5u, 2u);
```

Poznamka:
- funkcia cita priamo z uvedeneho ADC kanala (0..17)
- kanal pre externy pin musi byt fyzicky spravne zapojeny

## Interny teplotny senzor
```c
int16_t t_c10 = ADC_readInternalTempC10();
// napr. 253 = 25.3 C
```

Poznamky:
- pouziva sa typicka linearizacia pre STM32F1 (V25 a slope)
- presnost je orientacna, vhodna na trend/monitoring
- pre lepsiu stabilitu je vhodne priemerovanie viacerych citani
- interny teplotny senzor je aktivovany iba pocas merania a potom sa vypne
