#include "rs485.h"

#include "stm32f10x.h"
#include "debug.h"
#include "gpio.h"

// #define RS485_PARITY_ENABLE 1
// #define RS485_LOG_IRQ_COUNTER

static void RS485_gpioInit(const RS485_config_t *config);
static void RS485_txEnable(void);
static void RS485_txDisable(void);
static uint32_t RS485_getUsartClockHz(void);
static void RS485_usartInit(void);
static void RS485_sendByte(uint8_t data);
static void RS485_logString(const char *msg);

static RS485_config_t rs485_config;
static USART_TypeDef *usart;
static RS485_rxCallback_t rs485_rx_callback = 0;
#ifdef RS485_LOG_IRQ_COUNTER
static volatile uint32_t RS485_irqCounter = 0u;
#endif

static void RS485_logString(const char *msg)
{
    DEBUG_writeString(msg);
}

void RS485_init(const RS485_config_t *config)
{
    rs485_config = *config;
    usart = config->usart;
    rs485_rx_callback = 0;
  #ifdef RS485_LOG_IRQ_COUNTER
    RS485_irqCounter = 0u;
  #endif

    RS485_gpioInit(&rs485_config);
    RS485_usartInit();
    RS485_logString("RS485 init done\r\n");
}

void RS485_setRxCallback(RS485_rxCallback_t rx_callback)
{
    rs485_rx_callback = rx_callback;
}

void RS485_send(const uint8_t *data, uint16_t length)
{
    if ((data == 0) || (length == 0u))
    {
        return;
    }

    RS485_txEnable();
    for (uint16_t i = 0u; i < length; i++)
    {
        RS485_sendByte(data[i]);
    }
    while ((usart->SR & USART_SR_TC) == 0u) { }
    RS485_txDisable();
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

static uint32_t RS485_getUsartClockHz(void)
{
    uint32_t ppre_bits;
    uint32_t div = 1u;

    /* urcenie, ci je USART na APB2 alebo APB1 */
    if (rs485_config.usartRccReg == &RCC->APB2ENR)
        ppre_bits = (RCC->CFGR >> 11) & 0x7u; /* PPRE2 */
    else
        ppre_bits = (RCC->CFGR >> 8) & 0x7u;  /* PPRE1 */

    /* 0xx: /1, 100:/2, 101:/4, 110:/8, 111:/16 */
    if (ppre_bits >= 4u)
    {
        div = 1u << (ppre_bits - 3u);
    }

    return SystemCoreClock / div;
}

static void RS485_usartInit(void)
{
    uint32_t usart_clk = RS485_getUsartClockHz();

    *rs485_config.usartRccReg |= rs485_config.usartRccBit;
    if (rs485_config.baudrate == 0u)
    {
        return;
    }

    usart->CR1 =
      #if RS485_PARITY_ENABLE
        USART_CR1_PCE |  // Parity control enable, parity selection is EVEN by default
      #endif
        USART_CR1_RE |
        USART_CR1_TE |
        USART_CR1_RXNEIE;
    /* BRR pre oversampling x16: BRR ~= fCK / baud, so zaokruhlenim */
    usart->BRR = (usart_clk + (rs485_config.baudrate / 2u)) / rs485_config.baudrate;
    usart->CR1 |= USART_CR1_UE;

    NVIC_EnableIRQ(rs485_config.usartIrqn);
}

static void RS485_sendByte(uint8_t data)
{
    while ((usart->SR & USART_SR_TXE) == 0u) { }
    usart->DR = (uint16_t)data;
}

void RS485_goToMuteMode(void)
{
    RS485_logString("Entering mute mode\r\n");
    usart->CR1 |= USART_CR1_RWU; // go to mute
}

void RS485_usartIrqHandler(void)
{
    if ((usart->SR & USART_SR_RXNE) != 0u)
    {
        uint8_t data = (uint8_t)(usart->DR & 0xFFu);
      #ifdef RS485_LOG_IRQ_COUNTER
        RS485_irqCounter++;
      #endif
        if (rs485_rx_callback != 0)
        {
            rs485_rx_callback(data);
        }
    }

    if ((usart->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE | USART_SR_IDLE)) != 0u)
    {
        RS485_logString("USART error\r\n");
        (void)usart->SR;
        (void)usart->DR;
    }
}
