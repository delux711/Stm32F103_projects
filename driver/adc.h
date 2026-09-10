#ifndef ADC_H
#define ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32f10x.h"

typedef enum
{
    ADC_LIGHT_DARK = 0,
    ADC_LIGHT_TWILIGHT,
    ADC_LIGHT_BRIGHT
} ADC_lightLevel_t;

typedef struct
{
    ADC_TypeDef  *adc;
    GPIO_TypeDef *port;
    uint8_t       pin;
    uint8_t       channel;
    uint8_t       sample_time;
    uint16_t      dark_max;
    uint16_t      bright_min;
    uint16_t      vref_mv;
} ADC_config_t;

void ADC_init(const ADC_config_t *config);
uint16_t ADC_readRaw(void);
uint16_t ADC_readAverage(uint8_t samples);
uint16_t ADC_readMilliVolts(void);
uint16_t ADC_readMilliVoltsChannel(uint8_t channel, uint8_t sample_time);

void ADC_setThresholds(uint16_t dark_max, uint16_t bright_min);
ADC_lightLevel_t ADC_classifyRaw(uint16_t raw);
ADC_lightLevel_t ADC_readLightLevel(void);
const char *ADC_lightLevelToString(ADC_lightLevel_t level);

uint16_t ADC_readInternalTempRaw(void);
int16_t ADC_readInternalTempC10(void);

#ifdef __cplusplus
}
#endif

#endif
