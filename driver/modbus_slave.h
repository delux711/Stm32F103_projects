#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "modbus_rtu.h"

typedef int (*ModbusSlave_coilGetFunc_t)(uint16_t addr, uint16_t *value);
typedef int (*ModbusSlave_regGetFunc_t)(uint16_t addr, uint16_t *value);
typedef int (*ModbusSlave_regSetFunc_t)(uint16_t addr, uint16_t value);

void ModbusSlave_init(const MODBUS_RTU_config_t *config, uint8_t slave_addr);
void ModbusSlave_setCoilHandler(ModbusSlave_coilGetFunc_t handler);
void ModbusSlave_setRegisterReadHandler(ModbusSlave_regGetFunc_t handler);
void ModbusSlave_setRegisterWriteHandler(ModbusSlave_regSetFunc_t handler);

void ModbusSlave_onRxByte(uint8_t byte);
void ModbusSlave_process(void);

#ifdef __cplusplus
}
#endif

#endif
