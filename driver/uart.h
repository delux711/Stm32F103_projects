#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "stm32f10x.h"

typedef void (*UART_rxCallback_t)(uint8_t data);
typedef void (*UART_txCallback_t)(void);

enum UART_parity_t
{
    UART_PARITY_NONE = 0u,
    UART_PARITY_EVEN = USART_CR1_PCE,
    UART_PARITY_ODD  = (USART_CR1_PCE | USART_CR1_PS)
};

enum UART_dataBits_t
{
    UART_DATA_BITS_8 = 0u,
    UART_DATA_BITS_9 = USART_CR1_M
};

typedef struct
{
    GPIO_TypeDef *txPort;
    uint8_t txPin;
    GPIO_TypeDef *rxPort;
    uint8_t rxPin;
    uint32_t usartRemapMask;
    uint32_t usartRemap;
    USART_TypeDef *usart;
    IRQn_Type usartIrqn;
    volatile uint32_t *usartRccReg;
    uint32_t usartRccBit;
    uint32_t baudrate;
    enum UART_dataBits_t dataBits;
    enum UART_parity_t parity;
    UART_rxCallback_t rxCallback;
    UART_txCallback_t txCallback;
} UART_config_t;

void UART_init(const UART_config_t *config);
void UART_setRxCallback(UART_rxCallback_t rx_callback);
void UART_setTxCallback(UART_txCallback_t tx_callback);
void UART_send(const uint8_t *data, uint16_t length);
void UART_goToMuteMode(void);
void UART_irqHandler(void);

#endif