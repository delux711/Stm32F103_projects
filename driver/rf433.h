#ifndef RF433_H
#define RF433_H

#include <stdint.h>
#include "stm32f10x.h"

typedef void (*RF433_rxCallback_t)(uint8_t data);

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
} RF433_config_t;

void RF433_init(const RF433_config_t *config);
void RF433_setRxCallback(RF433_rxCallback_t rx_callback);
void RF433_send(const uint8_t *data, uint16_t length);
void RF433_usartIrqHandler(void);

#endif /* RF433_H */
