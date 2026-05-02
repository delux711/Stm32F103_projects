#ifndef DEBUG_H
#define DEBUG_H
#include "stm32f10x.h"
#include <stdbool.h>


void DEBUG_initTrace(uint32_t cpu_freq_hz);

static inline void DEBUG_sendChar(uint8_t ch, uint8_t channel)
{
    if ((CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 0u) return;
    if ((ITM->TCR & ITM_TCR_ITMENA_Msk) == 0u) return;
    if ((ITM->TER & (1UL << channel)) == 0u) return;
    ITM->PORT[channel].u8 = ch;
}

static inline void DEBUG_ledPinToggle(void) {
    // bit banding pre PC13 ODR registra je adresa 0x422201B4 = 0x42000000 + (0x1100c*32)+(0xD*4)
    // adresa bitu BS13 pre PORTC je: 0x42220234 = 0x42000000 + (0x11010*32)+(13*4)
    // adresa bitu BR13 pre PORTC je: 0x42220274 = 0x42000000 + (0x11010*32)+(29*4)
    GPIOC->BSRR = (GPIOC->ODR & GPIO_ODR_ODR13) ? GPIO_BSRR_BR13 : GPIO_BSRR_BS13;
}

static inline void DEBUG_ledPinOn(void)
{
    GPIOC->BSRR = GPIO_BSRR_BS13;
}

static inline void DEBUG_ledPinOff(void)
{
    GPIOC->BSRR = GPIO_BSRR_BR13;
}

static inline void DEBUG_setPin(bool on) {
    GPIOC->BSRR = on ? GPIO_BSRR_BS13 : GPIO_BSRR_BR13;
}

static inline bool DEBUG_getPin(void) {
    return (GPIOC->IDR & GPIO_IDR_IDR13) != 0;
}

#endif // DEBUG_H
