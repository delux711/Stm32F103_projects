#ifndef ONEWIRE_H
#define ONEWIRE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "stm32f10x.h"

/*
 * 1-Wire driver for STM32F103
 *
 * The data line must be configured as open-drain with an external pull-up
 * resistor (typically 4.7 kΩ to VCC).
 *
 * Timing is based on the DWT cycle counter – call OW_init() before use so the
 * counter is enabled.
 */

/* ------------------------------------------------------------------ types -- */

typedef enum
{
    OW_OK          = 0,  /* operation completed successfully              */
    OW_NO_PRESENCE = 1,  /* no device detected during reset               */
    OW_CRC_ERROR   = 2   /* CRC check failed (ROM search / scratchpad)    */
} OW_status_t;

typedef struct
{
    GPIO_TypeDef *port;        /* GPIO port of the data line, e.g. GPIOA  */
    uint8_t       pin;         /* GPIO pin number 0-15                     */
    uint32_t      cpu_freq_hz; /* CPU frequency in Hz (e.g. 72000000)      */
} OW_config_t;

/* --------------------------------------------------------- ROM address type -- */

#define OW_ROM_SIZE  8u   /* 64-bit ROM: 1B family + 6B serial + 1B CRC  */

typedef struct
{
    uint8_t bytes[OW_ROM_SIZE];
} OW_rom_t;

/* ------------------------------------------------------ ROM command codes -- */

#define OW_CMD_SEARCH_ROM   0xF0u
#define OW_CMD_READ_ROM     0x33u
#define OW_CMD_MATCH_ROM    0x55u
#define OW_CMD_SKIP_ROM     0xCCu
#define OW_CMD_ALARM_SEARCH 0xECu

/* --------------------------------------------------------- public API ------ */

/**
 * @brief  Initialise the 1-Wire driver.
 *         Enables the GPIO clock, configures the pin as open-drain output and
 *         enables the DWT cycle counter used for microsecond delays.
 * @param  config  Pointer to a filled OW_config_t structure.
 */
void OW_init(const OW_config_t *config);

/**
 * @brief  Issue a reset pulse and detect the presence pulse.
 * @return OW_OK if at least one device responded, OW_NO_PRESENCE otherwise.
 */
OW_status_t OW_reset(void);

/**
 * @brief  Write a single byte on the bus (LSB first).
 * @param  byte  Data byte to send.
 */
void OW_writeByte(uint8_t byte);

/**
 * @brief  Read a single byte from the bus (LSB first).
 * @return Received byte.
 */
uint8_t OW_readByte(void);

/**
 * @brief  Write a single bit on the bus.
 * @param  bit  0 or 1.
 */
void OW_writeBit(uint8_t bit);

/**
 * @brief  Read a single bit from the bus.
 * @return 0 or 1.
 */
uint8_t OW_readBit(void);

/**
 * @brief  Read the 64-bit ROM code from a single device on the bus.
 *         Must be called immediately after OW_reset().
 * @param  rom  Output – filled with the 8-byte ROM code.
 * @return OW_OK or OW_CRC_ERROR.
 */
OW_status_t OW_readROM(OW_rom_t *rom);

/**
 * @brief  Address a specific device by its ROM code.
 *         Must be called immediately after OW_reset().
 * @param  rom  ROM code of the target device.
 */
void OW_matchROM(const OW_rom_t *rom);

/**
 * @brief  Skip ROM addressing (use when only one device is on the bus).
 *         Must be called immediately after OW_reset().
 */
void OW_skipROM(void);

/**
 * @brief  Search for all devices on the bus (standard ROM search algorithm).
 * @param  found      Output array of ROM codes.
 * @param  max_count  Capacity of the found[] array.
 * @param  count      Output – number of devices found.
 * @return OW_OK, OW_NO_PRESENCE (no devices) or OW_CRC_ERROR.
 */
OW_status_t OW_searchROM(OW_rom_t *found, uint8_t max_count, uint8_t *count);

/**
 * @brief  Calculate Dallas/Maxim CRC-8 (poly 0x31, init 0x00).
 * @param  data  Pointer to data buffer.
 * @param  len   Number of bytes.
 * @return CRC-8 value.
 */
uint8_t OW_crc8(const uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* ONEWIRE_H */
