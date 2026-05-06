#include "app_config.h"
#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "rs485.h"
#include "debug.h"

static const char *APP_commandPing(void);
static const char *APP_commandTemp(void);
static const char *APP_commandPir(void);
static const char *APP_commandAll(void);
static const char *APP_commandLenka(void);
static const char *APP_commandParkside(void);

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

static const RS485_command_t app_command_table[] = {
    {"PING", APP_commandPing},
    {"TEMP", APP_commandTemp},
    {"PIR", APP_commandPir},
    {"ALL", APP_commandAll},
    {"LENKA", APP_commandLenka},
    {"PARKSIDE", APP_commandParkside}
};

#define APP_COMMAND_COUNT ((uint32_t)(sizeof(app_command_table) / sizeof(app_command_table[0])))

int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u);

    DEBUG_initTrace(SystemCoreClock);
    RS485_init(&rs485_config);
    RS485_setCommandTable(app_command_table, APP_COMMAND_COUNT);
    DEBUG_sendString("RS485 test\r\n", 0);

    while (1)
    {
        RS485_process();
        __WFI();
    }
}

static const char *APP_commandPing(void)
{
    return "PONG\r\n";
}

static const char *APP_commandTemp(void)
{
    return "TEMP=25.4\r\n";
}

static const char *APP_commandPir(void)
{
    return "PIR=0\r\n";
}

static const char *APP_commandAll(void)
{
    return "TEMP=25.4,PIR=0,HUM=40,LUX=120\r\n";
}

static const char *APP_commandLenka(void)
{
    return "Pusztaiova\r\n";
}

static const char *APP_commandParkside(void)
{
    return "POHAR\r\n";
}
