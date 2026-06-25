#include "app_config.h"

#ifdef DRIVER_BUTTON_USE
  #include "button.h"
#endif
#ifdef DRIVER_SYSTICK_USE
  #include "systick.h"
#endif
#ifdef DRIVER_RS485_USE
  #include "rs485.h"
#endif
#ifdef DRIVER_IR_RX_USE
  #include "ir_rx.h"
#endif
#ifdef DRIVER_RF433_USE
  #include "rf433.h"
#endif

void EXTI0_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
  #ifdef DRIVER_IR_RX_USE
    IR_RX_irqGlobalHandler();
  #endif
}
void EXTI1_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
  #ifdef DRIVER_IR_RX_USE
    IR_RX_irqGlobalHandler();
  #endif
}
void EXTI2_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
  #ifdef DRIVER_IR_RX_USE
    IR_RX_irqGlobalHandler();
  #endif
}
void EXTI3_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
  #ifdef DRIVER_IR_RX_USE
    IR_RX_irqGlobalHandler();
  #endif
}
void EXTI4_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
  #ifdef DRIVER_IR_RX_USE
    IR_RX_irqGlobalHandler();
  #endif
}
void EXTI9_5_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
  #ifdef DRIVER_IR_RX_USE
    IR_RX_irqGlobalHandler();
  #endif
}
void EXTI15_10_IRQHandler(void)
{
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
  #ifdef DRIVER_IR_RX_USE
    IR_RX_irqGlobalHandler();
  #endif
}

void SysTick_Handler(void)
{
  #ifdef DRIVER_SYSTICK_USE
  SYS_incrementMs();
  #endif
  #ifdef DRIVER_BUTTON_USE
    BUTTON_process(); // spracovanie aktivnych tlacidiel
  #endif
}

void USART1_IRQHandler(void)
{
  #ifdef DRIVER_RS485_USE
    RS485_usartIrqHandler();
  #endif
}

void USART2_IRQHandler(void)
{
  #ifdef DRIVER_RF433_USE
    RF433_usartIrqHandler();
  #endif
}
