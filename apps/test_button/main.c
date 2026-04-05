#include <stm32f10x.h>
#include "system_stm32f10x.h"
#include "button.h"
#include "debug.h"

int main(void) {
    SystemInit();
    SystemCoreClockUpdate();

    DEBUG_initTrace(SystemCoreClock);
    BUTTON_init();
    SysTick_Config(SystemCoreClock / 1000);

    DEBUG_sendChar('H', 0);

    uint16_t counter = 0;
    for(;;) {
        // ITM_SendChar('A' + (counter++ % 26));
        __WFI();         // EXTI zobudí CPU
        // GPIOC->BSRR = (GPIOC->ODR & GPIO_ODR_ODR13) ? GPIO_BSRR_BR13 : GPIO_BSRR_BS13;
    }
    return 0;
}
