#include "app_config.h"
#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "systick.h"
#include "adc.h"
#include "debug.h"
#include "debug_rtt_levels.h"

#define LDR_PORT        GPIOA
#define LDR_PIN         0u
#define LDR_CHANNEL     0u

#define ADC_VREF_MV     3300u
#define ADC_DARK_MAX    1400u
#define ADC_BRIGHT_MIN  2800u

static const ADC_config_t adc_cfg = {
    .adc = ADC1,
    .port = LDR_PORT,
    .pin = LDR_PIN,
    .channel = LDR_CHANNEL,
    .sample_time = 2u,
    .dark_max = ADC_DARK_MAX,
    .bright_min = ADC_BRIGHT_MIN,
    .vref_mv = ADC_VREF_MV
};

static void APP_writeU16(uint16_t value)
{
    char buf[6];
    uint8_t i = 0u;

    if (value == 0u)
    {
        DEBUG_writeChar('0');
        return;
    }

    while ((value > 0u) && (i < (uint8_t)sizeof(buf)))
    {
        buf[i++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (i > 0u)
    {
        i--;
        DEBUG_writeChar(buf[i]);
    }
}

static void APP_writeTempC10(int16_t temp_c10)
{
    int16_t whole = temp_c10 / 10;
    int16_t frac = temp_c10 % 10;

    if (frac < 0)
    {
        frac = -frac;
    }

    if ((whole == 0) && (temp_c10 < 0))
    {
        DEBUG_writeChar('-');
    }

    if (whole < 0)
    {
        whole = -whole;
    }

    APP_writeU16((uint16_t)whole);
    DEBUG_writeChar('.');
    DEBUG_writeChar((char)('0' + frac));
}

int main(void)
{
    uint16_t ldr_raw;
    uint16_t ldr_mv;
    int16_t temp_c10;
    ADC_lightLevel_t light;

    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u);

    DEBUG_initTrace(SystemCoreClock);
    ADC_init(&adc_cfg);

    rtt_ok("ADC light test start\r\n");

    while (1)
    {
        ldr_raw = ADC_readAverage(8u);
        ldr_mv = ADC_readMilliVoltsChannel(LDR_CHANNEL, 2u);
        light = ADC_classifyRaw(ldr_raw);
        temp_c10 = ADC_readInternalTempC10();

        rtt_info("LDR raw=%u mv=%u level=%s temp=%d C\r\n",
                 ldr_raw, ldr_mv, ADC_lightLevelToString(light), temp_c10);

        SYS_delayMs(1000u);
    }
}
