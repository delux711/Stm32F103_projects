#include "app_config.h"
#include <stdbool.h>
#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "rs485.h"
#include "modbus_rtu.h"
#include "debug.h"

static bool APP_readHolding(void *context, uint16_t address, uint16_t *value);
static bool APP_writeHolding(void *context, uint16_t address, uint16_t value);

static const RS485_config_t rs485_config = {
    .txPort = GPIOB,
    .txPin = 6u,
    .rxPort = GPIOB,
    .rxPin = 7u,
    .dirPort = GPIOB,
    .dirPin = 8u,
    .usartRemapMask = AFIO_MAPR_USART1_REMAP,
    .usartRemap = AFIO_MAPR_USART1_REMAP,
    .usart = USART1,
    .usartIrqn = USART1_IRQn,
    .usartRccReg = &RCC->APB2ENR,
    .usartRccBit = RCC_APB2ENR_USART1EN,
    .baudrate = 9600u};

static uint16_t app_holding_registers[] = {
    0x1234u,
    0x2026u,
    250u,
    0u,
    1u,
    77u,
    0xBEEFu,
    0x0001u
};

#define APP_HOLDING_REG_COUNT ((uint16_t)(sizeof(app_holding_registers) / sizeof(app_holding_registers[0])))

int main(void)
{
    MODBUS_RTU_config_t modbus_config = {
        .slave_address = 1u,
        .interframe_timeout_ms = 5u,
        .max_registers_per_request = APP_HOLDING_REG_COUNT,
        .read_holding_reg = APP_readHolding,
        .write_holding_reg = APP_writeHolding,
        .context = app_holding_registers
    };

    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u);

    DEBUG_initTrace(SystemCoreClock);
    RS485_init(&rs485_config);
    MODBUS_RTU_init(&modbus_config);
    DEBUG_sendString("Modbus RTU slave test\r\n", 0);
    DEBUG_writeString("FC03/FC06/FC10 ready\r\n");

    while (1)
    {
        MODBUS_RTU_process();
        __WFI();
    }
}

static bool APP_readHolding(void *context, uint16_t address, uint16_t *value)
{
    uint16_t *registers = (uint16_t *)context;

    if ((registers == 0) || (value == 0) || (address >= APP_HOLDING_REG_COUNT))
    {
        return false;
    }

    *value = registers[address];
    return true;
}

static bool APP_writeHolding(void *context, uint16_t address, uint16_t value)
{
    uint16_t *registers = (uint16_t *)context;

    if ((registers == 0) || (address >= APP_HOLDING_REG_COUNT))
    {
        return false;
    }

    registers[address] = value;
    return true;
}
