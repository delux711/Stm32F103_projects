#include "app_config.h"
#include "debug.h"
#include "ir_rx.h"
#include "gpio.h"

/*
 * IR_RX_DBG - debug output macro.
 * Default: calls DEBUG_writeString().
 * To disable: add  #define IR_RX_DBG(msg)  to app_config.h.
 * Linker discards all related string literals when the macro is empty.
 */
#ifndef IR_RX_DBG
#  define IR_RX_DBG(msg)  DEBUG_writeString(msg)
#endif

/* ------------------------------------------------------- timing windows (us) */

#define IR_RX_HEADER_MIN_US    12500u
#define IR_RX_HEADER_MAX_US    14500u

#define IR_RX_REPEAT_MIN_US    10800u
#define IR_RX_REPEAT_MAX_US    11800u

#define IR_RX_BIT0_MIN_US        900u
#define IR_RX_BIT0_MAX_US       1400u

#define IR_RX_BIT1_MIN_US       1900u
#define IR_RX_BIT1_MAX_US       2600u

/* ------------------------------------------------------------- module state */

typedef enum
{
    IR_RX_STATE_IDLE = 0u,
    IR_RX_STATE_RECEIVE
} IR_RX_state_t;

static IR_RX_config_t        ir_cfg;
static volatile uint32_t     ir_exti_mask;
static volatile uint8_t      ir_has_last_edge;
static volatile uint32_t     ir_last_edge_us;
static volatile IR_RX_state_t ir_state;
static volatile uint32_t     ir_raw;
static volatile uint8_t      ir_bit_index;

static volatile IR_RX_frame_t ir_frame;
static volatile uint8_t      ir_frame_ready;
static volatile uint8_t      ir_repeat_ready;

/* --------------------------------------------------------------- helpers --- */

static uint32_t IR_RX_getPortSource(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        return 0u;
    }
    if (port == GPIOB)
    {
        return 1u;
    }
    if (port == GPIOC)
    {
        return 2u;
    }
    if (port == GPIOD)
    {
        return 3u;
    }
    return 0u;
}

static void IR_RX_timerInit(const IR_RX_config_t *config)
{
    TIM_TypeDef *tim;
    uint32_t psc;

    *config->timer_rcc_reg |= config->timer_rcc_bit;

    tim = config->timer;
    tim->CR1 = 0u;

    psc = (SystemCoreClock / 1000000u);
    if (psc == 0u)
    {
        psc = 1u;
    }

    tim->PSC = psc - 1u;
    tim->ARR = 0xFFFFu;
    tim->CNT = 0u;
    tim->EGR = TIM_EGR_UG;
    tim->CR1 = TIM_CR1_CEN;
}

static void IR_RX_extiInit(const IR_RX_config_t *config)
{
    uint32_t port_source;
    uint32_t idx;
    uint32_t shift;

    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    port_source = IR_RX_getPortSource(config->port);
    idx = (uint32_t)config->exti_line / 4u;
    shift = ((uint32_t)config->exti_line % 4u) * 4u;

    AFIO->EXTICR[idx] = (AFIO->EXTICR[idx] & ~(0xFu << shift))
                      | (port_source << shift);

    EXTI->IMR  |= ir_exti_mask;
    EXTI->EMR  &= ~ir_exti_mask;
    EXTI->RTSR &= ~ir_exti_mask;
    EXTI->FTSR |= ir_exti_mask;
    EXTI->PR    = ir_exti_mask;

    if (config->exti_line <= 4u)
    {
        NVIC_EnableIRQ((IRQn_Type)(EXTI0_IRQn + config->exti_line));
    }
    else if (config->exti_line <= 9u)
    {
        NVIC_EnableIRQ(EXTI9_5_IRQn);
    }
    else
    {
        NVIC_EnableIRQ(EXTI15_10_IRQn);
    }
}

static void IR_RX_storeFrame(uint32_t raw)
{
    uint8_t addr;
    uint8_t addr_inv;
    uint8_t cmd;
    uint8_t cmd_inv;

    addr = (uint8_t)(raw & 0xFFu);
    addr_inv = (uint8_t)((raw >> 8u) & 0xFFu);
    cmd = (uint8_t)((raw >> 16u) & 0xFFu);
    cmd_inv = (uint8_t)((raw >> 24u) & 0xFFu);

    ir_frame.raw = raw;
    ir_frame.address = addr;
    ir_frame.address_inv = addr_inv;
    ir_frame.command = cmd;
    ir_frame.command_inv = cmd_inv;
    ir_frame.valid = (((uint8_t)(addr ^ addr_inv) == 0xFFu)
                   && ((uint8_t)(cmd ^ cmd_inv) == 0xFFu)) ? 1u : 0u;

    if (ir_frame.valid == 0u)
    {
        IR_RX_DBG("IR RX frame invalid (CRC)\r\n");
    }

    ir_frame_ready = 1u;
}

static void IR_RX_restartReceive(void)
{
    ir_state = IR_RX_STATE_RECEIVE;
    ir_raw = 0u;
    ir_bit_index = 0u;
}

/* ------------------------------------------------------------- public API -- */

void IR_RX_init(const IR_RX_config_t *config)
{
    ir_cfg = *config;
    ir_exti_mask = (1u << config->exti_line);

    GPIO_enableClock(config->port);
    GPIO_configPin(config->port, config->pin, GPIO_CFG_INPUT_PULL);
    config->port->BSRR = (1u << config->pin);

    IR_RX_timerInit(config);
    IR_RX_extiInit(config);
    IR_RX_reset();
    IR_RX_DBG("IR RX init\r\n");
}

void IR_RX_irqGlobalHandler(void)
{
    uint32_t pending;
    uint32_t now;
    uint32_t delta;
    uint8_t bit;

    pending = EXTI->PR & ir_exti_mask;
    if (pending == 0u)
    {
        return;
    }

    EXTI->PR = ir_exti_mask;

    now = ir_cfg.timer->CNT;
    if (ir_has_last_edge == 0u)
    {
        ir_last_edge_us = now;
        ir_has_last_edge = 1u;
        return;
    }

    delta = now - ir_last_edge_us;
    ir_last_edge_us = now;

    if (ir_state == IR_RX_STATE_IDLE)
    {
        if ((delta >= IR_RX_HEADER_MIN_US) && (delta <= IR_RX_HEADER_MAX_US))
        {
            IR_RX_restartReceive();
            return;
        }

        if ((delta >= IR_RX_REPEAT_MIN_US) && (delta <= IR_RX_REPEAT_MAX_US))
        {
            ir_repeat_ready = 1u;
            IR_RX_DBG("IR RX repeat\r\n");
            return;
        }

        return;
    }

    if ((delta >= IR_RX_BIT0_MIN_US) && (delta <= IR_RX_BIT0_MAX_US))
    {
        bit = 0u;
    }
    else if ((delta >= IR_RX_BIT1_MIN_US) && (delta <= IR_RX_BIT1_MAX_US))
    {
        bit = 1u;
    }
    else if ((delta >= IR_RX_HEADER_MIN_US) && (delta <= IR_RX_HEADER_MAX_US))
    {
        IR_RX_restartReceive();
        return;
    }
    else
    {
        ir_state = IR_RX_STATE_IDLE;
        IR_RX_DBG("IR RX timing err\r\n");
        return;
    }

    ir_raw |= ((uint32_t)bit << ir_bit_index);
    ir_bit_index++;

    if (ir_bit_index >= 32u)
    {
        IR_RX_storeFrame(ir_raw);
        ir_state = IR_RX_STATE_IDLE;
        IR_RX_DBG("IR RX frame ready\r\n");
    }
}

uint8_t IR_RX_readFrame(IR_RX_frame_t *out_frame)
{
    uint8_t primask;

    if ((out_frame == 0) || (ir_frame_ready == 0u))
    {
        return 0u;
    }

    primask = __get_PRIMASK();
    __disable_irq();

    *out_frame = (IR_RX_frame_t)ir_frame;
    ir_frame_ready = 0u;

    if (primask == 0u)
    {
        __enable_irq();
    }

    return 1u;
}

uint8_t IR_RX_readRepeat(void)
{
    uint8_t primask;
    uint8_t value;

    primask = __get_PRIMASK();
    __disable_irq();

    value = ir_repeat_ready;
    ir_repeat_ready = 0u;

    if (primask == 0u)
    {
        __enable_irq();
    }

    return value;
}

void IR_RX_reset(void)
{
    IR_RX_DBG("IR RX reset\r\n");
    ir_has_last_edge = 0u;
    ir_last_edge_us = 0u;
    ir_state = IR_RX_STATE_IDLE;
    ir_raw = 0u;
    ir_bit_index = 0u;
    ir_frame_ready = 0u;
    ir_repeat_ready = 0u;
}