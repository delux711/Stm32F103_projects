#include "app_config.h"
#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "systick.h"
#include "ds18b20.h"
#include "debug.h"

#define DS_PORT           GPIOB
#define DS_PIN            7u
#define DS_CPU_FREQ_HZ    72000000u

static const DS18B20_config_t ds_cfg = {
    .ow = {
        .port = DS_PORT,
        .pin = DS_PIN,
        .cpu_freq_hz = DS_CPU_FREQ_HZ
    },
    .use_rom_match = 0u
};

static void APP_printTempC10(int16_t temp_c10)
{
    char buf[20];
    uint8_t idx = 0u;
    int16_t whole;
    int16_t frac;

    whole = temp_c10 / 10;
    frac = temp_c10 % 10;
    if (frac < 0)
    {
        frac = -frac;
    }

    if (whole < 0)
    {
        buf[idx++] = '-';
        whole = -whole;
    }

    if (whole >= 100)
    {
        buf[idx++] = (char)('0' + (whole / 100));
        whole %= 100;
    }
    if ((idx > 0u) || (whole >= 10))
    {
        buf[idx++] = (char)('0' + (whole / 10));
    }
    buf[idx++] = (char)('0' + (whole % 10));
    buf[idx++] = '.';
    buf[idx++] = (char)('0' + frac);
    buf[idx++] = ' ';
    buf[idx++] = 'C';
    buf[idx++] = '\r';
    buf[idx++] = '\n';
    buf[idx] = '\0';

    DEBUG_writeString(buf);
}

int main(void)
{
    int16_t temp_c10;
    DS18B20_status_t st;

    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u);

    DEBUG_initTrace(SystemCoreClock);
    DS18B20_init(&ds_cfg);

    DEBUG_writeString("DS18B20 test start\r\n");

    while (1)
    {
        st = DS18B20_startConversion();
        if (st != DS18B20_OK)
        {
            DEBUG_writeString("DS18B20: no device\r\n");
            SYS_delayMs(1000u);
            continue;
        }

        SYS_delayMs(750u);

        st = DS18B20_readTemperatureC10(&temp_c10);
        if (st == DS18B20_OK)
        {
            DEBUG_writeString("Temp: ");
            APP_printTempC10(temp_c10);
            DEBUG_ledPinToggle();
        }
        else if (st == DS18B20_ERR_CRC)
        {
            DEBUG_writeString("DS18B20: CRC error\r\n");
        }
        else
        {
            DEBUG_writeString("DS18B20: read error\r\n");
        }

        SYS_delayMs(1000u);
    }
}