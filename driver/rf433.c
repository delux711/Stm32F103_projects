#include "app_config.h"
#include "debug.h"
#include "rf433.h"
#include "stm32f10x.h"
#include "gpio.h"

/*
 * RF433_DBG - debug output macro.
 * Default: calls DEBUG_writeString().
 * To disable: add  #define RF433_DBG(msg)  to app_config.h.
 * Linker discards all related string literals when the macro is empty.
 */
#ifndef RF433_DBG
#  define RF433_DBG(msg)  DEBUG_writeString(msg)
#endif

static void RF433_gpioInit(const RF433_config_t *config);
static uint32_t RF433_getUsartClockHz(void);
static void RF433_usartInit(void);
static void RF433_sendByte(uint8_t data);

static RF433_config_t rf433_config;
static USART_TypeDef *usart;
static RF433_rxCallback_t rf433_rx_callback = 0;

void RF433_init(const RF433_config_t *config)
{
    rf433_config = *config;
    usart = config->usart;
    rf433_rx_callback = 0;

    RF433_gpioInit(&rf433_config);
    RF433_usartInit();
    RF433_DBG("RF433 init\r\n");
}

void RF433_setRxCallback(RF433_rxCallback_t rx_callback)
{
    rf433_rx_callback = rx_callback;
}

void RF433_send(const uint8_t *data, uint16_t length)
{
    if ((data == 0) || (length == 0u))
    {
        return;
    }

    for (uint16_t i = 0u; i < length; i++)
    {
        RF433_sendByte(data[i]);
    }
    while ((usart->SR & USART_SR_TC) == 0u) { }
    RF433_DBG("RF433 send done\r\n");
}

static void RF433_gpioInit(const RF433_config_t *config)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    GPIO_enableClock(config->txPort);
    GPIO_enableClock(config->rxPort);

    AFIO->MAPR = (AFIO->MAPR & ~config->usartRemapMask) | config->usartRemap;

    GPIO_configPin(config->txPort, config->txPin, GPIO_CFG_OUTPUT_AF_PP_2MHZ);
    GPIO_configPin(config->rxPort, config->rxPin, GPIO_CFG_INPUT_FLOATING);
}

static uint32_t RF433_getUsartClockHz(void)
{
    uint32_t ppre_bits;
    uint32_t div = 1u;

    if (rf433_config.usartRccReg == &RCC->APB2ENR)
        ppre_bits = (RCC->CFGR >> 11) & 0x7u; /* PPRE2 */
    else
        ppre_bits = (RCC->CFGR >> 8) & 0x7u;  /* PPRE1 */

    if (ppre_bits >= 4u)
    {
        div = 1u << (ppre_bits - 3u);
    }

    return SystemCoreClock / div;
}

static void RF433_usartInit(void)
{
    uint32_t usart_clk = RF433_getUsartClockHz();

    *rf433_config.usartRccReg |= rf433_config.usartRccBit;
    if (rf433_config.baudrate == 0u)
    {
        return;
    }

    usart->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
    usart->BRR = (usart_clk + (rf433_config.baudrate / 2u)) / rf433_config.baudrate;
    usart->CR1 |= USART_CR1_UE;

    NVIC_EnableIRQ(rf433_config.usartIrqn);
}

static void RF433_sendByte(uint8_t data)
{
    while ((usart->SR & USART_SR_TXE) == 0u) { }
    usart->DR = (uint16_t)data;
}

void RF433_usartIrqHandler(void)
{
    if ((usart->SR & USART_SR_RXNE) != 0u)
    {
        uint8_t data = (uint8_t)(usart->DR & 0xFFu);
        if (rf433_rx_callback != 0)
        {
            rf433_rx_callback(data);
        }
    }
}
