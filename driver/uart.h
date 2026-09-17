#ifndef UART_H
#define UART_H

#include <stdint.h>

#if defined(DRIVER_RS485_UART_SW_USE) && defined(DRIVER_RS485_UART_HW_USE)
#error "Select only one RS485 UART implementation"
#endif

#if defined(DRIVER_RS485_UART_SW_USE)
#include "uart_sw.h"
typedef UART_SW_config_t UART_config_t;
typedef void (*UART_rxCallback_t)(uint8_t data);
#define UART_init UART_SW_init
#define UART_setRxCallback UART_SW_setRxCallback
#define UART_setTxCallback UART_SW_setTxCallback
#define UART_goToMuteMode UART_SW_goToMuteMode
#define UART_irqHandler UART_SW_irqHandler
#define UART_DATA_BITS_4 UART_SW_DATA_BITS_4
#define UART_DATA_BITS_5 UART_SW_DATA_BITS_5
#define UART_DATA_BITS_6 UART_SW_DATA_BITS_6
#define UART_DATA_BITS_7 UART_SW_DATA_BITS_7
#define UART_DATA_BITS_8 UART_SW_DATA_BITS_8
#define UART_DATA_BITS_9 UART_SW_DATA_BITS_9
#define UART_PARITY_NONE UART_SW_PARITY_NONE
#define UART_PARITY_EVEN UART_SW_PARITY_EVEN
#define UART_PARITY_ODD UART_SW_PARITY_ODD
#define UART_STOP_BITS_1 UART_SW_STOP_BITS_1
#define UART_STOP_BITS_2 UART_SW_STOP_BITS_2

static inline void UART_sendBytes(const uint8_t *data, uint16_t length)
{
    if ((data == 0) || (length == 0u))
    {
        return;
    }

    for (uint16_t i = 0u; i < length; i++)
    {
        uint32_t word = data[i];
        UART_SW_send(&word, 1u);
    }
}
#else
#include "uart_hw.h"
typedef UART_HW_config_t UART_config_t;
typedef UART_HW_rxCallback_t UART_rxCallback_t;
#define UART_init UART_HW_init
#define UART_setRxCallback UART_HW_setRxCallback
#define UART_setTxCallback UART_HW_setTxCallback
#define UART_sendBytes UART_HW_send
#define UART_goToMuteMode UART_HW_goToMuteMode
#define UART_irqHandler UART_HW_irqHandler
#define UART_DATA_BITS_8 UART_HW_DATA_BITS_8
#define UART_DATA_BITS_9 UART_HW_DATA_BITS_9
#define UART_PARITY_NONE UART_HW_PARITY_NONE
#define UART_PARITY_EVEN UART_HW_PARITY_EVEN
#define UART_PARITY_ODD UART_HW_PARITY_ODD
#endif

#endif
