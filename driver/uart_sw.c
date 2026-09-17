#include "uart_sw.h"

#include "gpio.h"

#define UART_SW_OVERSAMPLE 8u

enum UART_SW_rxState_t { UART_SW_RX_IDLE = 0u, UART_SW_RX_START, UART_SW_RX_DATA, UART_SW_RX_PARITY, UART_SW_RX_STOP };
enum UART_SW_txState_t { UART_SW_TX_IDLE = 0u, UART_SW_TX_START, UART_SW_TX_DATA, UART_SW_TX_PARITY, UART_SW_TX_STOP };

static UART_SW_config_t uart_sw_config;
static UART_SW_rxCallback_t uart_sw_rx_callback;
static UART_SW_txCallback_t uart_sw_tx_callback;
static volatile enum UART_SW_rxState_t uart_sw_rx_state;
static volatile enum UART_SW_txState_t uart_sw_tx_state;
static volatile uint8_t uart_sw_rx_ticks;
static volatile uint8_t uart_sw_tx_ticks;
static volatile uint8_t uart_sw_rx_bit;
static volatile uint8_t uart_sw_tx_bit;
static volatile uint32_t uart_sw_rx_data;
static volatile uint32_t uart_sw_tx_data;
static volatile uint8_t uart_sw_rx_parity;
static volatile uint8_t uart_sw_tx_parity;
static volatile uint8_t uart_sw_tx_stop_bits;

static uint32_t UART_SW_getTimerClockHz(void);
static void UART_SW_timerInit(void);
static uint8_t UART_SW_readRx(void);
static void UART_SW_writeTx(uint8_t value);
static uint8_t UART_SW_makeParity(uint32_t data);
static void UART_SW_txStartWord(uint32_t data);
static void UART_SW_rxTick(void);
static void UART_SW_txTick(void);

void UART_SW_init(const UART_SW_config_t *config)
{
    if ((config == 0) || (config->baudrate == 0u)
        || (config->dataBits < UART_SW_DATA_BITS_MIN)
        || (config->dataBits > UART_SW_DATA_BITS_MAX)) return;

    uart_sw_config = *config;
    uart_sw_rx_callback = config->rxCallback;
    uart_sw_tx_callback = config->txCallback;
    uart_sw_rx_state = UART_SW_RX_IDLE;
    uart_sw_tx_state = UART_SW_TX_IDLE;

    GPIO_enableClock(config->txPort);
    GPIO_enableClock(config->rxPort);
    GPIO_configPin(config->txPort, config->txPin, GPIO_CFG_OUTPUT_PP_2MHZ);
    GPIO_configPin(config->rxPort, config->rxPin, GPIO_CFG_INPUT_PULL);
    config->txPort->BSRR = (uint32_t)1u << config->txPin;
    config->rxPort->BSRR = (uint32_t)1u << config->rxPin;
    UART_SW_timerInit();
}

void UART_SW_setRxCallback(UART_SW_rxCallback_t rx_callback) { uart_sw_rx_callback = rx_callback; }
void UART_SW_setTxCallback(UART_SW_txCallback_t tx_callback) { uart_sw_tx_callback = tx_callback; }

void UART_SW_send(const uint32_t *data, uint16_t length)
{
    if ((data == 0) || (length == 0u)) return;

    for (uint16_t i = 0u; i < length; i++)
    {
        while (uart_sw_tx_state != UART_SW_TX_IDLE) { }
        UART_SW_txStartWord(data[i]);
    }
    while (uart_sw_tx_state != UART_SW_TX_IDLE) { }
    if (uart_sw_tx_callback != 0) uart_sw_tx_callback();
}

void UART_SW_irqHandler(void)
{
    if ((uart_sw_config.timer->SR & TIM_SR_UIF) == 0u) return;
    uart_sw_config.timer->SR &= ~TIM_SR_UIF;
    UART_SW_rxTick();
    UART_SW_txTick();
}

static uint32_t UART_SW_getTimerClockHz(void)
{
    uint32_t ppre_bits;
    uint32_t div = 1u;

    if (uart_sw_config.timerRccReg == &RCC->APB2ENR) ppre_bits = (RCC->CFGR >> 11) & 0x7u;
    else ppre_bits = (RCC->CFGR >> 8) & 0x7u;
    if (ppre_bits >= 4u) div = 1u << (ppre_bits - 3u);
    return (ppre_bits >= 4u) ? (SystemCoreClock / div) * 2u : SystemCoreClock;
}

static void UART_SW_timerInit(void)
{
    TIM_TypeDef *timer = uart_sw_config.timer;
    uint32_t tick_rate = uart_sw_config.baudrate * UART_SW_OVERSAMPLE;
    uint32_t timer_ticks = (UART_SW_getTimerClockHz() + tick_rate / 2u) / tick_rate;
    if (timer_ticks == 0u) timer_ticks = 1u;

    *uart_sw_config.timerRccReg |= uart_sw_config.timerRccBit;
    timer->CR1 = 0u;
    timer->PSC = 0u;
    timer->ARR = timer_ticks - 1u;
    timer->CNT = 0u;
    timer->EGR = TIM_EGR_UG;
    timer->SR = 0u;
    timer->DIER = TIM_DIER_UIE;
    timer->CR1 = TIM_CR1_CEN;
    NVIC_EnableIRQ(uart_sw_config.timerIrqn);
}

static uint8_t UART_SW_readRx(void)
{
    return (uart_sw_config.rxPort->IDR & ((uint32_t)1u << uart_sw_config.rxPin)) != 0u;
}

static void UART_SW_writeTx(uint8_t value)
{
    if (value != 0u) uart_sw_config.txPort->BSRR = (uint32_t)1u << uart_sw_config.txPin;
    else uart_sw_config.txPort->BRR = (uint32_t)1u << uart_sw_config.txPin;
}

static uint8_t UART_SW_makeParity(uint32_t data)
{
    uint8_t parity = 0u;
    for (uint8_t i = 0u; i < (uint8_t)uart_sw_config.dataBits; i++) parity ^= (uint8_t)((data >> i) & 1u);
    return parity;
}

static void UART_SW_txStartWord(uint32_t data)
{
    uart_sw_tx_data = data;
    uart_sw_tx_bit = 0u;
    uart_sw_tx_parity = UART_SW_makeParity(data);
    uart_sw_tx_stop_bits = (uint8_t)uart_sw_config.stopBits;
    uart_sw_tx_ticks = UART_SW_OVERSAMPLE;
    uart_sw_tx_state = UART_SW_TX_START;
    UART_SW_writeTx(0u);
}

static void UART_SW_rxTick(void)
{
    if (uart_sw_rx_state == UART_SW_RX_IDLE)
    {
        if (UART_SW_readRx() == 0u)
        {
            uart_sw_rx_ticks = UART_SW_OVERSAMPLE / 2u;
            uart_sw_rx_state = UART_SW_RX_START;
        }
        return;
    }
    if (uart_sw_rx_ticks != 0u) { uart_sw_rx_ticks--; return; }
    uart_sw_rx_ticks = UART_SW_OVERSAMPLE - 1u;

    if (uart_sw_rx_state == UART_SW_RX_START)
    {
        if (UART_SW_readRx() == 0u)
        {
            uart_sw_rx_data = 0u; uart_sw_rx_bit = 0u; uart_sw_rx_parity = 0u;
            uart_sw_rx_state = UART_SW_RX_DATA;
        }
        else uart_sw_rx_state = UART_SW_RX_IDLE;
    }
    else if (uart_sw_rx_state == UART_SW_RX_DATA)
    {
        if (UART_SW_readRx() != 0u)
        {
            uart_sw_rx_data |= (uint32_t)1u << uart_sw_rx_bit;
            uart_sw_rx_parity ^= 1u;
        }
        uart_sw_rx_bit++;
        if (uart_sw_rx_bit >= (uint8_t)uart_sw_config.dataBits)
            uart_sw_rx_state = (uart_sw_config.parity == UART_SW_PARITY_NONE) ? UART_SW_RX_STOP : UART_SW_RX_PARITY;
    }
    else if (uart_sw_rx_state == UART_SW_RX_PARITY)
    {
        uint8_t expected = (uart_sw_config.parity == UART_SW_PARITY_EVEN) ? uart_sw_rx_parity : (uint8_t)!uart_sw_rx_parity;
        uart_sw_rx_state = (UART_SW_readRx() == expected) ? UART_SW_RX_STOP : UART_SW_RX_IDLE;
    }
    else
    {
        if ((UART_SW_readRx() != 0u) && (uart_sw_rx_callback != 0)) uart_sw_rx_callback(uart_sw_rx_data);
        uart_sw_rx_state = UART_SW_RX_IDLE;
    }
}

static void UART_SW_txTick(void)
{
    if (uart_sw_tx_state == UART_SW_TX_IDLE) return;
    if (uart_sw_tx_ticks != 0u) { uart_sw_tx_ticks--; return; }
    uart_sw_tx_ticks = UART_SW_OVERSAMPLE - 1u;

    if (uart_sw_tx_state == UART_SW_TX_START)
    {
        UART_SW_writeTx((uint8_t)(uart_sw_tx_data & 1u));
        uart_sw_tx_bit = 1u;
        uart_sw_tx_state = UART_SW_TX_DATA;
    }
    else if (uart_sw_tx_state == UART_SW_TX_DATA)
    {
        if (uart_sw_tx_bit < (uint8_t)uart_sw_config.dataBits)
        {
            UART_SW_writeTx((uint8_t)((uart_sw_tx_data >> uart_sw_tx_bit) & 1u));
            uart_sw_tx_bit++;
        }
        else if (uart_sw_config.parity != UART_SW_PARITY_NONE)
        {
            UART_SW_writeTx((uart_sw_config.parity == UART_SW_PARITY_EVEN) ? uart_sw_tx_parity : (uint8_t)!uart_sw_tx_parity);
            uart_sw_tx_state = UART_SW_TX_PARITY;
        }
        else { UART_SW_writeTx(1u); uart_sw_tx_stop_bits = (uint8_t)uart_sw_config.stopBits; uart_sw_tx_state = UART_SW_TX_STOP; }
    }
    else if (uart_sw_tx_state == UART_SW_TX_PARITY)
    {
        UART_SW_writeTx(1u); uart_sw_tx_stop_bits = (uint8_t)uart_sw_config.stopBits; uart_sw_tx_state = UART_SW_TX_STOP;
    }
    else if (uart_sw_tx_stop_bits > 1u) uart_sw_tx_stop_bits--;
    else uart_sw_tx_state = UART_SW_TX_IDLE;
}
