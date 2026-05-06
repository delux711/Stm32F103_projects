#include "rs485.h"

#include <stdbool.h>
#include <string.h>

#include "stm32f10x.h"
#include "systick.h"
#include "debug.h"
#include "gpio.h"

#define FLASH_ID_ADDRESS  (0x0801FC00u)
#define DEFAULT_NODE_ID   '1'//(0xF1u)
#define DELAY_RESPONSE_MS (3u)
#define RX_BUFFER_SIZE    (64u)

static uint8_t RS485_readNodeId(void);
static void RS485_initGlobalVariables(void);
static void RS485_gpioInit(const RS485_config_t *config);
static void RS485_txEnable(void);
static void RS485_txDisable(void);
static void RS485_usartInit(void);
static void RS485_goToMuteMode(void);
static void RS485_usartSendChar(char c);
static void RS485_usartSendString(const char *s);
static void RS485_processCommand(void);

static RS485_config_t rs485_config;
static USART_TypeDef *usart;
static uint8_t node_id = DEFAULT_NODE_ID;
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_index = 0u;
static volatile uint32_t command_due_tick = 0u;
static volatile bool command_pending = false;

void RS485_init(const RS485_config_t *config)
{
    rs485_config = *config;
    usart = config->usart;
    RS485_initGlobalVariables();
    RS485_gpioInit(&rs485_config);
    RS485_usartInit();
}

void RS485_process(void)
{
    if (command_pending && ((int32_t)(SYS_getMs() - command_due_tick) >= 0))
    {
        RS485_processCommand();
        rx_index = 0u;
        command_pending = false;
    }
}

static void RS485_initGlobalVariables(void)
{
    node_id = RS485_readNodeId();
    rx_index = 0u;
    command_due_tick = 0u;
    command_pending = false;
}

static void RS485_goToMuteMode(void)
{
    DEBUG_sendString("Entering mute mode\r\n", 0);
    usart->CR1 &= ~USART_CR1_UE;                                               // disable USART to change mode
    usart->CR1 = (usart->CR1 & ~USART_CR1_M) | USART_CR1_RWU | USART_CR1_WAKE; // back to mute
    usart->CR1 |= USART_CR1_UE;
}

static uint8_t RS485_readNodeId(void)
{
    uint8_t id = *(volatile uint8_t *)FLASH_ID_ADDRESS;

    if ((id == 0xFFu) || (id == 0x00u))
    {
        return DEFAULT_NODE_ID;
    }

    return id;
}

static void RS485_gpioInit(const RS485_config_t *config)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    GPIO_enableClock(config->txPort);
    GPIO_enableClock(config->rxPort);
    GPIO_enableClock(config->dirPort);

    AFIO->MAPR = (AFIO->MAPR & ~config->usartRemapMask) | config->usartRemap;

    GPIO_configPin(config->txPort, config->txPin, GPIO_CFG_OUTPUT_AF_PP_2MHZ);
    GPIO_configPin(config->rxPort, config->rxPin, GPIO_CFG_INPUT_FLOATING);
    GPIO_configPin(config->dirPort, config->dirPin, GPIO_CFG_OUTPUT_PP_2MHZ);

    RS485_txDisable();
}

static void RS485_txEnable(void)
{
    rs485_config.dirPort->BSRR = (uint32_t)1u << rs485_config.dirPin;
}

static void RS485_txDisable(void)
{
    rs485_config.dirPort->BRR = (uint32_t)1u << rs485_config.dirPin;
}

static void RS485_usartInit(void)
{
    *rs485_config.usartRccReg |= rs485_config.usartRccBit;
    usart->BRR = SystemCoreClock / rs485_config.baudrate;
    usart->CR1 =
        USART_CR1_PCE |  // Parity control enable, parity selection is EVEN by default
        USART_CR1_RE |
        USART_CR1_TE |
        USART_CR1_WAKE | // Wake on address
        USART_CR1_RXNEIE;
    usart->CR2 = (node_id & USART_CR2_ADD);
    usart->CR1 |= USART_CR1_RWU; // start in mute mode
    usart->CR1 |= USART_CR1_UE;

    NVIC_EnableIRQ(rs485_config.usartIrqn);
}

static void RS485_usartSendChar(char c)
{
    while ((usart->SR & USART_SR_TXE) == 0u) { }
    usart->DR = (uint16_t)c;
}

static void RS485_usartSendString(const char *s)
{
    RS485_txEnable();

    while (*s != '\0')
    {
        RS485_usartSendChar(*s);
        s++;
    }

    while ((usart->SR & USART_SR_TC) == 0u) { }
    RS485_txDisable();
    RS485_goToMuteMode();
}

static void RS485_processCommand(void)
{
    if (strcmp((const char *)&rx_buffer[1u], "PING") == 0)
    {
        RS485_usartSendString("PONG\r\n");
    }
    else if (strcmp((const char *)&rx_buffer[1u], "TEMP") == 0)
    {
        RS485_usartSendString("TEMP=25.4\r\n");
    }
    else if (strcmp((const char *)&rx_buffer[1u], "PIR") == 0)
    {
        RS485_usartSendString("PIR=0\r\n");
    }
    else if (strcmp((const char *)&rx_buffer[1u], "ALL") == 0)
    {
        RS485_usartSendString("TEMP=25.4,PIR=0,HUM=40,LUX=120\r\n");
    }
    else if (strcmp((const char *)&rx_buffer[1u], "LENKA") == 0)
    {
        RS485_usartSendString("Pusztaiova\r\n");
    }
    else if (strcmp((const char *)&rx_buffer[1u], "PARKSIDE") == 0)
    {
        RS485_usartSendString("POHAR\r\n");
    }
    else
    {
        RS485_usartSendString("ERR\r\n");
    }
}

void RS485_usartIrqHandler(void)
{
    // DEBUG_sendString("IRQ-UART\r\n", 0);
    if ((usart->CR1 & USART_CR1_WAKE) != 0u)
    {
        DEBUG_sendString("W", 0);
        usart->CR1 &= ~USART_CR1_UE;
        usart->CR1 = (usart->CR1 & ~USART_CR1_WAKE) | USART_CR1_M;
        usart->CR1 |= USART_CR1_UE;
        (void)usart->DR;  // read DR to clear
        return;
    }

    if ((usart->SR & USART_SR_RXNE) != 0u)
    {
        uint16_t data = usart->DR;
        uint8_t c = (uint8_t)(data & 0xFFu);
        // DEBUG_sendString("R-ch:", 0);
        // DEBUG_sendChar(c, 0);
        // DEBUG_sendString("\r\n", 0);

        if (command_pending)
        {
            return;
        }

        if (c == '\n')
        {
            DEBUG_sendString("Command received\r\n", 0);
            rx_buffer[rx_index] = 0u;
            command_due_tick = SYS_getMs() + DELAY_RESPONSE_MS;
            command_pending = true;
        }
        else if ((rx_index == 0u) && (c != node_id))  // first byte is address, must match node_id
        {
            DEBUG_sendString("Invalid node ID\r\n", 0);
            RS485_goToMuteMode();
            return;
        }
        else if ((rx_index < (RX_BUFFER_SIZE - 1u)) && (c != '\r'))
        {
            rx_buffer[rx_index++] = c;
        }
    }

    if ((usart->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0u)
    {
        DEBUG_sendString("USART error\r\n", 0);
        (void)usart->SR; // clear error flags
        (void)usart->DR;
        RS485_goToMuteMode();
        return;
    }
}
