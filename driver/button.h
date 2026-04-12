#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32f10x.h"

#define BUTTON_DEF(port, pin, single_cb, double_cb, triple_cb, long_cb) \
    { port, (1 << pin), pin, 0, 0, 0, 0, 0, 0, single_cb, double_cb, triple_cb, long_cb }

typedef void (*btnCb_t)(void);
typedef enum {
    BTN_IDLE = 0u,
    BTN_DEBOUNCE_PRESS,
    BTN_PRESSED,
    BTN_DEBOUNCE_RELEASE
} btnState_t;

typedef struct {
    /* HW */
    GPIO_TypeDef *port;
    uint16_t      pin_mask;
    uint8_t       exti_line;   // 0..15

    /* SW */
    btnState_t    state;
    uint32_t      timer;
    uint32_t      press_time;
    uint8_t       click_count;
    uint8_t       long_sent;
    uint8_t       active;      // 0 = EXTI mode, 1 = polling

    /* callbacks */
    btnCb_t       on_single;
    btnCb_t       on_double;
    btnCb_t       on_triple;
    btnCb_t       on_long;
} btn_t;

// Initialization
void BUTTON_init(const volatile btn_t *button_configs, uint32_t count);
void BUTTON_delayMs(uint32_t ms);

// IRQ handlers implemented in button.c
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);
void EXTI2_IRQHandler(void);
void EXTI3_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);
void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif // BUTTON_H
