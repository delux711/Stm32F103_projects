#include "app_config.h"
#include "debug.h"
#include "onewire.h"
#include "gpio.h"

/*
 * OW_DBG – debug output macro.
 * Default: calls DEBUG_writeString().
 * To disable: add  #define OW_DBG(msg)  to app_config.h.
 * Linker discards all related string literals when the macro is empty.
 */
#ifndef OW_DBG
#  define OW_DBG(msg)  DEBUG_writeString(msg)
#endif

/* ---------------------------------------------------------------- timing (µs) --
 *
 *  Slot timing follows Maxim application note AN 126 / DS18B20 datasheet.
 *
 *  Reset:   master drives low ≥ 480 µs   (use 500 µs)
 *           master releases, wait 70 µs then sample for presence
 *           wait remainder up to 480 µs after release (use 410 µs)
 *
 *  Write-1: drive low 6 µs, release for 64 µs               (total 70 µs)
 *  Write-0: drive low 60 µs, release for 10 µs              (total 70 µs)
 *  Read:    drive low 6 µs, release, sample at ~9 µs,
 *           hold until 70 µs slot complete                   (total 70 µs)
 *  Recovery between slots: ≥ 1 µs (1 µs included in slot gaps above)
 * ----------------------------------------------------------------------------- */

#define OW_TIMING_RESET_LOW_US    500u
#define OW_TIMING_PRESENCE_WAIT   70u
#define OW_TIMING_PRESENCE_TAIL   410u
#define OW_TIMING_WRITE1_LOW_US   6u
#define OW_TIMING_WRITE1_HIGH_US  64u
#define OW_TIMING_WRITE0_LOW_US   60u
#define OW_TIMING_WRITE0_HIGH_US  10u
#define OW_TIMING_READ_LOW_US     6u
#define OW_TIMING_READ_SAMPLE_US  9u
#define OW_TIMING_READ_TAIL_US    55u

/* --------------------------------------------------------- module state ----- */

static OW_config_t ow_cfg;
static uint16_t    ow_pin_mask;   /* pre-computed (1u << pin) */

/* --------------------------------------------------------- DWT delay -------- */

static void OW_dwtInit(void)
{
    /* Enable DWT and the cycle counter */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT  = 0u;
    DWT->CTRL   |= DWT_CTRL_CYCCNTENA_Msk;
}

static void OW_delayUs(uint32_t us)
{
    uint32_t cycles = (ow_cfg.cpu_freq_hz / 1000000u) * us;
    uint32_t start  = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles)
    {
        /* busy-wait */
    }
}

/* --------------------------------------------------------- GPIO helpers ----- */

/* Drive the line LOW (output open-drain, write 0 to ODR) */
static inline void OW_pinLow(void)
{
    ow_cfg.port->BRR = ow_pin_mask;
}

/* Release the line (output open-drain, write 1 to ODR → pin floats high via pull-up) */
static inline void OW_pinRelease(void)
{
    ow_cfg.port->BSRR = ow_pin_mask;
}

/* Sample the line state */
static inline uint8_t OW_pinRead(void)
{
    return (ow_cfg.port->IDR & ow_pin_mask) ? 1u : 0u;
}

/* --------------------------------------------------------- public API ------- */

void OW_init(const OW_config_t *config)
{
    ow_cfg      = *config;
    ow_pin_mask = (uint16_t)(1u << config->pin);

    OW_dwtInit();

    GPIO_enableClock(config->port);

    /* Release the line (idle high) before configuring as output */
    OW_pinRelease();

    /* Open-drain output at 50 MHz so the pin can be both driven low and sampled */
    GPIO_configPin(config->port, config->pin, GPIO_CFG_OUTPUT_OD_50MHZ);
    OW_DBG("OW init\r\n");
}

OW_status_t OW_reset(void)
{
    uint8_t presence;

    /* Pull bus low for the reset pulse */
    OW_pinLow();
    OW_delayUs(OW_TIMING_RESET_LOW_US);

    /* Release and wait for the device to assert a presence pulse */
    OW_pinRelease();
    OW_delayUs(OW_TIMING_PRESENCE_WAIT);

    /* Sample – a low level means a device is present */
    presence = OW_pinRead();

    /* Wait out the rest of the reset window */
    OW_delayUs(OW_TIMING_PRESENCE_TAIL);

    if (presence == 0u)
    {
        return OW_OK;
    }
    OW_DBG("OW no presence\r\n");
    return OW_NO_PRESENCE;
}

void OW_writeBit(uint8_t bit)
{
    if (bit != 0u)
    {
        /* Write-1 slot */
        OW_pinLow();
        OW_delayUs(OW_TIMING_WRITE1_LOW_US);
        OW_pinRelease();
        OW_delayUs(OW_TIMING_WRITE1_HIGH_US);
    }
    else
    {
        /* Write-0 slot */
        OW_pinLow();
        OW_delayUs(OW_TIMING_WRITE0_LOW_US);
        OW_pinRelease();
        OW_delayUs(OW_TIMING_WRITE0_HIGH_US);
    }
}

uint8_t OW_readBit(void)
{
    uint8_t bit;

    /* Initiate the read slot */
    OW_pinLow();
    OW_delayUs(OW_TIMING_READ_LOW_US);
    OW_pinRelease();

    /* Wait until the sample point (~9 µs from the falling edge) */
    OW_delayUs(OW_TIMING_READ_SAMPLE_US);
    bit = OW_pinRead();

    /* Complete the 70 µs slot */
    OW_delayUs(OW_TIMING_READ_TAIL_US);

    return bit;
}

void OW_writeByte(uint8_t byte)
{
    uint8_t i;

    for (i = 0u; i < 8u; i++)
    {
        OW_writeBit(byte & 0x01u);
        byte >>= 1u;
    }
}

uint8_t OW_readByte(void)
{
    uint8_t byte = 0u;
    uint8_t i;

    for (i = 0u; i < 8u; i++)
    {
        byte >>= 1u;
        if (OW_readBit() != 0u)
        {
            byte |= 0x80u;
        }
    }

    return byte;
}

OW_status_t OW_readROM(OW_rom_t *rom)
{
    uint8_t i;

    OW_DBG("OW read ROM\r\n");
    OW_writeByte(OW_CMD_READ_ROM);

    for (i = 0u; i < OW_ROM_SIZE; i++)
    {
        rom->bytes[i] = OW_readByte();
    }

    if (OW_crc8(rom->bytes, OW_ROM_SIZE) != 0u)
    {
        OW_DBG("OW ROM CRC err\r\n");
        return OW_CRC_ERROR;
    }

    return OW_OK;
}

void OW_matchROM(const OW_rom_t *rom)
{
    uint8_t i;

    OW_writeByte(OW_CMD_MATCH_ROM);

    for (i = 0u; i < OW_ROM_SIZE; i++)
    {
        OW_writeByte(rom->bytes[i]);
    }
}

void OW_skipROM(void)
{
    OW_writeByte(OW_CMD_SKIP_ROM);
}

/* --------------------------------------------------------- ROM search ------- */
/*
 * Maxim/Dallas standard search algorithm.
 * Reference: Maxim application note AN 187.
 */
OW_status_t OW_searchROM(OW_rom_t *found, uint8_t max_count, uint8_t *count)
{
    uint8_t  rom_buf[OW_ROM_SIZE];
    int8_t   last_discrepancy    = -1;
    int8_t   last_zero           = -1;
    uint8_t  search_done         = 0u;
    uint8_t  device_count        = 0u;

    *count = 0u;

    OW_DBG("OW search ROM\r\n");
    while ((search_done == 0u) && (device_count < max_count))
    {
        if (OW_reset() != OW_OK)
        {
            OW_DBG("OW search: no presence\r\n");
            return (device_count > 0u) ? OW_OK : OW_NO_PRESENCE;
        }

        OW_writeByte(OW_CMD_SEARCH_ROM);

        last_zero = -1;

        for (int8_t bit_idx = 0; bit_idx < 64; bit_idx++)
        {
            uint8_t id_bit      = OW_readBit();
            uint8_t cmp_id_bit  = OW_readBit();
            uint8_t bit_to_send;

            uint8_t byte_idx  = (uint8_t)bit_idx / 8u;
            uint8_t bit_shift = (uint8_t)bit_idx % 8u;

            if ((id_bit == 1u) && (cmp_id_bit == 1u))
            {
                /* No devices participated – bus error */
                return OW_NO_PRESENCE;
            }

            if ((id_bit == 0u) && (cmp_id_bit == 0u))
            {
                /* Discrepancy */
                if (bit_idx == last_discrepancy)
                {
                    bit_to_send = 1u;
                }
                else if (bit_idx > last_discrepancy)
                {
                    bit_to_send = 0u;
                    last_zero   = bit_idx;
                }
                else
                {
                    /* Use the bit from the previous pass */
                    bit_to_send = (rom_buf[byte_idx] >> bit_shift) & 0x01u;
                    if (bit_to_send == 0u)
                    {
                        last_zero = bit_idx;
                    }
                }
            }
            else
            {
                bit_to_send = id_bit;
            }

            /* Store the chosen bit */
            if (bit_to_send == 0u)
            {
                rom_buf[byte_idx] &= ~(uint8_t)(1u << bit_shift);
            }
            else
            {
                rom_buf[byte_idx] |= (uint8_t)(1u << bit_shift);
            }

            OW_writeBit(bit_to_send);
        }

        /* Verify CRC of the found ROM code */
        if (OW_crc8(rom_buf, OW_ROM_SIZE) != 0u)
        {
            OW_DBG("OW search CRC err\r\n");
            return OW_CRC_ERROR;
        }

        /* Save result */
        for (uint8_t i = 0u; i < OW_ROM_SIZE; i++)
        {
            found[device_count].bytes[i] = rom_buf[i];
        }
        device_count++;

        last_discrepancy = last_zero;

        if (last_discrepancy == -1)
        {
            search_done = 1u;
        }
    }

    *count = device_count;
    OW_DBG("OW search done\r\n");
    return OW_OK;
}

/* --------------------------------------------------------- CRC-8 ----------- */
/*
 * Dallas/Maxim CRC-8: polynomial x^8 + x^5 + x^4 + 1 (0x31), init = 0x00.
 * A correct 8-byte ROM (including the CRC byte) will produce a result of 0x00.
 */
uint8_t OW_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0x00u;
    uint8_t i;
    uint8_t j;

    for (i = 0u; i < len; i++)
    {
        crc ^= data[i];
        for (j = 0u; j < 8u; j++)
        {
            if ((crc & 0x01u) != 0u)
            {
                crc = (crc >> 1u) ^ 0x8Cu;
            }
            else
            {
                crc >>= 1u;
            }
        }
    }

    return crc;
}
