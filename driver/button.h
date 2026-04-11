#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef uint8_t (*BUTTON_readF)(void);  // načítanie stavu tlačidla (0 = stlačené, 1 = uvoľnené)
typedef uint32_t (*BUTTON_timeF)(void); // načítanie aktuálního času v ms
typedef void (*BUTTON_cbF)(void);       // callback pro udalost (single, double, triple, long)

typedef struct {
    BUTTON_readF read;     // funkce pro načítanie stavu tlačítka
    BUTTON_cbF   onSingle; // callback pro single click
    BUTTON_cbF   onDouble; // callback pro double click
    BUTTON_cbF   onTriple; // callback pro triple click
    BUTTON_cbF   onLong;   // callback pro long press
} BUTTON_config_t;

// Initialization
void BUTTON_init(BUTTON_config_t *cfg, uint32_t count, BUTTON_timeF timeFunc);
void BUTTON_process(void); // musí byť volané v hlavnej slučke programu

// void BUTTON_delayMs(uint32_t ms);

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
