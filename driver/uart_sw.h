#ifndef UART_SW_H
#define UART_SW_H

#include <stdint.h>
#include "stm32f10x.h"

typedef void (*UART_SW_rxCallback_t)(uint32_t data);
typedef void (*UART_SW_txCallback_t)(void);

enum UART_SW_dataBits_t
{
    UART_SW_DATA_BITS_MIN = 4u,
    UART_SW_DATA_BITS_4 = 4u,
    UART_SW_DATA_BITS_5 = 5u,
    UART_SW_DATA_BITS_6 = 6u,
    UART_SW_DATA_BITS_7 = 7u,
    UART_SW_DATA_BITS_8 = 8u,
    UART_SW_DATA_BITS_9 = 9u,
    UART_SW_DATA_BITS_10 = 10u,
    UART_SW_DATA_BITS_11 = 11u,
    UART_SW_DATA_BITS_12 = 12u,
    UART_SW_DATA_BITS_13 = 13u,
    UART_SW_DATA_BITS_14 = 14u,
    UART_SW_DATA_BITS_15 = 15u,
    UART_SW_DATA_BITS_16 = 16u,
    UART_SW_DATA_BITS_17 = 17u,
    UART_SW_DATA_BITS_18 = 18u,
    UART_SW_DATA_BITS_19 = 19u,
    UART_SW_DATA_BITS_20 = 20u,
    UART_SW_DATA_BITS_21 = 21u,
    UART_SW_DATA_BITS_22 = 22u,
    UART_SW_DATA_BITS_23 = 23u,
    UART_SW_DATA_BITS_24 = 24u,
    UART_SW_DATA_BITS_25 = 25u,
    UART_SW_DATA_BITS_26 = 26u,
    UART_SW_DATA_BITS_27 = 27u,
    UART_SW_DATA_BITS_28 = 28u,
    UART_SW_DATA_BITS_29 = 29u,
    UART_SW_DATA_BITS_30 = 30u,
    UART_SW_DATA_BITS_31 = 31u,
    UART_SW_DATA_BITS_32 = 32u,
    UART_SW_DATA_BITS_MAX = 32u
};

enum UART_SW_parity_t
{
    UART_SW_PARITY_NONE = 0u,
    UART_SW_PARITY_EVEN,
    UART_SW_PARITY_ODD
};

enum UART_SW_stopBits_t
{
    UART_SW_STOP_BITS_1 = 1u,
    UART_SW_STOP_BITS_2 = 2u
};

typedef struct
{
    GPIO_TypeDef *txPort;
    uint8_t txPin;
    GPIO_TypeDef *rxPort;
    uint8_t rxPin;
    TIM_TypeDef *timer;
    IRQn_Type timerIrqn;
    volatile uint32_t *timerRccReg;
    uint32_t timerRccBit;
    uint32_t baudrate;
    enum UART_SW_dataBits_t dataBits;
    enum UART_SW_parity_t parity;
    enum UART_SW_stopBits_t stopBits;
    UART_SW_rxCallback_t rxCallback;
    UART_SW_txCallback_t txCallback;
} UART_SW_config_t;

void UART_SW_init(const UART_SW_config_t *config);
void UART_SW_setRxCallback(UART_SW_rxCallback_t rx_callback);
void UART_SW_setTxCallback(UART_SW_txCallback_t tx_callback);
void UART_SW_send(const uint32_t *data, uint16_t length);
void UART_SW_irqHandler(void);

#endif
