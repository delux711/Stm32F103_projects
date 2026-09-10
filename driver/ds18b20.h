#ifndef DS18B20_H
#define DS18B20_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "onewire.h"

/* DS18B20 ROM family code */
#define DS18B20_FAMILY_CODE              0x28u

/* DS18B20 function commands */
#define DS18B20_CMD_CONVERT_T            0x44u
#define DS18B20_CMD_READ_SCRATCHPAD      0xBEu
#define DS18B20_CMD_WRITE_SCRATCHPAD     0x4Eu
#define DS18B20_CMD_COPY_SCRATCHPAD      0x48u
#define DS18B20_CMD_RECALL_EE            0xB8u
#define DS18B20_CMD_READ_POWER_SUPPLY    0xB4u

typedef enum
{
    DS18B20_OK = 0,
    DS18B20_ERR_NO_DEVICE,
    DS18B20_ERR_CRC,
    DS18B20_ERR_BAD_PARAM
} DS18B20_status_t;

typedef struct
{
    OW_config_t ow;               /* 1-Wire bus configuration              */
    uint8_t     use_rom_match;    /* 0 = SKIP ROM, 1 = MATCH ROM          */
    OW_rom_t    rom;              /* used only when use_rom_match == 1    */
} DS18B20_config_t;

/**
 * @brief Initialise DS18B20 driver and underlying 1-Wire driver.
 */
void DS18B20_init(const DS18B20_config_t *config);

/**
 * @brief Start temperature conversion.
 */
DS18B20_status_t DS18B20_startConversion(void);

/**
 * @brief Read 9-byte scratchpad and verify CRC.
 */
DS18B20_status_t DS18B20_readScratchpad(uint8_t scratchpad[9]);

/**
 * @brief Read temperature in tenths of degree Celsius.
 * @param temp_c10 output temperature in 0.1 deg C units.
 */
DS18B20_status_t DS18B20_readTemperatureC10(int16_t *temp_c10);

/**
 * @brief Discover one DS18B20 on bus and store its ROM.
 */
DS18B20_status_t DS18B20_discoverSingle(OW_rom_t *rom_out);

#ifdef __cplusplus
}
#endif

#endif /* DS18B20_H */