#include "app_config.h"
#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "systick.h"
#include "ir_tx.h"
#include "debug.h"

/* -----------------------------------------------------------------------
 * IR Transmitter test application – NEC protocol
 *
 * Hardware:
 *   PA6 → 33 Ω → IR LED anode
 *   IR LED cathode → GND
 *   (TIM3 CH1 drives the LED directly via push-pull AF output)
 *
 *   For better range add a transistor driver:
 *   PA6 → 1 kΩ → NPN base (BC337), collector → IR LED → VCC 3.3 V
 *
 * What the app does (repeating sequence, 3 s between commands):
 *   1.  LED strip – POWER ON/OFF
 *   2.  LED strip – BRIGHTNESS +
 *   3.  LED strip – BRIGHTNESS +
 *   4.  LED strip – WHITE
 *   5.  LED strip – RED
 *   6.  LED strip – GREEN
 *   7.  LED strip – BLUE
 *   8.  LED strip – SMOOTH  (colour fade)
 *   9.  LG TV     – POWER   (bonus)
 *   10. LG TV     – VOL UP  (bonus)
 *
 * Each command is sent once per iteration and logged via SEGGER RTT.
 * ----------------------------------------------------------------------- */

/* ---------------------------------------------------------------- config -- */

#define IR_PORT          GPIOA
#define IR_PIN           6u         /* PA6 = TIM3_CH1 (no AFIO remap)     */
#define IR_TIMER         TIM3
#define IR_CHANNEL       1u
#define IR_BETWEEN_MS    2000u      /* pause between commands (ms)         */

static const IR_TX_config_t ir_config = {
    .port            = IR_PORT,
    .pin             = IR_PIN,
    .timer           = IR_TIMER,
    .channel         = IR_CHANNEL,
    .timer_rcc_reg   = (volatile uint32_t *)&RCC->APB1ENR,
    .timer_rcc_bit   = RCC_APB1ENR_TIM3EN,
    .afio_remap_mask = 0u,
    .afio_remap_val  = 0u,
};

/* ---------------------------------------------------------------- helpers -- */

typedef struct
{
    uint8_t     address;
    uint8_t     command;
    const char *label;
} IR_TX_step_t;

static const IR_TX_step_t app_sequence[] = {
    /* LED strip commands (address 0x00) */
    { IR_ADDR_LED_STRIP, IR_CMD_LED_POWER,      "LED POWER"    },
    { IR_ADDR_LED_STRIP, IR_CMD_LED_BRIGHT_UP,  "LED BRIGHT+"  },
    { IR_ADDR_LED_STRIP, IR_CMD_LED_BRIGHT_UP,  "LED BRIGHT+"  },
    { IR_ADDR_LED_STRIP, IR_CMD_LED_WHITE,       "LED WHITE"    },
    { IR_ADDR_LED_STRIP, IR_CMD_LED_RED,         "LED RED"      },
    { IR_ADDR_LED_STRIP, IR_CMD_LED_GREEN,       "LED GREEN"    },
    { IR_ADDR_LED_STRIP, IR_CMD_LED_BLUE,        "LED BLUE"     },
    { IR_ADDR_LED_STRIP, IR_CMD_LED_SMOOTH,      "LED SMOOTH"   },
    /* LG TV commands – bonus (address 0x04) */
    { IR_ADDR_LG_TV,     IR_CMD_LG_POWER,        "LG TV POWER"  },
    { IR_ADDR_LG_TV,     IR_CMD_LG_VOL_UP,       "LG TV VOL+"   },
};

#define APP_SEQUENCE_COUNT ((uint32_t)(sizeof(app_sequence) / sizeof(app_sequence[0])))

static void APP_printHex8(uint8_t val)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[5] = "0x";
    buf[2] = hex[(val >> 4u) & 0x0Fu];
    buf[3] = hex[val & 0x0Fu];
    buf[4] = '\0';
    DEBUG_writeString(buf);
}

static void APP_sendStep(const IR_TX_step_t *step)
{
    DEBUG_writeString("TX: ");
    DEBUG_writeString(step->label);
    DEBUG_writeString("  addr=");
    APP_printHex8(step->address);
    DEBUG_writeString(" cmd=");
    APP_printHex8(step->command);
    DEBUG_writeString("\r\n");

    DEBUG_ledPinOn();
    IR_TX_sendNEC(step->address, step->command);
    DEBUG_ledPinOff();
}

/* --------------------------------------------------------------- main ------ */

int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u);

    DEBUG_initTrace(SystemCoreClock);
    IR_TX_init(&ir_config);

    DEBUG_writeString("IR TX test start – NEC protocol\r\n");
    DEBUG_writeString("PA6 -> 33R -> IR LED -> GND\r\n");

    uint32_t step = 0u;

    while (1)
    {
        SYS_delayMs(IR_BETWEEN_MS);

        APP_sendStep(&app_sequence[step]);

        step++;
        if (step >= APP_SEQUENCE_COUNT)
        {
            step = 0u;
            DEBUG_writeString("--- sequence restart ---\r\n");
        }
    }
}
