#include <stddef.h>
#include "app_config.h"
#include "modbus_slave.h"
#include "modbus_rtu.h"
#include "device_registry.h"
#include "debug.h"

static ModbusSlave_coilGetFunc_t coil_handler = NULL;
static ModbusSlave_regGetFunc_t reg_read_handler = NULL;
static ModbusSlave_regSetFunc_t reg_write_handler = NULL;

static bool ModbusSlave_readHoldingReg(void *context, uint16_t address, uint16_t *value)
{
    (void)context;

    if (reg_read_handler != NULL)
    {
        return (reg_read_handler(address, value) == 0) ? true : false;
    }

    if (DeviceRegistry_findByAddress(address) != NULL)
    {
        *value = DeviceRegistry_read(address);
        return true;
    }

    return false;
}

static bool ModbusSlave_writeHoldingReg(void *context, uint16_t address, uint16_t value)
{
    (void)context;

    if (reg_write_handler != NULL)
    {
        reg_write_handler(address, value);
        return true;
    }

    DeviceRegistry_write(address, value);
    return true;
}

void ModbusSlave_init(const MODBUS_RTU_config_t *config, uint8_t slave_addr)
{
    MODBUS_RTU_config_t mb_config;

    mb_config.slave_address = slave_addr;
    mb_config.interframe_timeout_ms = config->interframe_timeout_ms;
    mb_config.max_registers_per_request = config->max_registers_per_request;
    mb_config.read_holding_reg = ModbusSlave_readHoldingReg;
    mb_config.write_holding_reg = ModbusSlave_writeHoldingReg;
    mb_config.context = NULL;

    MODBUS_RTU_init(&mb_config);
}

void ModbusSlave_setCoilHandler(ModbusSlave_coilGetFunc_t handler)
{
    coil_handler = handler;
}

void ModbusSlave_setRegisterReadHandler(ModbusSlave_regGetFunc_t handler)
{
    reg_read_handler = handler;
}

void ModbusSlave_setRegisterWriteHandler(ModbusSlave_regSetFunc_t handler)
{
    reg_write_handler = handler;
}

void ModbusSlave_onRxByte(uint8_t byte)
{
    (void)byte;
}

void ModbusSlave_process(void)
{
    MODBUS_RTU_process();
}
