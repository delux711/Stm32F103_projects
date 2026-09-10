#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include "stm32f10x.h"

typedef void (*RS485_rxCallback_t)(uint8_t data);
enum RS485_parity_t
{
    USART_PARITY_NONE = 0u,
    USART_PARITY_EVEN = USART_CR1_PCE,
    USART_PARITY_ODD  = (USART_CR1_PCE | USART_CR1_PS)
};
enum RS485_dataBits_t
{
    USART_DATA_BITS_8 = 0u,
    USART_DATA_BITS_9 = USART_CR1_M
};
typedef struct
{
    GPIO_TypeDef *txPort;
    uint8_t txPin;
    GPIO_TypeDef *rxPort;
    uint8_t rxPin;
    GPIO_TypeDef *dirPort;
    uint8_t dirPin;
    uint32_t usartRemapMask;
    uint32_t usartRemap;
    USART_TypeDef *usart;
    IRQn_Type usartIrqn;
    volatile uint32_t *usartRccReg;
    uint32_t usartRccBit;
    uint32_t baudrate;
    enum RS485_dataBits_t dataBits;
    enum RS485_parity_t parity;
} RS485_config_t;

void RS485_init(const RS485_config_t *config);
void RS485_setRxCallback(RS485_rxCallback_t rx_callback);
void RS485_send(const uint8_t *data, uint16_t length);
void RS485_goToMuteMode(void);
void RS485_usartIrqHandler(void);

#endif
