#include <stdint.h>
#include "button.h"
#include "debug.h"

#define DEBOUNCE_MS        20
#define LONG_PRESS_MS     800
#define MULTICLICK_MS     400   // max pauza medzi klikmi

static uint8_t BUTTON_totalCount = 0u;
volatile uint32_t sys_ms;
static volatile btn_t *buttons;

static void initExti(void);
static void initGlobalVars(void);
static void irqGlobalHandler(void);

static inline uint8_t buttonRaw(const volatile btn_t *b)
{
    return (b->port->IDR & b->pin_mask) ? 1 : 0;
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
                if (b->on_long)
                    b->on_long();
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
                        EXTI->IMR |= (1u << b->exti_line);
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
                case 1: if (b->on_single) b->on_single(); break;
                case 2: if (b->on_double) b->on_double(); break;
                case 3: if (b->on_triple) b->on_triple(); break;
            }
            b->click_count = 0;

            /* návrat do EXTI režimu */
            b->active = 0;
            EXTI->IMR |= (1 << b->exti_line);
        }
    }
}

static void initExti(void)
{
    // Povoliť AFIO
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    for (uint32_t i = 0; i < BUTTON_totalCount; i++) {
        volatile btn_t *b = (volatile btn_t *)&buttons[i];

        // Mapovanie EXTI
        uint32_t line = b->exti_line;
        uint32_t port_source = 0;

        if (b->port == GPIOA) port_source = 0;
        else if (b->port == GPIOB) port_source = 1;
        else if (b->port == GPIOC) port_source = 2;
        else if (b->port == GPIOD) port_source = 3;

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
        if (b->exti_line <= 4u) {
            NVIC_EnableIRQ(EXTI0_IRQn + b->exti_line);
        } else if(b->exti_line <= 9u) {
            NVIC_EnableIRQ(EXTI9_5_IRQn);
        } else {
            NVIC_EnableIRQ(EXTI15_10_IRQn);
        }
    }
}

static void initGlobalVars(void) {
    sys_ms = 0u;
    BUTTON_totalCount = 0u;
}

static void irqGlobalHandler(void) {
    uint32_t pending = EXTI->PR & EXTI->IMR;

    for (uint32_t i = 0; i < BUTTON_totalCount; i++) {
        uint32_t mask = (1 << buttons[i].exti_line);
        if (pending & mask) {
            EXTI->PR = mask;                 // clear pending
            EXTI->IMR &= ~mask;              // vypnúť EXTI
            buttons[i].active = 1;           // prejsť do polling
        }
    }
}

void BUTTON_init(const volatile btn_t *button_configs, uint32_t count)
{
    initGlobalVars();
    BUTTON_totalCount = count;
    buttons = (volatile btn_t *)button_configs;

    for (uint32_t i = 0; i < BUTTON_totalCount; i++) {
        volatile btn_t *b = (volatile btn_t *)&buttons[i];

        // povoliť hodiny pre port
        if (b->port == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
        else if (b->port == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        else if (b->port == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
        else if (b->port == GPIOD) RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;

        // GPIO pin: vstup pull-up / pull-down
        uint32_t pin = __builtin_ctz(b->pin_mask);
        uint32_t value;
        volatile uint32_t *port;

        if (b->pin_mask < (1 << 8)) {
            port = &b->port->CRL;
        } else {
            pin -= 8;
            port = &b->port->CRH;
        }
        value = *port;
        value &= ~(0xF << (pin * 4));
        value |=  (0x8 << (pin * 4)); // CNF=10, MODE=00
        *port = value;

        // pull-up
        b->port->BSRR = b->pin_mask;   // nastav high
    }
    initExti();
}

void BUTTON_delayMs(uint32_t ms) {
    uint32_t start = sys_ms;
    while ((sys_ms - start) < ms) {
        // __WFI(); // šetrí CPU
    }
}

void EXTI0_IRQHandler(void) {
    irqGlobalHandler();
}
void EXTI1_IRQHandler(void) {
    irqGlobalHandler();
}
void EXTI2_IRQHandler(void) {
    irqGlobalHandler();
}
void EXTI3_IRQHandler(void) {
    irqGlobalHandler();
}
void EXTI4_IRQHandler(void) {
    irqGlobalHandler();
}
void EXTI9_5_IRQHandler(void) {
    irqGlobalHandler();
}
void EXTI15_10_IRQHandler(void)
{
    irqGlobalHandler();
}

void SysTick_Handler(void)
{
    sys_ms++;
    for (uint32_t i = 0; i < BUTTON_totalCount; i++) {
        if (buttons[i].active) {
            buttonProcess(&buttons[i]);
            buttonMulticlickProcess(&buttons[i]);
        }
    }
}
