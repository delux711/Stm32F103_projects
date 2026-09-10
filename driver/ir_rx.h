#ifndef IR_RX_H
#define IR_RX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32f10x.h"

/*
 * IR Receiver driver – NEC protocol via demodulated IR receiver
 * (e.g. TSOP4838, VS1838B, HX1838, TSOP38238).
 *
 * The receiver output is active LOW. The driver uses EXTI falling edge
 * timestamps and decodes NEC timing intervals in microseconds.
 */

typedef struct
{
    GPIO_TypeDef      *port;             /* GPIO port of receiver output      */
    uint8_t            pin;              /* GPIO pin number 0-15              */
    uint8_t            exti_line;        /* EXTI line 0-15                    */
    TIM_TypeDef       *timer;            /* free-running timer, e.g. TIM2     */
    volatile uint32_t *timer_rcc_reg;    /* &RCC->APB1ENR or &RCC->APB2ENR    */
    uint32_t           timer_rcc_bit;    /* e.g. RCC_APB1ENR_TIM2EN           */
} IR_RX_config_t;

typedef struct
{
    uint32_t raw;           /* full 32-bit NEC frame (LSB first) */
    uint8_t  address;
    uint8_t  address_inv;
    uint8_t  command;
    uint8_t  command_inv;
    uint8_t  valid;         /* 1 if addr/cmd inverse bytes match */
} IR_RX_frame_t;

/**
 * @brief  Initialise NEC IR receiver.
 *         Configures GPIO input pull-up, EXTI falling-edge IRQ and
 *         free-running timer at 1 MHz for edge timestamping.
 * @param  config  Pointer to filled IR_RX_config_t.
 */
void IR_RX_init(const IR_RX_config_t *config);

/**
 * @brief  Global IRQ handler for EXTI lines.
 *         Call from EXTIx_IRQHandler / EXTI9_5_IRQHandler / EXTI15_10_IRQHandler.
 */
void IR_RX_irqGlobalHandler(void);

/**
 * @brief  Read one decoded NEC frame, if available.
 * @param  out_frame  Destination frame structure.
 * @return 1 if a frame was read, 0 if no new frame is ready.
 */
uint8_t IR_RX_readFrame(IR_RX_frame_t *out_frame);

/**
 * @brief  Read and clear repeat event flag.
 * @return 1 if NEC repeat event occurred, otherwise 0.
 */
uint8_t IR_RX_readRepeat(void);

/**
 * @brief  Reset decoder internal state.
 */
void IR_RX_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* IR_RX_H */