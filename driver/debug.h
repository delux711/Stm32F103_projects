#ifndef DEBUG_H
#define DEBUG_H
#include <stm32f10x.h>

void DEBUG_initTrace(uint32_t cpu_freq_hz);

static inline void DEBUG_sendChar(uint8_t ch, uint8_t channel)
{
    while (!(ITM->PORT[channel].u32));
    ITM->PORT[channel].u8 = ch;
}

#endif // DEBUG_H
