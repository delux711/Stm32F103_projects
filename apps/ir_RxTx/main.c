#include "app_config.h"
#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "systick.h"
#include "ir_tx.h"
#include "ir_rx.h"
#include "rs485.h"
#include "debug.h"
#include <stddef.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * IR Receiver test application – NEC protocol
 *
 * Hardware (example):
 *   TSOP4838 VCC -> 3.3V
 *   TSOP4838 GND -> GND
 *   TSOP4838 OUT -> PA6 (EXTI6)
 *
 * What the app does:
 *   - waits for NEC frames from IR remote
 *   - prints decoded address/command via SEGGER RTT
 *   - reports repeat frames while key is held
 * ----------------------------------------------------------------------- */

#define IR_RX_PORT           GPIOA
#define IR_RX_PIN            6u         /* PA6 = TIM3_CH1 (no AFIO remap)     */
#define IR_RX_EXTI_LINE      6u
#define IR_RX_TIMER          TIM3

#define IR_TX_PORT          GPIOA
#define IR_TX_PIN           1u         /* PA1 = TIM2_CH2 (no AFIO remap)     */
#define IR_TX_TIMER         TIM2
#define IR_TX_CHANNEL       2u
#define IR_TX_ETR_PORT      GPIOA
#define IR_TX_ETR_PIN       0u         /* PA0 = TIM2_ETR  (no AFIO remap)     */
#define IR_BETWEEN_MS    2000u         /* pause between commands (ms)         */


static const IR_RX_config_t ir_rx_config = {
    .port           = IR_RX_PORT,
    .pin            = IR_RX_PIN,
    .exti_line      = IR_RX_EXTI_LINE,
    .timer          = IR_RX_TIMER,
    .timer_rcc_reg  = (volatile uint32_t *)&RCC->APB1ENR,
    .timer_rcc_bit  = RCC_APB1ENR_TIM3EN
};
static const IR_TX_config_t ir_tx_config = {
    .port            = IR_TX_PORT,
    .pin             = IR_TX_PIN,
    .etrPort         = IR_TX_ETR_PORT,
    .etrPin          = IR_TX_ETR_PIN,
    .timer           = IR_TX_TIMER,
    .channel         = IR_TX_CHANNEL,
    .timer_rcc_reg   = (volatile uint32_t *)&RCC->APB1ENR,
    .timer_rcc_bit   = RCC_APB1ENR_TIM2EN,
    .afio_remap_mask = 0u,
    .afio_remap_val  = 0u
};
static const RS485_config_t rs485_cfg = {
    .uart          = {
        .txPort        = GPIOB,
        .txPin         = 6u,
        .rxPort        = GPIOB,
        .rxPin         = 7u,
        .usartRemapMask = AFIO_MAPR_USART1_REMAP,
        .usartRemap     = AFIO_MAPR_USART1_REMAP,
        .usart          = USART1,
        .usartIrqn      = USART1_IRQn,
        .usartRccReg    = &RCC->APB2ENR,
        .usartRccBit    = RCC_APB2ENR_USART1EN,
        .baudrate       = 9600u,
        .dataBits       = UART_DATA_BITS_8,
        .parity         = UART_PARITY_EVEN
    },
    .dirPort       = NULL,
    .dirPin        = 0u
};


/* -----------------------------------------------------------------------
 * IR Transmitter test application – NEC protocol
 *
 * Hardware:
 *   PA1 → 33 Ω → IR LED anode
 *   IR LED cathode → GND
 *   (TIM2 CH2 drives the LED directly via push-pull AF output)
 *
 *   For better range add a transistor driver:
 *   PA1 → 1 kΩ → NPN base (BC337), collector → IR LED → VCC 3.3 V
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

void sendIr(const uint8_t *data, uint16_t length) {
    ir_tx_config.timer->CCER |= TIM_CCER_CC1E << ((IR_TX_CHANNEL - 1u) * 4); // enable output for channel 2
    DEBUG_writeString("TX: ");
    DEBUG_writeString((const char *)data);
    // APP_printHex8(*data);
    DEBUG_writeString("\r\n");
    RS485_send(data, length);
    ir_tx_config.timer->CCER &= ~(TIM_CCER_CC1E << ((IR_TX_CHANNEL - 1u) * 4)); // disable output for channel 2
}

void rxCallback(uint8_t data) {
    DEBUG_writeString("RS485 received: ");
    APP_printHex8(data);
    DEBUG_writeString("\r\n");
}

int main(void)
{
    IR_RX_frame_t frame;

    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u);

    DEBUG_initTrace(SystemCoreClock);
    IR_RX_init(&ir_rx_config);
    IR_TX_init(&ir_tx_config);
    RS485_init(&rs485_cfg);
    RS485_setRxCallback(rxCallback);

    DEBUG_writeString("IR RX TX test start - NEC, TSOP4838 on PA6\r\n");
    DEBUG_writeString("PA1 -> 33R -> IR LED -> GND\r\n");

    uint32_t step = 0u;

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

        static uint32_t last_send = 0u;
        static uint32_t last_rs485 = 0u;

        // if ((SYS_getMs() - last_send) >= 1000u)
        // {
        //     // APP_sendStep(&app_sequence[step]);
        //     step = (step + 1u) % APP_SEQUENCE_COUNT;
        //     last_send = SYS_getMs();
        // }

        if ((SYS_getMs() - last_rs485) >= 2000u)
        {
            // IR_TX_PWMOn();
            DEBUG_ledPinOn();
            // DEBUG_writeString("RS485 test: sending 'Hello RS485' via USART1\r\n");
            // const char *msg = "/?!\r\n";
            const char *msg = "?\r\n";
            sendIr((const uint8_t *)msg, (uint16_t)strlen(msg));
            last_rs485 = SYS_getMs();
            DEBUG_ledPinOff();
        }
    }
}