#include "app_config.h"
#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "systick.h"
#include "onewire.h"
#include "debug.h"

/* ---------------------------------------------------------------
 * 1-Wire test application
 *
 * Connects a DS18B20 (or any 1-Wire device) to PB7 with a 4.7 kΩ
 * pull-up resistor to 3.3 V.
 *
 * Every second the application:
 *   1. Performs a bus reset – reports presence/no-presence via RTT
 *   2. (First run) searches for all devices and prints ROM codes
 *   3. Starts a DS18B20 temperature conversion (Skip ROM)
 *   4. Waits 750 ms (12-bit conversion time)
 *   5. Reads the 9-byte scratchpad and prints the temperature
 * --------------------------------------------------------------- */

#define OW_PORT          GPIOB
#define OW_PIN           7u
#define OW_CPU_FREQ_HZ   72000000u

/* DS18B20 function commands */
#define DS18B20_CMD_CONVERT_T      0x44u
#define DS18B20_CMD_READ_SCRATCHPAD 0xBEu

#define MAX_DEVICES  8u

static OW_config_t ow_config = {
    .port        = OW_PORT,
    .pin         = OW_PIN,
    .cpu_freq_hz = OW_CPU_FREQ_HZ
};

static void APP_printHex(uint8_t val)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[3];
    buf[0] = hex[(val >> 4u) & 0x0Fu];
    buf[1] = hex[val & 0x0Fu];
    buf[2] = '\0';
    DEBUG_writeString(buf);
}

static void APP_printROM(const OW_rom_t *rom)
{
    for (uint8_t i = 0u; i < OW_ROM_SIZE; i++)
    {
        APP_printHex(rom->bytes[i]);
        if (i < (OW_ROM_SIZE - 1u))
        {
            DEBUG_writeString(":");
        }
    }
}

static void APP_searchDevices(void)
{
    OW_rom_t  devices[MAX_DEVICES];
    uint8_t   count = 0u;
    OW_status_t status;

    DEBUG_writeString("--- ROM search ---\r\n");

    status = OW_searchROM(devices, MAX_DEVICES, &count);

    if (status == OW_NO_PRESENCE)
    {
        DEBUG_writeString("No devices found.\r\n");
        return;
    }
    if (status == OW_CRC_ERROR)
    {
        DEBUG_writeString("CRC error during search.\r\n");
        return;
    }

    for (uint8_t i = 0u; i < count; i++)
    {
        DEBUG_writeString("  Device ");
        APP_printHex(i);
        DEBUG_writeString(": ");
        APP_printROM(&devices[i]);
        DEBUG_writeString("\r\n");
    }
}

/* Convert DS18B20 raw 16-bit scratchpad word to tenths of degrees Celsius */
static int16_t APP_ds18b20RawToTenths(uint16_t raw)
{
    /* raw is a signed 16-bit value, 1 LSB = 1/16 °C */
    int16_t signed_raw = (int16_t)raw;
    int16_t tenths     = (int16_t)((signed_raw * 10) / 16);
    return tenths;
}

static void APP_readTemperature(void)
{
    uint8_t scratchpad[9];

    /* Step 1 – start conversion */
    if (OW_reset() != OW_OK)
    {
        DEBUG_writeString("No presence (convert)\r\n");
        return;
    }
    OW_skipROM();
    OW_writeByte(DS18B20_CMD_CONVERT_T);

    /* Step 2 – wait 750 ms (12-bit resolution) */
    SYS_delayMs(750u);

    /* Step 3 – read scratchpad */
    if (OW_reset() != OW_OK)
    {
        DEBUG_writeString("No presence (read)\r\n");
        return;
    }
    OW_skipROM();
    OW_writeByte(DS18B20_CMD_READ_SCRATCHPAD);

    for (uint8_t i = 0u; i < 9u; i++)
    {
        scratchpad[i] = OW_readByte();
    }

    /* Step 4 – verify CRC */
    if (OW_crc8(scratchpad, 9u) != 0u)
    {
        DEBUG_writeString("Scratchpad CRC error\r\n");
        return;
    }

    /* Step 5 – decode and print */
    uint16_t raw   = (uint16_t)((uint16_t)scratchpad[1] << 8u) | scratchpad[0];
    int16_t tenths = APP_ds18b20RawToTenths(raw);

    int16_t deg     = tenths / 10;
    int16_t frac    = tenths % 10;
    if (frac < 0) { frac = -frac; }

    /* Simple integer-to-string print */
    char buf[16];
    uint8_t idx = 0u;

    if (deg < 0)
    {
        buf[idx++] = '-';
        deg = -deg;
    }

    /* Hundreds */
    if (deg >= 100) { buf[idx++] = (char)('0' + (deg / 100)); deg %= 100; }
    /* Tens */
    if (idx > 0u || deg >= 10) { buf[idx++] = (char)('0' + (deg / 10)); }
    /* Units */
    buf[idx++] = (char)('0' + (deg % 10));
    buf[idx++] = '.';
    buf[idx++] = (char)('0' + frac);
    buf[idx++] = ' ';
    buf[idx++] = 'C';
    buf[idx++] = '\r';
    buf[idx++] = '\n';
    buf[idx]   = '\0';

    DEBUG_writeString("Temp: ");
    DEBUG_writeString(buf);
}

int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u);

    DEBUG_initTrace(SystemCoreClock);
    OW_init(&ow_config);

    DEBUG_writeString("1-Wire test start\r\n");

    /* Search once at startup */
    APP_searchDevices();

    while (1)
    {
        if (OW_reset() == OW_OK)
        {
            DEBUG_writeString("Presence OK\r\n");
            APP_readTemperature();
        }
        else
        {
            DEBUG_writeString("No presence\r\n");
        }

        SYS_delayMs(1000u);
    }
}
