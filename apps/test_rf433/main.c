#include "app_config.h"
#include <stdint.h>
#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "debug.h"
#include "rf433.h"
#include "systick.h"

/* RF433 konfiguracia - USART2 (PA2=TX, PA3=RX) */
static const RF433_config_t rf433_cfg = {
    .txPort         = GPIOA,
    .txPin          = 2u,
    .rxPort         = GPIOA,
    .rxPin          = 3u,
    .usartRemapMask = 0u,
    .usartRemap     = 0u,
    .usart          = USART2,
    .usartIrqn      = USART2_IRQn,
    .usartRccReg    = (volatile uint32_t *)&RCC->APB1ENR,
    .usartRccBit    = RCC_APB1ENR_USART2EN,
    .baudrate       = 9600u
};

#define RF433_RX_BUFFER_SIZE 256u
#define RF433_FRAME_MAX_SIZE 64u
#define RF433_FRAME_GAP_MS   20u

static uint8_t rf433_rx_buffer[RF433_RX_BUFFER_SIZE];
static volatile uint32_t rf433_rx_head = 0u;
static volatile uint32_t rf433_rx_tail = 0u;

static uint8_t rf433_frame[RF433_FRAME_MAX_SIZE];
static uint32_t rf433_frame_len = 0u;
static uint32_t rf433_last_rx_ms = 0u;

static void APP_rf433RxCallback(uint8_t data)
{
    uint32_t next = (rf433_rx_head + 1u) % RF433_RX_BUFFER_SIZE;
    if (next != rf433_rx_tail) {
        rf433_rx_buffer[rf433_rx_head] = data;
        rf433_rx_head = next;
        rf433_last_rx_ms = SYS_getMs();
    }
}

static uint16_t APP_calcCRC16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFFu;

    for (uint32_t i = 0u; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0u; j < 8u; j++) {
            crc = (crc & 1u) ? (uint16_t)((crc >> 1u) ^ 0xA001u) : (uint16_t)(crc >> 1u);
        }
    }

    return crc;
}

static void APP_sendTestFrame(void)
{
    uint8_t frame[8];
    frame[0] = 0x01u; /* slave address */
    frame[1] = 0x03u; /* function: read holding registers */
    frame[2] = 0x00u;
    frame[3] = 0x00u;
    frame[4] = 0x00u;
    frame[5] = 0x02u;

    uint16_t crc = APP_calcCRC16(frame, 6u);
    frame[6] = (uint8_t)(crc & 0xFFu);
    frame[7] = (uint8_t)((crc >> 8u) & 0xFFu);

    RF433_send(frame, 8u);
}

static void APP_processRf433Buffer(void)
{
    while (rf433_rx_tail != rf433_rx_head) {
        uint8_t data = rf433_rx_buffer[rf433_rx_tail];
        rf433_rx_tail = (rf433_rx_tail + 1u) % RF433_RX_BUFFER_SIZE;

        if (rf433_frame_len < RF433_FRAME_MAX_SIZE) {
            rf433_frame[rf433_frame_len++] = data;
        }
    }
}

static void APP_tryPublishFrame(uint32_t now_ms)
{
    if ((rf433_frame_len >= 4u) && ((now_ms - rf433_last_rx_ms) >= RF433_FRAME_GAP_MS)) {
        uint16_t crc_rx;
        uint16_t crc_calc;

        crc_rx = (uint16_t)rf433_frame[rf433_frame_len - 2u] |
                 (uint16_t)((uint16_t)rf433_frame[rf433_frame_len - 1u] << 8u);
        crc_calc = APP_calcCRC16(rf433_frame, rf433_frame_len - 2u);

        if (crc_rx == crc_calc) {
            DEBUG_writeString("RF433 RX frame OK\r\n");
        } else {
            DEBUG_writeString("RF433 RX frame CRC error\r\n");
        }

        rf433_frame_len = 0u;
    }
}

static void APP_hardwareInit(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    DEBUG_initTrace(SystemCoreClock);
    DEBUG_writeString("=== RF433 Test App ===\r\n");

    RF433_init(&rf433_cfg);
    RF433_setRxCallback(APP_rf433RxCallback);
}

int main(void)
{
    uint32_t last_send_time = 0u;

    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u);

    APP_hardwareInit();

    while (1) {
        uint32_t now = SYS_getMs();

        APP_processRf433Buffer();
        APP_tryPublishFrame(now);

        if ((now - last_send_time) >= 2000u) {
            DEBUG_writeString("RF433 TX test frame\r\n");
            APP_sendTestFrame();
            last_send_time = now;
        }
    }
}
