#include "modbus_rtu.h"

#include <string.h>
#include "rs485.h"
#include "systick.h"

#define MODBUS_RTU_MAX_FRAME_SIZE            (256u)
#define MODBUS_RTU_MIN_REQUEST_SIZE          (8u)
#define MODBUS_RTU_DEFAULT_ADDRESS           (1u)
#define MODBUS_RTU_DEFAULT_INTERFRAME_MS     (5u)
#define MODBUS_RTU_DEFAULT_MAX_REGS_PER_REQ  (32u)
#define MODBUS_RTU_FC_READ_HOLDING_REGS      (0x03u)
#define MODBUS_RTU_FC_WRITE_SINGLE_REG       (0x06u)
#define MODBUS_RTU_FC_WRITE_MULTIPLE_REGS    (0x10u)

#define MODBUS_EX_ILLEGAL_FUNCTION           (0x01u)
#define MODBUS_EX_ILLEGAL_DATA_ADDRESS       (0x02u)
#define MODBUS_EX_ILLEGAL_DATA_VALUE         (0x03u)
#define MODBUS_EX_SLAVE_DEVICE_FAILURE       (0x04u)

typedef struct
{
    uint8_t slave_address;
    uint32_t interframe_timeout_ms;
    uint16_t max_registers_per_request;
    MODBUS_RTU_readHoldingRegCb_t read_holding_reg;
    MODBUS_RTU_writeHoldingRegCb_t write_holding_reg;
    void *context;
} MODBUS_RTU_runtime_t;

static MODBUS_RTU_runtime_t modbus_runtime;
static volatile uint8_t modbus_rx_buffer[MODBUS_RTU_MAX_FRAME_SIZE];
static volatile uint16_t modbus_rx_len = 0u;
static volatile bool modbus_frame_active = false;
static volatile uint32_t modbus_last_rx_tick = 0u;
static uint8_t modbus_tx_buffer[MODBUS_RTU_MAX_FRAME_SIZE];

static uint16_t MODBUS_RTU_crc16(const uint8_t *data, uint16_t len);
static void MODBUS_RTU_onRxByte(uint8_t data);
static void MODBUS_RTU_handleFrame(const uint8_t *frame, uint16_t len);
static void MODBUS_RTU_sendException(uint8_t slave_address, uint8_t function, uint8_t exception);
static void MODBUS_RTU_sendWithCrc(const uint8_t *pdu, uint16_t length);
static uint16_t MODBUS_RTU_readU16Be(const uint8_t *ptr);

void MODBUS_RTU_init(const MODBUS_RTU_config_t *config)
{
    modbus_runtime.slave_address = MODBUS_RTU_DEFAULT_ADDRESS;
    modbus_runtime.interframe_timeout_ms = MODBUS_RTU_DEFAULT_INTERFRAME_MS;
    modbus_runtime.max_registers_per_request = MODBUS_RTU_DEFAULT_MAX_REGS_PER_REQ;
    modbus_runtime.read_holding_reg = 0;
    modbus_runtime.write_holding_reg = 0;
    modbus_runtime.context = 0;

    if (config != 0)
    {
        if (config->slave_address != 0u)
        {
            modbus_runtime.slave_address = config->slave_address;
        }
        if (config->interframe_timeout_ms != 0u)
        {
            modbus_runtime.interframe_timeout_ms = config->interframe_timeout_ms;
        }
        if (config->max_registers_per_request != 0u)
        {
            modbus_runtime.max_registers_per_request = config->max_registers_per_request;
        }
        modbus_runtime.read_holding_reg = config->read_holding_reg;
        modbus_runtime.write_holding_reg = config->write_holding_reg;
        modbus_runtime.context = config->context;
    }

    modbus_rx_len = 0u;
    modbus_frame_active = false;
    modbus_last_rx_tick = 0u;

    RS485_setRxCallback(MODBUS_RTU_onRxByte);
}

void MODBUS_RTU_process(void)
{
    uint16_t frame_len;
    uint8_t frame_copy[MODBUS_RTU_MAX_FRAME_SIZE];

    if (!modbus_frame_active)
    {
        return;
    }

    if ((int32_t)(SYS_getMs() - modbus_last_rx_tick) < (int32_t)modbus_runtime.interframe_timeout_ms)
    {
        return;
    }

    frame_len = modbus_rx_len;
    if (frame_len > MODBUS_RTU_MAX_FRAME_SIZE)
    {
        frame_len = MODBUS_RTU_MAX_FRAME_SIZE;
    }

    for (uint16_t i = 0u; i < frame_len; i++)
    {
        frame_copy[i] = modbus_rx_buffer[i];
    }

    modbus_rx_len = 0u;
    modbus_frame_active = false;

    MODBUS_RTU_handleFrame(frame_copy, frame_len);
}

static void MODBUS_RTU_onRxByte(uint8_t data)
{
    if (modbus_rx_len < MODBUS_RTU_MAX_FRAME_SIZE)
    {
        modbus_rx_buffer[modbus_rx_len++] = data;
        modbus_last_rx_tick = SYS_getMs();
        modbus_frame_active = true;
    }
    else
    {
        modbus_rx_len = 0u;
        modbus_frame_active = false;
    }
}

static void MODBUS_RTU_handleFrame(const uint8_t *frame, uint16_t len)
{
    uint8_t slave_address;
    uint8_t function;
    uint16_t crc_calc;
    uint16_t crc_frame;
    bool is_broadcast;

    if ((frame == 0) || (len < MODBUS_RTU_MIN_REQUEST_SIZE))
    {
        return;
    }

    slave_address = frame[0u];
    function = frame[1u];
    is_broadcast = (slave_address == 0u);

    if ((!is_broadcast) && (slave_address != modbus_runtime.slave_address))
    {
        return;
    }

    crc_calc = MODBUS_RTU_crc16(frame, (uint16_t)(len - 2u));
    crc_frame = (uint16_t)frame[len - 2u] | ((uint16_t)frame[len - 1u] << 8);
    if (crc_calc != crc_frame)
    {
        return;
    }

    if (function == MODBUS_RTU_FC_READ_HOLDING_REGS)
    {
        uint16_t start_address = MODBUS_RTU_readU16Be(&frame[2u]);
        uint16_t quantity = MODBUS_RTU_readU16Be(&frame[4u]);
        uint8_t byte_count;

        if ((quantity == 0u) || (quantity > 125u) || (quantity > modbus_runtime.max_registers_per_request))
        {
            if (!is_broadcast)
            {
                MODBUS_RTU_sendException(slave_address, function, MODBUS_EX_ILLEGAL_DATA_VALUE);
            }
            return;
        }

        if (is_broadcast)
        {
            return;
        }

        byte_count = (uint8_t)(quantity * 2u);
        modbus_tx_buffer[0u] = slave_address;
        modbus_tx_buffer[1u] = function;
        modbus_tx_buffer[2u] = byte_count;

        for (uint16_t i = 0u; i < quantity; i++)
        {
            uint16_t value;
            bool ok;

            if (modbus_runtime.read_holding_reg == 0)
            {
                MODBUS_RTU_sendException(slave_address, function, MODBUS_EX_SLAVE_DEVICE_FAILURE);
                return;
            }

            ok = modbus_runtime.read_holding_reg(modbus_runtime.context, (uint16_t)(start_address + i), &value);
            if (!ok)
            {
                MODBUS_RTU_sendException(slave_address, function, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
                return;
            }

            modbus_tx_buffer[3u + (i * 2u)] = (uint8_t)((value >> 8) & 0xFFu);
            modbus_tx_buffer[4u + (i * 2u)] = (uint8_t)(value & 0xFFu);
        }

        MODBUS_RTU_sendWithCrc(modbus_tx_buffer, (uint16_t)(3u + byte_count));
        return;
    }

    if (function == MODBUS_RTU_FC_WRITE_SINGLE_REG)
    {
        uint16_t address = MODBUS_RTU_readU16Be(&frame[2u]);
        uint16_t value = MODBUS_RTU_readU16Be(&frame[4u]);

        if (modbus_runtime.write_holding_reg == 0)
        {
            if (!is_broadcast)
            {
                MODBUS_RTU_sendException(slave_address, function, MODBUS_EX_SLAVE_DEVICE_FAILURE);
            }
            return;
        }

        if (!modbus_runtime.write_holding_reg(modbus_runtime.context, address, value))
        {
            if (!is_broadcast)
            {
                MODBUS_RTU_sendException(slave_address, function, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
            }
            return;
        }

        if (!is_broadcast)
        {
            MODBUS_RTU_sendWithCrc(frame, 6u);
        }
        return;
    }

    if (function == MODBUS_RTU_FC_WRITE_MULTIPLE_REGS)
    {
        uint16_t start_address = MODBUS_RTU_readU16Be(&frame[2u]);
        uint16_t quantity = MODBUS_RTU_readU16Be(&frame[4u]);
        uint8_t byte_count = frame[6u];

        if ((quantity == 0u) || (quantity > 123u) || (quantity > modbus_runtime.max_registers_per_request))
        {
            if (!is_broadcast)
            {
                MODBUS_RTU_sendException(slave_address, function, MODBUS_EX_ILLEGAL_DATA_VALUE);
            }
            return;
        }

        if (byte_count != (uint8_t)(quantity * 2u))
        {
            if (!is_broadcast)
            {
                MODBUS_RTU_sendException(slave_address, function, MODBUS_EX_ILLEGAL_DATA_VALUE);
            }
            return;
        }

        if (len != (uint16_t)(9u + byte_count))
        {
            if (!is_broadcast)
            {
                MODBUS_RTU_sendException(slave_address, function, MODBUS_EX_ILLEGAL_DATA_VALUE);
            }
            return;
        }

        if (modbus_runtime.write_holding_reg == 0)
        {
            if (!is_broadcast)
            {
                MODBUS_RTU_sendException(slave_address, function, MODBUS_EX_SLAVE_DEVICE_FAILURE);
            }
            return;
        }

        for (uint16_t i = 0u; i < quantity; i++)
        {
            uint16_t value = MODBUS_RTU_readU16Be(&frame[7u + (i * 2u)]);

            if (!modbus_runtime.write_holding_reg(modbus_runtime.context, (uint16_t)(start_address + i), value))
            {
                if (!is_broadcast)
                {
                    MODBUS_RTU_sendException(slave_address, function, MODBUS_EX_ILLEGAL_DATA_ADDRESS);
                }
                return;
            }
        }

        if (!is_broadcast)
        {
            MODBUS_RTU_sendWithCrc(frame, 6u);
        }
        return;
    }

    if (!is_broadcast)
    {
        MODBUS_RTU_sendException(slave_address, function, MODBUS_EX_ILLEGAL_FUNCTION);
    }
}

static void MODBUS_RTU_sendException(uint8_t slave_address, uint8_t function, uint8_t exception)
{
    modbus_tx_buffer[0u] = slave_address;
    modbus_tx_buffer[1u] = (uint8_t)(function | 0x80u);
    modbus_tx_buffer[2u] = exception;
    MODBUS_RTU_sendWithCrc(modbus_tx_buffer, 3u);
}

static void MODBUS_RTU_sendWithCrc(const uint8_t *pdu, uint16_t length)
{
    uint16_t crc;

    if ((pdu == 0) || (length == 0u) || ((uint16_t)(length + 2u) > MODBUS_RTU_MAX_FRAME_SIZE))
    {
        return;
    }

    memcpy(modbus_tx_buffer, pdu, length);
    crc = MODBUS_RTU_crc16(modbus_tx_buffer, length);
    modbus_tx_buffer[length] = (uint8_t)(crc & 0x00FFu);
    modbus_tx_buffer[length + 1u] = (uint8_t)((crc >> 8) & 0x00FFu);
    RS485_send(modbus_tx_buffer, (uint16_t)(length + 2u));
}

static uint16_t MODBUS_RTU_readU16Be(const uint8_t *ptr)
{
    return (uint16_t)(((uint16_t)ptr[0u] << 8) | (uint16_t)ptr[1u]);
}

static uint16_t MODBUS_RTU_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFu;

    if (data == 0)
    {
        return crc;
    }

    for (uint16_t i = 0u; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0u; bit < 8u; bit++)
        {
            if ((crc & 0x0001u) != 0u)
            {
                crc = (uint16_t)((crc >> 1) ^ 0xA001u);
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}