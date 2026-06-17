#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <stdint.h>
#include <stdbool.h>

typedef bool (*MODBUS_RTU_readHoldingRegCb_t)(void *context, uint16_t address, uint16_t *value);
typedef bool (*MODBUS_RTU_writeHoldingRegCb_t)(void *context, uint16_t address, uint16_t value);

typedef struct
{
    uint8_t slave_address;
    uint32_t interframe_timeout_ms;
    uint16_t max_registers_per_request;
    MODBUS_RTU_readHoldingRegCb_t read_holding_reg;
    MODBUS_RTU_writeHoldingRegCb_t write_holding_reg;
    void *context;
} MODBUS_RTU_config_t;

void MODBUS_RTU_init(const MODBUS_RTU_config_t *config);
void MODBUS_RTU_process(void);

#endif