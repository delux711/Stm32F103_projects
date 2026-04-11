#include <stm32f10x.h>
#include "system_stm32f10x.h"
#include "button.h"
#include "debug.h"

uint8_t btn0_read(void) {
    return (GPIOB->IDR & (1 << 12)) ? 1 : 0;
}
uint8_t btn1_read(void) {
    return (GPIOC->IDR & (1 << 15)) ? 1 : 0;
}
uint8_t btn2_read(void) {
    return (GPIOA->IDR & (1 << 1)) ? 1 : 0;
}

static void btn0_single(void) {
    DEBUG_sendChar('S', 0);
    DEBUG_ledPinToggle();
}
static void btn0_double(void) {
    DEBUG_sendChar('D', 0);
}
static void btn0_triple(void) {
    DEBUG_sendChar('T', 0);
}
static void btn0_long(void) {
    DEBUG_sendChar('L', 0);
}

int main(void) {
    SystemInit();
    SystemCoreClockUpdate();

    DEBUG_initTrace(SystemCoreClock);
    SysTick_Config(SystemCoreClock / 1000);

    BUTTON_config_t buttons[] = {
        {
            .read = btn0_read,
            .onSingle = btn0_single,
            .onDouble = btn0_double,
            .onTriple = btn0_triple,
            .onLong = btn0_long
        },
        {
            .read = btn1_read,
            .onSingle = NULL,
            .onDouble = NULL,
            .onTriple = NULL,
            .onLong = NULL
        },
        {
            .read = btn2_read,
            .onSingle = NULL,
            .onDouble = NULL,
            .onTriple = NULL,
            .onLong = NULL
        }
     };
    BUTTON_init(buttons, 3, NULL);

    DEBUG_sendChar('H', 0);

    uint16_t counter = 0;
    for(;;) {
        // ITM_SendChar('A' + (counter++ % 26));
        __WFI();         // EXTI zobudí CPU
        // GPIOC->BSRR = (GPIOC->ODR & GPIO_ODR_ODR13) ? GPIO_BSRR_BR13 : GPIO_BSRR_BS13;
    }
    return 0;
}
