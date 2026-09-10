#include "app_config.h"
#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "systick.h"
#include "ir_rx.h"
#include "debug.h"

/* -----------------------------------------------------------------------
 * IR Receiver test application – NEC protocol
 *
 * Hardware (example):
 *   TSOP4838 VCC -> 3.3V
 *   TSOP4838 GND -> GND
 *   TSOP4838 OUT -> PA1 (EXTI1)
 *
 * What the app does:
 *   - waits for NEC frames from IR remote
 *   - prints decoded address/command via SEGGER RTT
 *   - reports repeat frames while key is held
 * ----------------------------------------------------------------------- */

#define IR_RX_PORT           GPIOA
#define IR_RX_PIN            1u
#define IR_RX_EXTI_LINE      1u
#define IR_RX_TIMER          TIM2

static const IR_RX_config_t ir_config = {
    .port           = IR_RX_PORT,
    .pin            = IR_RX_PIN,
    .exti_line      = IR_RX_EXTI_LINE,
    .timer          = IR_RX_TIMER,
    .timer_rcc_reg  = (volatile uint32_t *)&RCC->APB1ENR,
    .timer_rcc_bit  = RCC_APB1ENR_TIM2EN
};

static void APP_printHex8(uint8_t val)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[5] = "0x";

    buf[2] = hex[(val >> 4u) & 0x0Fu];
    buf[3] = hex[val & 0x0Fu];
    buf[4] = '\0';
    DEBUG_writeString(buf);
}

static void APP_printHex32(uint32_t val)
{
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

int main(void)
{
    IR_RX_frame_t frame;

    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u);

    DEBUG_initTrace(SystemCoreClock);
    IR_RX_init(&ir_config);

    DEBUG_writeString("IR RX test start - NEC, TSOP4838 on PA1\r\n");

    while (1)
    {
        if (IR_RX_readFrame(&frame) != 0u)
        {
            DEBUG_ledPinToggle();

            DEBUG_writeString("RX frame: raw=");
            APP_printHex32(frame.raw);
            DEBUG_writeString(" addr=");
            APP_printHex8(frame.address);
            DEBUG_writeString(" cmd=");
            APP_printHex8(frame.command);
            DEBUG_writeString(" valid=");
            DEBUG_writeString(frame.valid ? "1" : "0");
            DEBUG_writeString("\r\n");
        }

        if (IR_RX_readRepeat() != 0u)
        {
            DEBUG_writeString("RX repeat\r\n");
        }

        SYS_delayMs(5u);
    }
}