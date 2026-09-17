#ifndef UART_HW_H
#define UART_HW_H

#include <stdint.h>
#include "stm32f10x.h"

typedef void (*UART_HW_rxCallback_t)(uint8_t data);
typedef void (*UART_HW_txCallback_t)(void);

enum UART_HW_parity_t
{
    UART_HW_PARITY_NONE = 0u,
    UART_HW_PARITY_EVEN = USART_CR1_PCE,
    UART_HW_PARITY_ODD  = (USART_CR1_PCE | USART_CR1_PS)
};

enum UART_HW_dataBits_t
{
    UART_HW_DATA_BITS_8 = 0u,
    UART_HW_DATA_BITS_9 = USART_CR1_M
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
    enum UART_HW_dataBits_t dataBits;
    enum UART_HW_parity_t parity;
    UART_HW_rxCallback_t rxCallback;
    UART_HW_txCallback_t txCallback;
} UART_HW_config_t;

void UART_HW_init(const UART_HW_config_t *config);
void UART_HW_setRxCallback(UART_HW_rxCallback_t rx_callback);
void UART_HW_setTxCallback(UART_HW_txCallback_t tx_callback);
void UART_HW_send(const uint8_t *data, uint16_t length);
void UART_HW_goToMuteMode(void);
void UART_HW_irqHandler(void);

#endif