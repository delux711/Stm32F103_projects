#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "button.h"
#include "debug.h"

static void APP_initLedPin(void);
static void APP_btn0Single(void);
static void APP_btn0Double(void);
static void APP_btn0Triple(void);
static void APP_btn0Long(void);

static const btn_t buttonInit[] = {
    BUTTON_DEF(GPIOB, 12u, APP_btn0Single, APP_btn0Double, APP_btn0Triple, APP_btn0Long  ),  // Tlacidlo 0 – PB12
    BUTTON_DEF(GPIOC, 15u, APP_btn0Long  , APP_btn0Triple, APP_btn0Double, APP_btn0Single),  // Tlacidlo 1 – PC15
    BUTTON_DEF(GPIOA,  1u,      0u       , APP_btn0Double, APP_btn0Triple,      0u       )   // Tlacidlo 2 – PA1
};
#define BUTTON_COUNT ((uint32_t)(sizeof(buttonInit) / sizeof(buttonInit[0])))

int main(void) {
    SystemInit();
    SystemCoreClockUpdate();

    DEBUG_initTrace(SystemCoreClock);
    APP_initLedPin();
    BUTTON_init(buttonInit, BUTTON_COUNT);
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


static void APP_btn0Single(void) {
    DEBUG_sendChar('S', 0);
    DEBUG_ledPinToggle();
    GPIOB->BSRR = (GPIOB->ODR & GPIO_ODR_ODR13) ? GPIO_BSRR_BR13 : GPIO_BSRR_BS13;
}
static void APP_btn0Double(void) {
    DEBUG_sendChar('D', 0);
    // DEBUG_ledPinToggle();
    GPIOB->BSRR = (GPIOB->ODR & GPIO_ODR_ODR14) ? GPIO_BSRR_BR14 : GPIO_BSRR_BS14;
}
static void APP_btn0Triple(void) {
    DEBUG_sendChar('T', 0);
    // DEBUG_ledPinToggle();
    GPIOB->BSRR = (GPIOB->ODR & GPIO_ODR_ODR15) ? GPIO_BSRR_BR15 : GPIO_BSRR_BS15;
}
static void APP_btn0Long(void) {
    DEBUG_sendChar('L', 0);
    // DEBUG_ledPinToggle();
    GPIOA->BSRR = (GPIOA->ODR & GPIO_ODR_ODR8) ? GPIO_BSRR_BR8 : GPIO_BSRR_BS8;
}

static void APP_initLedPin(void) {
    uint32_t reg;
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    reg = GPIOC->CRH;
    reg &= ~((uint32_t)GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    reg |= GPIO_CRH_MODE13; // 11: Output mode, max speed 50 MHz
    GPIOC->CRH = reg;

    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    reg = GPIOB->CRH;
    reg &= ~((uint32_t)GPIO_CRH_CNF13 | GPIO_CRH_MODE13 | GPIO_CRH_CNF14 | GPIO_CRH_MODE14 | GPIO_CRH_CNF15 | GPIO_CRH_MODE15);
    reg |= GPIO_CRH_MODE13 | GPIO_CRH_MODE14 | GPIO_CRH_MODE15; // 11: Output mode, max speed 50 MHz
    GPIOB->CRH = reg;

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    reg = GPIOA->CRH;
    reg &= ~((uint32_t)GPIO_CRH_CNF8 | GPIO_CRH_MODE8);
    reg |= GPIO_CRH_MODE8; // 11: Output mode, max speed 50 MHz
    GPIOA->CRH = reg;
}
