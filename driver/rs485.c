#include "rs485.h"

#include "uart.h"
#include "debug.h"
#include "gpio.h"
#include <stddef.h> // for NULL

// #define RS485_PARITY_ENABLE 1
// #define RS485_LOG_IRQ_COUNTER

static void RS485_gpioInit(const RS485_config_t *config);
static void RS485_txEnable(void);
static void RS485_txDisable(void);
static void RS485_logString(const char *msg);

static RS485_config_t rs485_config;

static void RS485_logString(const char *msg)
{
    DEBUG_writeString(msg);
}

void RS485_init(const RS485_config_t *config)
{
    rs485_config = *config;

    RS485_gpioInit(&rs485_config);
    UART_init(&config->uart);
    RS485_logString("RS485 init done\r\n");
}

void RS485_setRxCallback(RS485_rxCallback_t rx_callback)
{
    UART_setRxCallback(rx_callback);
}

void RS485_send(const uint8_t *data, uint16_t length)
{
    if ((data == 0) || (length == 0u))
    {
        return;
    }

    RS485_txEnable();
    UART_send(data, length);
    RS485_txDisable();
}

static void RS485_gpioInit(const RS485_config_t *config)
{
    if(config->dirPort != NULL)
        GPIO_enableClock(config->dirPort);

    if(config->dirPort != NULL)
        GPIO_configPin(config->dirPort, config->dirPin, GPIO_CFG_OUTPUT_PP_2MHZ);

    RS485_txDisable();
}

static void RS485_txEnable(void)
{
    if(rs485_config.dirPort != NULL)
    {
        rs485_config.dirPort->BSRR = (uint32_t)1u << rs485_config.dirPin;
    }
}

static void RS485_txDisable(void)
{
    if(rs485_config.dirPort != NULL)
    {
        rs485_config.dirPort->BRR = (uint32_t)1u << rs485_config.dirPin;
    }
}

void RS485_goToMuteMode(void)
{
    RS485_logString("Entering mute mode\r\n");
    UART_goToMuteMode();
}

void RS485_usartIrqHandler(void)
{
    UART_irqHandler();
}
