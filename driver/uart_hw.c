#include "uart_hw.h"

#include "debug.h"
#include "gpio.h"

static UART_HW_config_t uart_hw_config;
static UART_HW_rxCallback_t uart_hw_rx_callback = 0;
static UART_HW_txCallback_t uart_hw_tx_callback = 0;

static uint32_t UART_HW_getClockHz(void);
static void UART_HW_gpioInit(void);
static void UART_HW_usartInit(void);
static void UART_HW_sendByte(uint8_t data);

void UART_HW_init(const UART_HW_config_t *config)
{
    if (config == 0)
    {
        return;
    }

    uart_hw_config = *config;
    uart_hw_rx_callback = config->rxCallback;
    uart_hw_tx_callback = config->txCallback;
    UART_HW_gpioInit();
    UART_HW_usartInit();
}

void UART_HW_setRxCallback(UART_HW_rxCallback_t rx_callback)
{
    uart_hw_rx_callback = rx_callback;
}

void UART_HW_setTxCallback(UART_HW_txCallback_t tx_callback)
{
    uart_hw_tx_callback = tx_callback;
}

void UART_HW_send(const uint8_t *data, uint16_t length)
{
    if ((data == 0) || (length == 0u))
    {
        return;
    }

    for (uint16_t i = 0u; i < length; i++)
    {
        UART_HW_sendByte(data[i]);
    }

    while ((uart_hw_config.usart->SR & USART_SR_TC) == 0u) { }

    if (uart_hw_tx_callback != 0)
    {
        uart_hw_tx_callback();
    }
}

void UART_HW_goToMuteMode(void)
{
    uart_hw_config.usart->CR1 |= USART_CR1_RWU;
}

void UART_HW_irqHandler(void)
{
    USART_TypeDef *usart = uart_hw_config.usart;

    if ((usart->SR & USART_SR_RXNE) != 0u)
    {
        uint8_t data = (uint8_t)(usart->DR & 0xFFu);
        if (uart_hw_rx_callback != 0)
        {
            uart_hw_rx_callback(data);
        }
    }

    if ((usart->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE | USART_SR_IDLE)) != 0u)
    {
        DEBUG_writeString("USART error\r\n");
        DEBUG_writeString("SR=0x");
        DEBUG_writeHex32(usart->SR);
        DEBUG_writeString("\r\n");
        (void)usart->SR;
        (void)usart->DR;
    }
}

static void UART_HW_gpioInit(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    GPIO_enableClock(uart_hw_config.txPort);
    GPIO_enableClock(uart_hw_config.rxPort);
    AFIO->MAPR = (AFIO->MAPR & ~uart_hw_config.usartRemapMask) | uart_hw_config.usartRemap;
    GPIO_configPin(uart_hw_config.txPort, uart_hw_config.txPin, GPIO_CFG_OUTPUT_AF_PP_2MHZ);
    GPIO_configPin(uart_hw_config.rxPort, uart_hw_config.rxPin, GPIO_CFG_INPUT_FLOATING);
}

static uint32_t UART_HW_getClockHz(void)
{
    uint32_t ppre_bits;
    uint32_t div = 1u;

    if (uart_hw_config.usartRccReg == &RCC->APB2ENR)
    {
        ppre_bits = (RCC->CFGR >> 11) & 0x7u;
    }
    else
    {
        ppre_bits = (RCC->CFGR >> 8) & 0x7u;
    }

    if (ppre_bits >= 4u)
    {
        div = 1u << (ppre_bits - 3u);
    }

    return SystemCoreClock / div;
}

static void UART_HW_usartInit(void)
{
    USART_TypeDef *usart = uart_hw_config.usart;
    uint32_t usart_clk = UART_HW_getClockHz();

    *uart_hw_config.usartRccReg |= uart_hw_config.usartRccBit;
    if (uart_hw_config.baudrate == 0u)
    {
        return;
    }

    usart->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
    usart->CR1 &= ~(USART_CR1_PCE | USART_CR1_PS | USART_CR1_M);
    usart->CR1 |= uart_hw_config.parity | uart_hw_config.dataBits;
    usart->BRR = (usart_clk + (uart_hw_config.baudrate / 2u)) / uart_hw_config.baudrate;
    usart->CR1 |= USART_CR1_UE;
    NVIC_EnableIRQ(uart_hw_config.usartIrqn);
}

static void UART_HW_sendByte(uint8_t data)
{
    while ((uart_hw_config.usart->SR & USART_SR_TXE) == 0u) { }
    uart_hw_config.usart->DR = (uint16_t)data;
}