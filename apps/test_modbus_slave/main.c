#include <string.h>
#include <stdarg.h>
#include "app_config.h"
#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "debug.h"
#include "debug_rtt_levels.h"
#include "systick.h"
#include "rs485.h"
#include "device_config.h"
#include "device_registry.h"
#include "modbus_slave.h"

#define VECTOR_TABLE_OFFSET (0x1000U)

static uint32_t counter = 0;

static const RS485_config_t rs485_config = {
    .uart = {
        .txPort = GPIOB,
        .txPin = 6u,
        .rxPort = GPIOB,
        .rxPin = 7u,
        .usartRemapMask = AFIO_MAPR_USART1_REMAP,
        .usartRemap = AFIO_MAPR_USART1_REMAP,
        .usart = USART1,
        .usartIrqn = USART1_IRQn,
        .usartRccReg = &RCC->APB2ENR,
        .usartRccBit = RCC_APB2ENR_USART1EN,
        .baudrate = 9600u
    },
    .dirPort = GPIOB,
    .dirPin = 8u
};

static uint16_t read_device_id(void)
{
    DeviceConfig_t *cfg = DeviceConfig_get();
    return cfg->device_id;
}

static uint16_t read_fw_version(void)
{
    DeviceConfig_t *cfg = DeviceConfig_get();
    return cfg->fw_version;
}

static uint16_t read_counter(void)
{
    return (uint16_t)(counter & 0xFFFFu);
}

static uint16_t read_counter_hi(void)
{
    return (uint16_t)((counter >> 16) & 0xFFFFu);
}

static void write_counter(uint16_t value)
{
    counter = value;
}

static const RegistryEntry_t modbus_registry[] = {
    { 0,    "DEVICE_ID",    REGISTRY_TYPE_RO, read_device_id,     NULL },
    { 1,    "FW_VERSION",   REGISTRY_TYPE_RO, read_fw_version,    NULL },
    { 10,   "COUNTER_LO",   REGISTRY_TYPE_RW, read_counter,       write_counter },
    { 11,   "COUNTER_HI",   REGISTRY_TYPE_RO, read_counter_hi,    NULL },
    REGISTRY_END
};

int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000);

    SCB->VTOR = FLASH_BASE | VECTOR_TABLE_OFFSET;

    DEBUG_initTrace(SystemCoreClock);

    rtt_info("=== MODBUS Slave (Device Registry) ===\r\n");

    DeviceConfig_init();
    DeviceConfig_t *cfg = DeviceConfig_get();

    rtt_ok("Device ID: %d\r\n", cfg->device_id);
    rtt_ok("MODBUS Addr: %d\r\n", cfg->modbus_slave_addr);
    rtt_debug("Device Type: 0x%02x\r\n", cfg->device_type);
    rtt_debug("FW Version: %d\r\n", cfg->fw_version);

    DeviceRegistry_init(modbus_registry);

    const MODBUS_RTU_config_t mb_cfg = {
        .interframe_timeout_ms = 100,
        .max_registers_per_request = 100,
    };

    RS485_init(&rs485_config);
    ModbusSlave_init(&mb_cfg, cfg->modbus_slave_addr);

    rtt_ok("Ready - waiting for MODBUS requests\r\n");

    uint32_t last_ms = 0;

    while (1)
    {
        uint32_t now = SYS_getMs();

        if ((now - last_ms) >= 1000)
        {
            last_ms = now;
            counter++;
            rtt_info("Counter: %lu\r\n", counter);
        }

        ModbusSlave_process();
    }

    return 0;
}
