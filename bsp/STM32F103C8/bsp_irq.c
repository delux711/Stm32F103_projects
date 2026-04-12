#include "app_config.h"

#ifdef DRIVER_BUTTON_USE
#include "button.h"
#endif
#ifdef DRIVER_SYSTICK_USE
#include "systick.h"
#endif

void EXTI0_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}
void EXTI1_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}
void EXTI2_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}
void EXTI3_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}
void EXTI4_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}
void EXTI9_5_IRQHandler(void) {
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}
void EXTI15_10_IRQHandler(void)
{
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}

#ifdef DRIVER_SYSTICK_USE
void SysTick_Handler(void)
{
    SYS_incrementMs();
  #ifdef DRIVER_BUTTON_USE
    BUTTON_process(); // spracovanie aktivnych tlacidiel
  #endif
}
#endif
