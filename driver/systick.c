#include "systick.h"
#include "stm32f10x.h"

volatile uint32_t SYS_time_ms;

uint32_t SYS_getMs(void)
{
    return SYS_time_ms;
}

void SYS_incrementMs(void)
{
    SYS_time_ms++;
}

void SYS_delayMs(uint32_t ms) {
    uint32_t start = SYS_time_ms;
    while ((SYS_time_ms - start) < ms) {
        __WFI(); // šetrí CPU
    }
}
