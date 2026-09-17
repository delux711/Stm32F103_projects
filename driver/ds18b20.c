#include "app_config.h"
#include "debug.h"
#include "ds18b20.h"

/*
 * DS18B20_DBG – debug output macro.
 * Default: calls DEBUG_writeString().
 * To disable: add  #define DS18B20_DBG(msg)  to app_config.h.
 * Linker discards all related string literals when the macro is empty.
 */
#ifndef DS18B20_DBG
#  define DS18B20_DBG(msg)  DEBUG_writeString(msg)
#endif

static DS18B20_config_t ds_cfg;

static DS18B20_status_t DS18B20_addressDevice(void)
{
    if (OW_reset() != OW_OK)
    {
        DS18B20_DBG("DS18B20 no presence\r\n");
        return DS18B20_ERR_NO_DEVICE;
    }

    if (ds_cfg.use_rom_match != 0u)
    {
        OW_matchROM(&ds_cfg.rom);
    }
    else
    {
        OW_skipROM();
    }

    return DS18B20_OK;
}

void DS18B20_init(const DS18B20_config_t *config)
{
    ds_cfg = *config;
    OW_init(&ds_cfg.ow);
    DS18B20_DBG("DS18B20 init\r\n");
}

DS18B20_status_t DS18B20_startConversion(void)
{
    DS18B20_status_t st = DS18B20_addressDevice();
    if (st != DS18B20_OK)
    {
        return st;
    }

    DS18B20_DBG("DS18B20 convert T\r\n");
    OW_writeByte(DS18B20_CMD_CONVERT_T);
    return DS18B20_OK;
}

DS18B20_status_t DS18B20_readScratchpad(uint8_t scratchpad[9])
{
    uint8_t i;
    DS18B20_status_t st;

    if (scratchpad == 0)
    {
        return DS18B20_ERR_BAD_PARAM;
    }

    st = DS18B20_addressDevice();
    if (st != DS18B20_OK)
    {
        return st;
    }

    DS18B20_DBG("DS18B20 read scratchpad\r\n");
    OW_writeByte(DS18B20_CMD_READ_SCRATCHPAD);

    for (i = 0u; i < 9u; i++)
    {
        scratchpad[i] = OW_readByte();
    }

    if (OW_crc8(scratchpad, 9u) != 0u)
    {
        DS18B20_DBG("DS18B20 scratchpad CRC err\r\n");
        return DS18B20_ERR_CRC;
    }

    return DS18B20_OK;
}

DS18B20_status_t DS18B20_readTemperatureC10(int16_t *temp_c10)
{
    uint8_t scratchpad[9];
    int16_t raw;
    DS18B20_status_t st;

    if (temp_c10 == 0)
    {
        return DS18B20_ERR_BAD_PARAM;
    }

    st = DS18B20_readScratchpad(scratchpad);
    if (st != DS18B20_OK)
    {
        return st;
    }

    raw = (int16_t)(((uint16_t)scratchpad[1] << 8u) | scratchpad[0]);
    *temp_c10 = (int16_t)((raw * 10) / 16);

    return DS18B20_OK;
}

DS18B20_status_t DS18B20_discoverSingle(OW_rom_t *rom_out)
{
    OW_rom_t found[1];
    uint8_t count = 0u;
    OW_status_t ow_st;

    if (rom_out == 0)
    {
        return DS18B20_ERR_BAD_PARAM;
    }

    DS18B20_DBG("DS18B20 discover\r\n");
    ow_st = OW_searchROM(found, 1u, &count);
    if ((ow_st != OW_OK) || (count == 0u))
    {
        DS18B20_DBG("DS18B20 discover: no device\r\n");
        return DS18B20_ERR_NO_DEVICE;
    }

    if (found[0].bytes[0] != DS18B20_FAMILY_CODE)
    {
        DS18B20_DBG("DS18B20 discover: wrong family code\r\n");
        return DS18B20_ERR_NO_DEVICE;
    }

    DS18B20_DBG("DS18B20 discover: found\r\n");
    *rom_out = found[0];
    return DS18B20_OK;
}