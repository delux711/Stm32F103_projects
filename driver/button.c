#include <stdint.h>
#include "stm32f10x.h"
#include "button.h"
#include "debug.h"

#define DEBOUNCE_MS        20
#define LONG_PRESS_MS     800
#define MULTICLICK_MS     400   // max pauza medzi klikmi

typedef enum {
    BTN_IDLE = 0u,
    BTN_DEBOUNCE_PRESS,
    BTN_PRESSED,
    BTN_DEBOUNCE_RELEASE
} btnState_t;

typedef struct {
    BUTTON_config_t cfg;
    /* HW */
    // GPIO_TypeDef *port;
    // uint16_t      pin_mask; papu
    // uint8_t       exti_line;   // 0..15

    /* SW */
    btnState_t    state;
    uint32_t      timer;
    uint32_t      press_time;
    uint8_t       click_count;
    uint8_t       long_sent;
    uint8_t       active;      // 0 = EXTI mode, 1 = polling

    // /* callbacks */
    // btnCb_t       on_single;
    // btnCb_t       on_double;
    // btnCb_t       on_triple;
    // btnCb_t       on_long; papu
} btn_t;

static btn_t *buttons;

static inline uint8_t buttonRaw(const volatile btn_t *b)
{
    return (b->cfg.read()) ? 1 : 0;
}

static void buttonProcess(volatile btn_t *b)
{
    uint8_t raw = buttonRaw(b);

    switch (b->state) {
        case BTN_IDLE:
            if (raw == 0) {
                b->state = BTN_DEBOUNCE_PRESS;
                b->timer = DEBOUNCE_MS;
            }
            break;
        case BTN_DEBOUNCE_PRESS:
            if (raw == 0) {
                if (--b->timer == 0) {
                    b->state = BTN_PRESSED;
                    b->press_time = 0;
                    b->long_sent = 0;
                }
            } else {
                b->state = BTN_IDLE;
            }
            break;
        case BTN_PRESSED:
            b->press_time++;

            if (!b->long_sent && b->press_time >= LONG_PRESS_MS) {
                b->long_sent = 1;
                if (b->cfg.onLong)
                    b->cfg.onLong();
            }
            if (raw == 1) {
                b->state = BTN_DEBOUNCE_RELEASE;
                b->timer = DEBOUNCE_MS;
            }
            break;
        case BTN_DEBOUNCE_RELEASE:
            if (raw == 1) {
                if (--b->timer == 0) {
                    b->state = BTN_IDLE;

                    if (!b->long_sent) {
                        b->click_count++;
                        b->timer = MULTICLICK_MS;
                    } else {
                        // long press finished -> exit polling and re-enable EXTI immediately
                        b->active = 0;
                        EXTI->IMR |= (1u << b->cfg.exti_line); // papu
                    }
                }
            } else {
                b->state = BTN_PRESSED;
            }
            break;
    }
}

static void buttonMulticlickProcess(volatile btn_t *b)
{
    if (b->click_count > 0 && b->state == BTN_IDLE) {
        if (b->timer > 0 && --b->timer == 0) {
            switch (b->click_count) {
                case 1: if (b->cfg.onSingle) b->cfg.onSingle(); break;
                case 2: if (b->cfg.onDouble) b->cfg.onDouble(); break;
                case 3: if (b->cfg.onTriple) b->cfg.onTriple(); break;
            }
            b->click_count = 0;

            /* návrat do EXTI režimu */
            b->active = 0;
            EXTI->IMR |= (1 << b->cfg.exti_line); // papu
        }
    }
}

// static void initLedPin(void) {
//     uint32_t reg;
//     RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
//     reg = GPIOC->CRH;
//     reg &= ~((uint32_t)GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
//     reg |= GPIO_CRH_MODE13; // 11: Output mode, max speed 50 MHz
//     GPIOC->CRH = reg;

//     RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
//     reg = GPIOB->CRH;
//     reg &= ~((uint32_t)GPIO_CRH_CNF13 | GPIO_CRH_MODE13 | GPIO_CRH_CNF14 | GPIO_CRH_MODE14 | GPIO_CRH_CNF15 | GPIO_CRH_MODE15);
//     reg |= GPIO_CRH_MODE13 | GPIO_CRH_MODE14 | GPIO_CRH_MODE15; // 11: Output mode, max speed 50 MHz
//     GPIOB->CRH = reg;

//     RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
//     reg = GPIOA->CRH;
//     reg &= ~((uint32_t)GPIO_CRH_CNF8 | GPIO_CRH_MODE8);
//     reg |= GPIO_CRH_MODE8; // 11: Output mode, max speed 50 MHz
//     GPIOA->CRH = reg;
// }

static void initExti(void)
{
    // Povoliť AFIO
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    for (uint32_t i = 0; i < BUTTON_COUNT; i++) {
        volatile btn_t *b = (btn_t *)&buttons[i];

        // Mapovanie EXTI
        uint32_t line = b->cfg.exti_line;
        uint32_t port_source = 0;

        if (b->cfg.port == GPIOA) port_source = 0;
        else if (b->cfg.port == GPIOB) port_source = 1;
        else if (b->cfg.port == GPIOC) port_source = 2;
        else if (b->cfg.port == GPIOD) port_source = 3;

        // Clear previous mapping and set new one
        uint32_t idx = line / 4;
        uint32_t shift = (line % 4) * 4;
        AFIO->EXTICR[idx] = (AFIO->EXTICR[idx] & ~(0xF << shift)) | (port_source << shift);

        // Nastavenie EXTI
        EXTI->IMR  |= (1 << line);  // unmask
        EXTI->EMR  &= ~(1 << line); // len interrupt
        EXTI->RTSR |= (1 << line);  // zmena na nábežnú hranu
        EXTI->FTSR |= (1 << line);  // povoliť aj zostupnú hranu; obe hrany sú vždy aktívne, bez ohľadu na logiku tlačidla
        EXTI->PR    = (1 << line);  // clear pending

        // Povolenie NVIC pre EXTI 10-15 (ak máš viac tlačidiel)
        if (b->cfg.exti_line <= 4u) {
            NVIC_EnableIRQ(EXTI0_IRQn + b->cfg.exti_line);
        } else if(b->cfg.exti_line <= 9u) {
            NVIC_EnableIRQ(EXTI9_5_IRQn);
        } else {
            NVIC_EnableIRQ(EXTI15_10_IRQn);
        }
    }
}

static void initGlobalVars(void) {
    sys_ms = 0u;
    // for (uint32_t i = 0; i < BUTTON_COUNT; i++) {
    //     buttons[i] = buttonInit[i];
    // } papu
}
static void irqGlobalHandler(void) {
    uint32_t pending = EXTI->PR & EXTI->IMR;

    for (uint32_t i = 0; i < BUTTON_COUNT; i++) {
        uint32_t mask = (1 << buttons[i].exti_line);
        if (pending & mask) {
            EXTI->PR = mask;                 // clear pending
            EXTI->IMR &= ~mask;              // vypnúť EXTI
            buttons[i].active = 1;           // prejsť do polling
        }
    }
}

void BUTTON_init(BUTTON_config_t *cfg, uint32_t count, BUTTON_timeF timeFunc)
{
    initGlobalVars();

    for (uint32_t i = 0; i < BUTTON_COUNT; i++) {
        volatile btn_t *b = (btn_t *)&buttons[i];

        // povoliť hodiny pre port
        if (b->cfg.port == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
        else if (b->cfg.port == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        else if (b->cfg.port == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
        else if (b->cfg.port == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;

        // GPIO pin: vstup pull-up / pull-down
        uint32_t pin = __builtin_ctz(b->cfg.pin_mask);
        uint32_t value;
        volatile uint32_t *port;

        if (b->cfg.pin_mask < (1 << 8)) {
            port = &b->cfg.port->CRL;
        } else {
            pin -= 8;
            port = &b->cfg.port->CRH;
        }
        value = *port;
        value &= ~(0xF << (pin * 4));
        value |=  (0x8 << (pin * 4)); // CNF=10, MODE=00
        *port = value;

        // pull-up
        b->cfg.port->BSRR = b->cfg.pin_mask;   // nastav high
    }
    initExti();
}
