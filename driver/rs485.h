#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include "stm32f10x.h"

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
} RS485_config_t;

void RS485_init(const RS485_config_t *config);
void RS485_process(void);
void RS485_usartIrqHandler(void);

#endif
