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

static void BSP_exti0IrqHandler(void);
static void BSP_exti1IrqHandler(void);
static void BSP_exti2IrqHandler(void);
static void BSP_exti3IrqHandler(void);
static void BSP_exti4IrqHandler(void);
static void BSP_exti9_5IrqHandler(void);
static void BSP_exti15_10IrqHandler(void);
static void BSP_sysTickHandler(void);
static void BSP_usart1IrqHandler(void);

static void BSP_exti0IrqHandler(void)
{
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}

static void BSP_exti1IrqHandler(void)
{
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}

static void BSP_exti2IrqHandler(void)
{
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}

static void BSP_exti3IrqHandler(void)
{
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}

static void BSP_exti4IrqHandler(void)
{
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}

static void BSP_exti9_5IrqHandler(void)
{
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}

static void BSP_exti15_10IrqHandler(void)
{
  #ifdef DRIVER_BUTTON_USE
    BUTTON_irqGlobalHandler();
  #endif
}

static void BSP_sysTickHandler(void)
{
    SYS_incrementMs();
  #ifdef DRIVER_BUTTON_USE
    BUTTON_process(); // spracovanie aktivnych tlacidiel
  #endif
}

static void BSP_usart1IrqHandler(void)
{
    RS485_usartIrqHandler();
}

void EXTI0_IRQHandler(void) {
    BSP_exti0IrqHandler();
}
void EXTI1_IRQHandler(void) {
    BSP_exti1IrqHandler();
}
void EXTI2_IRQHandler(void) {
    BSP_exti2IrqHandler();
}
void EXTI3_IRQHandler(void) {
    BSP_exti3IrqHandler();
}
void EXTI4_IRQHandler(void) {
    BSP_exti4IrqHandler();
}
void EXTI9_5_IRQHandler(void) {
    BSP_exti9_5IrqHandler();
}
void EXTI15_10_IRQHandler(void)
{
    BSP_exti15_10IrqHandler();
}

#ifdef DRIVER_SYSTICK_USE
void SysTick_Handler(void)
{
    BSP_sysTickHandler();
}
#endif

#ifdef DRIVER_RS485_USE
void USART1_IRQHandler(void)
{
    BSP_usart1IrqHandler();
}
#endif
