#include "app_config.h"
#include "adc.h"
#include "debug.h"
#include "debug_rtt_levels.h"
#include "gpio.h"

#ifndef ADC_DBG
#  define ADC_DBG(msg) rtt_debug(msg)
#endif

#define ADC_SAMPLE_55CYCLES_5  (0x2u)
#define ADC_SAMPLE_239CYCLES_5 (0x7u)
#define ADC_DEFAULT_DARK_MAX   (1400u)
#define ADC_DEFAULT_BRIGHT_MIN (2800u)
#define ADC_DEFAULT_VREF_MV    (3300u)

#define ADC_CH_TEMP_SENSOR     (16u)
#define ADC_TS_V25_MV          (1430)
#define ADC_TS_SLOPE_UV_C      (4300)

static ADC_config_t adc_cfg;

static void ADC_enableClock(ADC_TypeDef *adc)
{
    if (adc == ADC1)
    {
        RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    }
    else if (adc == ADC2)
    {
        RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;
    }
}

static void ADC_setSampleTime(uint8_t channel, uint8_t sample_time)
{
    uint32_t shift;
    uint32_t reg;

    if (channel <= 9u)
    {
        shift = (uint32_t)channel * 3u;
        reg = adc_cfg.adc->SMPR2;
        reg &= ~(0x7u << shift);
        reg |= ((uint32_t)(sample_time & 0x7u) << shift);
        adc_cfg.adc->SMPR2 = reg;
    }
    else
    {
        shift = (uint32_t)(channel - 10u) * 3u;
        reg = adc_cfg.adc->SMPR1;
        reg &= ~(0x7u << shift);
        reg |= ((uint32_t)(sample_time & 0x7u) << shift);
        adc_cfg.adc->SMPR1 = reg;
    }
}

static uint16_t ADC_rawToMv(uint16_t raw)
{
    uint32_t mv;

    mv = (uint32_t)raw * adc_cfg.vref_mv;
    mv /= 4095u;
    return (uint16_t)mv;
}

static uint16_t ADC_readChannelRaw(uint8_t channel, uint8_t sample_time)
{
    adc_cfg.adc->SQR3 = channel;
    ADC_setSampleTime(channel, sample_time);

    adc_cfg.adc->CR2 |= ADC_CR2_ADON;
    adc_cfg.adc->CR2 |= ADC_CR2_SWSTART;

    while ((adc_cfg.adc->SR & ADC_SR_EOC) == 0u) { }

    return (uint16_t)(adc_cfg.adc->DR & 0xFFFFu);
}

void ADC_init(const ADC_config_t *config)
{
    uint8_t sample_time;

    if ((config == 0) || (config->adc == 0) || (config->port == 0) || (config->pin > 15u) || (config->channel > 17u))
    {
        return;
    }

    adc_cfg = *config;

    sample_time = adc_cfg.sample_time & 0x7u;
    if (sample_time == 0u)
    {
        sample_time = ADC_SAMPLE_55CYCLES_5;
    }
    adc_cfg.sample_time = sample_time;

    if (adc_cfg.dark_max == 0u)
    {
        adc_cfg.dark_max = ADC_DEFAULT_DARK_MAX;
    }
    if (adc_cfg.bright_min == 0u)
    {
        adc_cfg.bright_min = ADC_DEFAULT_BRIGHT_MIN;
    }
    if (adc_cfg.bright_min <= adc_cfg.dark_max)
    {
        adc_cfg.bright_min = adc_cfg.dark_max + 1u;
    }
    if (adc_cfg.vref_mv == 0u)
    {
        adc_cfg.vref_mv = ADC_DEFAULT_VREF_MV;
    }

    GPIO_enableClock(adc_cfg.port);
    ADC_enableClock(adc_cfg.adc);

    GPIO_configPin(adc_cfg.port, adc_cfg.pin, GPIO_CFG_INPUT_ANALOG);

    adc_cfg.adc->CR1 = 0u;
    adc_cfg.adc->CR2 = ADC_CR2_EXTTRIG | (0x7u << 17);
    adc_cfg.adc->SQR1 = 0u;
    adc_cfg.adc->SQR2 = 0u;
    adc_cfg.adc->SQR3 = adc_cfg.channel;

    ADC_setSampleTime(adc_cfg.channel, adc_cfg.sample_time);

    adc_cfg.adc->CR2 |= ADC_CR2_ADON;

    adc_cfg.adc->CR2 |= ADC_CR2_RSTCAL;
    while ((adc_cfg.adc->CR2 & ADC_CR2_RSTCAL) != 0u) { }

    adc_cfg.adc->CR2 |= ADC_CR2_CAL;
    while ((adc_cfg.adc->CR2 & ADC_CR2_CAL) != 0u) { }

    ADC_DBG("ADC init\r\n");
}

uint16_t ADC_readRaw(void)
{
    if (adc_cfg.adc == 0)
    {
        return 0u;
    }

    return ADC_readChannelRaw(adc_cfg.channel, adc_cfg.sample_time);
}

uint16_t ADC_readAverage(uint8_t samples)
{
    uint32_t sum = 0u;
    uint8_t count;

    if (samples == 0u)
    {
        samples = 1u;
    }

    for (count = 0u; count < samples; count++)
    {
        sum += ADC_readRaw();
    }

    return (uint16_t)(sum / samples);
}

uint16_t ADC_readMilliVolts(void)
{
    return ADC_rawToMv(ADC_readRaw());
}

uint16_t ADC_readMilliVoltsChannel(uint8_t channel, uint8_t sample_time)
{
    uint16_t raw;

    if ((adc_cfg.adc == 0) || (channel > 17u))
    {
        return 0u;
    }

    raw = ADC_readChannelRaw(channel, (uint8_t)(sample_time & 0x7u));
    return ADC_rawToMv(raw);
}

void ADC_setThresholds(uint16_t dark_max, uint16_t bright_min)
{
    adc_cfg.dark_max = dark_max;
    adc_cfg.bright_min = bright_min;

    if (adc_cfg.bright_min <= adc_cfg.dark_max)
    {
        adc_cfg.bright_min = adc_cfg.dark_max + 1u;
    }
}

ADC_lightLevel_t ADC_classifyRaw(uint16_t raw)
{
    if (raw <= adc_cfg.dark_max)
    {
        return ADC_LIGHT_DARK;
    }

    if (raw < adc_cfg.bright_min)
    {
        return ADC_LIGHT_TWILIGHT;
    }

    return ADC_LIGHT_BRIGHT;
}

ADC_lightLevel_t ADC_readLightLevel(void)
{
    return ADC_classifyRaw(ADC_readRaw());
}

const char *ADC_lightLevelToString(ADC_lightLevel_t level)
{
    switch (level)
    {
        case ADC_LIGHT_DARK:
            return "tma";
        case ADC_LIGHT_TWILIGHT:
            return "sero";
        case ADC_LIGHT_BRIGHT:
            return "svetlo";
        default:
            return "nezname";
    }
}

uint16_t ADC_readInternalTempRaw(void)
{
    uint16_t raw;

    if (adc_cfg.adc == 0)
    {
        return 0u;
    }

    adc_cfg.adc->CR2 |= ADC_CR2_TSVREFE;
    raw = ADC_readChannelRaw(ADC_CH_TEMP_SENSOR, ADC_SAMPLE_239CYCLES_5);
    adc_cfg.adc->CR2 &= ~ADC_CR2_TSVREFE;

    return raw;
}

int16_t ADC_readInternalTempC10(void)
{
    uint32_t vsense_mv;
    uint16_t raw;
    int32_t temp_c10;

    raw = ADC_readInternalTempRaw();
    vsense_mv = ADC_rawToMv(raw);

    temp_c10 = (((int32_t)ADC_TS_V25_MV - (int32_t)vsense_mv) * 10000) / ADC_TS_SLOPE_UV_C;
    temp_c10 += 250;

    return (int16_t)temp_c10;
}
