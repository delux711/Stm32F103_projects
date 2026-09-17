#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include "uart.h"

typedef void (*RS485_rxCallback_t)(uint8_t data);

typedef struct
{
    UART_config_t uart;
    GPIO_TypeDef *dirPort;
    uint8_t dirPin;
} RS485_config_t;

void RS485_init(const RS485_config_t *config);
void RS485_setRxCallback(RS485_rxCallback_t rx_callback);
void RS485_send(const uint8_t *data, uint16_t length);
void RS485_goToMuteMode(void);
void RS485_usartIrqHandler(void);

#endif
