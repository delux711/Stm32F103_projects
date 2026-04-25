#include "rs485.h"

#include <stdbool.h>
#include <string.h>

#include "stm32f10x.h"
#include "systick.h"

#define FLASH_ID_ADDRESS  (0x0801FC00u)
#define DEFAULT_NODE_ID   (0xF1u)
#define DELAY_RESPONSE_MS (3u)
#define RX_BUFFER_SIZE    (64u)

#define GPIO_CFG_INPUT_FLOATING       (0x4u)
#define GPIO_CFG_OUTPUT_PP_2MHZ       (0x2u)
#define GPIO_CFG_OUTPUT_AF_PP_50MHZ   (0xBu)

static uint8_t readNodeId(void);
static void initGlobalVariables(void);
static void gpioInit(const RS485_config_t *config);
static void gpioEnableClock(GPIO_TypeDef *port);
static void gpioConfigPin(GPIO_TypeDef *port, uint8_t pin, uint32_t config);
static void txEnable(void);
static void txDisable(void);
static void usartInit(void);
static void goToMuteMode(void);
static void usartSendChar(char c);
static void usartSendString(const char *s);
static void processCommand(void);

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
    initGlobalVariables();
    gpioInit(&rs485_config);
    usartInit();
    SysTick_Config(SystemCoreClock / 1000u);
}

void RS485_process(void)
{
	if (command_pending && ((int32_t)(SYS_getMs() - command_due_tick) >= 0))
	{
		processCommand();
		rx_index = 0u;
		command_pending = false;
	}
}

static void initGlobalVariables(void)
{
	node_id = readNodeId();
	rx_index = 0u;
	command_due_tick = 0u;
	command_pending = false;
}

static void goToMuteMode(void)
{
	usart->CR1 &= ~USART_CR1_UE;
	usart->CR1 = (usart->CR1 & ~USART_CR1_M) | USART_CR1_RWU | USART_CR1_WAKE;
	usart->CR1 |= USART_CR1_UE;
}

static uint8_t readNodeId(void)
{
	uint8_t id = *(volatile uint8_t *)FLASH_ID_ADDRESS;

	if ((id == 0xFFu) || (id == 0x00u))
	{
		return DEFAULT_NODE_ID;
	}

	return id;
}

static void gpioInit(const RS485_config_t *config)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    gpioEnableClock(config->txPort);
    gpioEnableClock(config->rxPort);
    gpioEnableClock(config->dirPort);

    AFIO->MAPR = (AFIO->MAPR & ~config->usartRemapMask) | config->usartRemap;

    gpioConfigPin(config->txPort, config->txPin, GPIO_CFG_OUTPUT_AF_PP_50MHZ);
    gpioConfigPin(config->rxPort, config->rxPin, GPIO_CFG_INPUT_FLOATING);
    gpioConfigPin(config->dirPort, config->dirPin, GPIO_CFG_OUTPUT_PP_2MHZ);

    txDisable();
}

static void gpioEnableClock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    }
    else if (port == GPIOB)
    {
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    }
    else if (port == GPIOC)
    {
        RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    }
}

static void gpioConfigPin(GPIO_TypeDef *port, uint8_t pin, uint32_t config)
{
    volatile uint32_t *reg;
    uint32_t shift;
    uint32_t value;

    if (pin < 8u)
    {
        reg = &port->CRL;
        shift = (uint32_t)pin * 4u;
    }
    else
    {
        reg = &port->CRH;
        shift = (uint32_t)(pin - 8u) * 4u;
    }

    value = *reg;
    value &= ~(0xFu << shift);
    value |= (config << shift);
    *reg = value;
}

static void txEnable(void)
{
    rs485_config.dirPort->BSRR = (uint32_t)1u << rs485_config.dirPin;
}

static void txDisable(void)
{
    rs485_config.dirPort->BRR = (uint32_t)1u << rs485_config.dirPin;
}

static void usartInit(void)
{
	*rs485_config.usartRccReg |= rs485_config.usartRccBit;
	usart->BRR = SystemCoreClock / 9600u;
	usart->CR1 =
		USART_CR1_PCE |
		USART_CR1_RE |
		USART_CR1_TE |
		USART_CR1_WAKE |
		USART_CR1_RXNEIE;
	usart->CR2 = (node_id & USART_CR2_ADD);
	usart->CR1 |= USART_CR1_RWU;
	usart->CR1 |= USART_CR1_UE;

	NVIC_EnableIRQ(rs485_config.usartIrqn);
}

static void usartSendChar(char c)
{
	while ((usart->SR & USART_SR_TXE) == 0u)
	{
	}

	usart->DR = (uint16_t)c;
}

static void usartSendString(const char *s)
{
    txEnable();

    while (*s != '\0')
    {
        usartSendChar(*s);
        s++;
    }

    while ((usart->SR & USART_SR_TC) == 0u)
    {
    }

    txDisable();
    goToMuteMode();
}

static void processCommand(void)
{
	if (strcmp((const char *)&rx_buffer[1u], "PING") == 0)
	{
		usartSendString("PONG\r\n");
	}
	else if (strcmp((const char *)&rx_buffer[1u], "TEMP") == 0)
	{
		usartSendString("TEMP=25.4\r\n");
	}
	else if (strcmp((const char *)&rx_buffer[1u], "PIR") == 0)
	{
		usartSendString("PIR=0\r\n");
	}
	else if (strcmp((const char *)&rx_buffer[1u], "ALL") == 0)
	{
		usartSendString("TEMP=25.4,PIR=0,HUM=40,LUX=120\r\n");
	}
	else if (strcmp((const char *)&rx_buffer[1u], "LENKA") == 0)
	{
		usartSendString("Pusztaiova\r\n");
	}
	else if (strcmp((const char *)&rx_buffer[1u], "PARKSIDE") == 0)
	{
		usartSendString("POHAR\r\n");
	}
	else
	{
		usartSendString("ERR\r\n");
	}
}

void RS485_usartIrqHandler(void)
{
	if ((usart->CR1 & USART_CR1_WAKE) != 0u)
	{
		usart->CR1 &= ~USART_CR1_UE;
		usart->CR1 = (usart->CR1 & ~USART_CR1_WAKE) | USART_CR1_M;
		usart->CR1 |= USART_CR1_UE;
		(void)usart->DR;
		return;
	}

	if ((usart->SR & USART_SR_RXNE) != 0u)
	{
		uint16_t data = usart->DR;
		uint8_t c = (uint8_t)(data & 0xFFu);

		if (command_pending)
		{
			return;
		}

		if (c == '\n')
		{
			rx_buffer[rx_index] = 0u;
			command_due_tick = SYS_getMs() + DELAY_RESPONSE_MS;
			command_pending = true;
		}
		else if ((rx_index == 0u) && (c != node_id))
		{
			goToMuteMode();
			return;
		}
		else if ((rx_index < (RX_BUFFER_SIZE - 1u)) && (c != '\r'))
		{
			rx_buffer[rx_index++] = c;
		}
	}

	if ((usart->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) != 0u)
	{
		(void)usart->SR;
		(void)usart->DR;
		goToMuteMode();
	}
}
