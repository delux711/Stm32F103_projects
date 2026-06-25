#include "app_config.h"
#include "debug.h"
#include "ir_tx.h"
#include "gpio.h"

/*
 * IR_TX_DBG - debug output macro.
 * Default: calls DEBUG_writeString().
 * To disable: add  #define IR_TX_DBG(msg)  to app_config.h.
 * Linker discards all related string literals when the macro is empty.
 */
#ifndef IR_TX_DBG
#  define IR_TX_DBG(msg)  DEBUG_writeString(msg)
#endif

/* ---------------------------------------------------------------- NEC timing (µs) --
 *
 *  Carrier:      38 kHz, 1/3 duty cycle
 *
 *  Leading mark: 9000 µs  (342 carrier bursts)
 *  Leading space:4500 µs
 *  Bit mark:      562 µs  (~21 bursts) – same for 0 and 1
 *  Bit-0 space:   562 µs
 *  Bit-1 space:  1687 µs
 *  Stop mark:     562 µs
 *
 *  Repeat (key held, ~110 ms period):
 *    9000 µs mark + 2250 µs space + 562 µs mark
 * ------------------------------------------------------------------------------ */

#define NEC_HDR_MARK_US     9000u
#define NEC_HDR_SPACE_US    4500u
#define NEC_BIT_MARK_US      562u
#define NEC_ONE_SPACE_US    1687u
#define NEC_ZERO_SPACE_US    562u
#define NEC_RPT_SPACE_US    2250u

/* --------------------------------------------------------- module state ----- */

static IR_TX_config_t        ir_cfg;
static uint32_t              ir_arr;        /* timer ARR → 38 kHz period – 1 */
static uint32_t              ir_ccr_on;     /* CCR for mark: 1/3 duty        */
static volatile uint32_t    *ir_ccr_reg;    /* pointer to active CCR register */

/* --------------------------------------------------------- DWT delay -------- */

static void IR_TX_dwtInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT  = 0u;
    DWT->CTRL   |= DWT_CTRL_CYCCNTENA_Msk;
}

static void IR_TX_delayUs(uint32_t us)
{
    uint32_t cycles = (ir_cfg.cpu_freq_hz / 1000000u) * us;
    uint32_t start  = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles)
    {
        /* busy-wait */
    }
}

/* --------------------------------------------------------- carrier helpers -- */

/*
 * Mark  = carrier ON:  set CCR to 1/3 duty → PWM mode 1 outputs carrier
 * Space = carrier OFF: set CCR to 0        → PWM mode 1 output stays LOW
 *         (in PWM mode 1: OC high while CNT < CCR; CCR=0 → always low)
 */
static inline void IR_TX_carrierOn(void)
{
    *ir_ccr_reg = ir_ccr_on;
}

static inline void IR_TX_carrierOff(void)
{
    *ir_ccr_reg = 0u;
}

static void IR_TX_mark(uint32_t us)
{
    IR_TX_carrierOn();
    IR_TX_delayUs(us);
}

static void IR_TX_space(uint32_t us)
{
    IR_TX_carrierOff();
    IR_TX_delayUs(us);
}

/* --------------------------------------------------------- timer channel init */

static void IR_TX_timerChannelInit(TIM_TypeDef *tim, uint8_t channel)
{
    switch (channel)
    {
        case 1u:
            /* PWM mode 1, no output-compare preload */
            tim->CCMR1 = (tim->CCMR1 & ~(TIM_CCMR1_OC1M | TIM_CCMR1_CC1S))
                       | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
            tim->CCER  = (tim->CCER  & ~TIM_CCER_CC1P) | TIM_CCER_CC1E;
            ir_ccr_reg = &tim->CCR1;
            break;

        case 2u:
            tim->CCMR1 = (tim->CCMR1 & ~(TIM_CCMR1_OC2M | TIM_CCMR1_CC2S))
                       | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
            tim->CCER  = (tim->CCER  & ~TIM_CCER_CC2P) | TIM_CCER_CC2E;
            ir_ccr_reg = &tim->CCR2;
            break;

        case 3u:
            tim->CCMR2 = (tim->CCMR2 & ~(TIM_CCMR2_OC3M | TIM_CCMR2_CC3S))
                       | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1;
            tim->CCER  = (tim->CCER  & ~TIM_CCER_CC3P) | TIM_CCER_CC3E;
            ir_ccr_reg = &tim->CCR3;
            break;

        default: /* channel 4 */
            tim->CCMR2 = (tim->CCMR2 & ~(TIM_CCMR2_OC4M | TIM_CCMR2_CC4S))
                       | TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1;
            tim->CCER  = (tim->CCER  & ~TIM_CCER_CC4P) | TIM_CCER_CC4E;
            ir_ccr_reg = &tim->CCR4;
            break;
    }
}

/* --------------------------------------------------------- public API ------- */

void IR_TX_init(const IR_TX_config_t *config)
{
    ir_cfg = *config;

    IR_TX_dwtInit();

    /* --- GPIO --- */
    GPIO_enableClock(config->port);
    GPIO_configPin(config->port, config->pin, GPIO_CFG_OUTPUT_AF_PP_50MHZ);

    /* --- AFIO remap (optional) --- */
    if (config->afio_remap_mask != 0u)
    {
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
        AFIO->MAPR = (AFIO->MAPR & ~config->afio_remap_mask)
                   | (config->afio_remap_val & config->afio_remap_mask);
    }

    /* --- Timer clock --- */
    *config->timer_rcc_reg |= config->timer_rcc_bit;

    TIM_TypeDef *tim = config->timer;

    /* --- 38 kHz carrier: PSC=0, ARR = f_timer/38000 - 1 --- */
    ir_arr    = (config->cpu_freq_hz / 38000u) - 1u;
    ir_ccr_on = ir_arr / 3u;   /* ~33 % duty cycle */

    tim->CR1 = 0u;             /* disable while configuring */
    tim->PSC = 0u;
    tim->ARR = ir_arr;

    IR_TX_timerChannelInit(tim, config->channel);

    /* Carrier OFF at startup */
    *ir_ccr_reg = 0u;

    /* Force register update, then enable */
    tim->EGR = TIM_EGR_UG;
    tim->CR1 = TIM_CR1_CEN;
    IR_TX_DBG("IR TX init\r\n");
}

void IR_TX_sendNEC(uint8_t address, uint8_t command)
{
    /*
     * 32-bit frame layout (transmitted LSB first):
     *   bits  0– 7 : address
     *   bits  8–15 : ~address  (logical inverse)
     *   bits 16–23 : command
     *   bits 24–31 : ~command
     */
    uint32_t frame = ((uint32_t)address)
                   | ((uint32_t)(uint8_t)(~address) << 8u)
                   | ((uint32_t)command              << 16u)
                   | ((uint32_t)(uint8_t)(~command)  << 24u);

    /* Leading burst */
    IR_TX_mark(NEC_HDR_MARK_US);
    IR_TX_space(NEC_HDR_SPACE_US);

    /* 32 data bits, LSB first */
    for (uint8_t i = 0u; i < 32u; i++)
    {
        IR_TX_mark(NEC_BIT_MARK_US);

        if (((frame >> i) & 0x01u) != 0u)
        {
            IR_TX_space(NEC_ONE_SPACE_US);
        }
        else
        {
            IR_TX_space(NEC_ZERO_SPACE_US);
        }
    }

    /* Stop bit mark + silence */
    IR_TX_mark(NEC_BIT_MARK_US);
    IR_TX_carrierOff();
    IR_TX_DBG("IR TX NEC sent\r\n");
}

void IR_TX_sendNECRepeat(void)
{
    IR_TX_DBG("IR TX NEC repeat\r\n");
    IR_TX_mark(NEC_HDR_MARK_US);
    IR_TX_space(NEC_RPT_SPACE_US);
    IR_TX_mark(NEC_BIT_MARK_US);
    IR_TX_carrierOff();
}
