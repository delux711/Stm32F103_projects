#include "debug.h"
#include "SEGGER_RTT.h"

// __STATIC_INLINE uint32_t ITM_SendChar (uint32_t ch)
// {
//   if ((ITM->TCR & ITM_TCR_ITMENA_Msk) &&
//       (ITM->TER & 1UL) )
//   {
//     while (ITM->PORT[0].u32 == 0);
//     ITM->PORT[0].u8 = (uint8_t)ch;
//   }
//   return (ch);
// }

static inline int DEBUG_debuggerActive(void)
{
    return (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0;
}

static void DEBUG_initSwo(void)
{
    /* Enable clocks */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;   // GPIOB clock
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;   // AFIO clock

    /* Disable JTAG, keep SWD
       SWJ_CFG[2:0] = 010 -> JTAG-DP Disabled and SW-DP Enabled */
    AFIO->MAPR &= ~(AFIO_MAPR_SWJ_CFG);
    AFIO->MAPR |=  (AFIO_MAPR_SWJ_CFG_JTAGDISABLE);

    /* PB3 = Alternate Function Push-Pull, 50 MHz */

    // Clear CNF3[1:0] and MODE3[1:0]
    GPIOB->CRL &= ~(GPIO_CRL_MODE3 | GPIO_CRL_CNF3);

    // MODE3 = 11 (50 MHz output)
    // CNF3  = 10 (Alternate function push-pull)
    GPIOB->CRL |=  (GPIO_CRL_MODE3) |
                   (GPIO_CRL_CNF3_1);
}

void DEBUG_initTrace(uint32_t cpu_freq_hz)
{
    // inicializace pinu pro LED (PC13) a SWO (PB3)
    uint32_t reg;
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    reg = GPIOC->CRH;
    reg &= ~((uint32_t)GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    reg |= GPIO_CRH_MODE13; // 11: Output mode, max speed 50 MHz
    DEBUG_ledPinOff();
    GPIOC->CRH = reg;

    if(DEBUG_debuggerActive())
    {
        DEBUG_initSwo();
        /* Enable trace in core debug */
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;

        /* ---------- DWT (cycle counter) ---------- */
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

        /* ---------- ITM ---------- */

        /* Unlock ITM (ak je locknutý) */
        ITM->LAR = 0xC5ACCE55;
        ITM->TPR = ITM_TPR_PRIVMASK_Msk;

        /* Disable ITM before configuration */
        ITM->TCR = 0;
        ITM->TER = 0;

        /* Enable channel 0 */
        ITM->TER |= (1 << 0) | (1 << 1); // povoliť kanál 0 a 1

        /* Enable ITM + SWO */
        ITM->TCR =
            ITM_TCR_ITMENA_Msk |
            ITM_TCR_SWOENA_Msk;

        /* --------- TPIU (nastavenie SWO rýchlosti) --------- */
        /* Predpoklad: chceš SWO napr. 2 MHz */

        uint32_t swo_freq = 2000000;

        TPIU->ACPR = (cpu_freq_hz / swo_freq) - 1;
        TPIU->SPPR = 2;          // NRZ/Async mode
        TPIU->FFCR = 0x100;      // disable formatter
    }
    SEGGER_RTT_Init();
}

void DEBUG_writeString(const char *s) {
    (void)SEGGER_RTT_WriteString(0u, s);
}

void DEBUG_writeChar(char c) {
    (void)SEGGER_RTT_Write(0u, &c, 1u);
}

void DEBUG_writeHex32(uint32_t val) {
    static const char hex[] = "0123456789ABCDEF";
    char buf[11] = "0x00000000";
    uint8_t i;

    for (i = 0u; i < 8u; i++)
    {
        buf[9u - i] = hex[(uint8_t)(val & 0x0Fu)];
        val >>= 4u;
    }

    DEBUG_writeString(buf);
}

void DEBUG_writeHex8(uint8_t val) {
    static const char hex[] = "0123456789ABCDEF";
    char buf[5] = "0x00";

    buf[2] = hex[(val >> 4u) & 0x0Fu];
    buf[3] = hex[val & 0x0Fu];

    DEBUG_writeString(buf);
}


// int DEBUG_printf(const char *fmt, ...) {
//     int r;
//     va_list args;
//     va_start(args, fmt);
//     r = SEGGER_RTT_vprintf(0u, fmt, &args);
//     va_end(args);
//     return r;
// }
