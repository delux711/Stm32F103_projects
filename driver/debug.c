#include "debug.h"

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

static inline int debuggerActive(void)
{
    return (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0;
}

static void initSWO(void)
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
    if(debuggerActive())
    {
        initSWO();
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
}
