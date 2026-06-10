#include "rs485.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h> // for snprintf

#include "stm32f10x.h"
#include "SEGGER_RTT.h"
#include "systick.h"
#include "debug.h"
#include "gpio.h"

// #define RS485_PARITY_ENABLE 1
// #define RS485_LOG_IRQ_COUNTER
#define RS485_COMMAND_LINE_ENABLE

#define FLASH_ID_ADDRESS  (0x0800FC00u)
#define DEFAULT_NODE_ID   '1'//(0xF1u)
#define DELAY_RESPONSE_MS (3u)
#define RX_BUFFER_SIZE    (64u)

enum RS485_IRQ_t {
    RS485_IRQ_INIT = 0,
    RS485_IRQ_WAIT_FOR_ADDRESS,
    RS485_IRQ_LENGTH,
    RS485_IRQ_DATA,
    RS485_IRQ_CRC,
    RS485_IRQ_IDLE,
    RS485_IRQ_MUTE
};

static uint8_t RS485_readNodeId(void);
static void RS485_initGlobalVariables(void);
static void RS485_gpioInit(const RS485_config_t *config);
static void RS485_txEnable(void);
static void RS485_txDisable(void);
static uint32_t RS485_getUsartClockHz(void);
static void RS485_usartInit(void);
static bool RS485_isMuteMode(void);
static void RS485_goToMuteMode(void);
static void RS485_goToActiveMode(void);
static void RS485_usartSendChar(char c);
static void RS485_usartSendString(const char *s);
static void RS485_processCommand(void);
static void RS485_logString(const char *msg);
static void RS485_logChar(char c);

static RS485_config_t rs485_config;
static USART_TypeDef *usart;
static uint8_t node_id = DEFAULT_NODE_ID;
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_index = 0u;
static volatile uint32_t command_due_tick = 0u;
static volatile bool command_pending = false;
static const RS485_command_t *rs485_command_table = 0;
static uint32_t rs485_command_count = 0u;
static volatile enum RS485_IRQ_t RS485_irqState = RS485_IRQ_WAIT_FOR_ADDRESS;
#ifdef RS485_LOG_IRQ_COUNTER
static volatile uint32_t RS485_irqCounter = 0u;
#endif

static void RS485_logString(const char *msg)
{
    // DEBUG_sendString(msg, 0);
    (void)SEGGER_RTT_WriteString(0u, msg);
}

static void RS485_logChar(char c)
{
    // DEBUG_sendChar((uint8_t)c, 0u);
    (void)SEGGER_RTT_Write(0u, &c, 1u);
}

static void RS485_logHex(uint32_t value)
{
    char buffer[11u]; // "0x" + 8 hex digits + null terminator
    snprintf(buffer, sizeof(buffer), "0x%08X", value);
    RS485_logString(buffer);
}

static void RS485_logDec(uint32_t value)
{
    char buffer[12u]; // max 10 digits for 32-bit + null terminator
    snprintf(buffer, sizeof(buffer), "%u", value);
    RS485_logString(buffer);
}

void RS485_init(const RS485_config_t *config)
{
    rs485_config = *config;
    usart = config->usart;
    RS485_initGlobalVariables();
    RS485_gpioInit(&rs485_config);
    RS485_usartInit();
    RS485_logString("RS485 init done\r\n");
}

void RS485_setCommandTable(const RS485_command_t *command_table, uint32_t command_count)
{
    rs485_command_table = command_table;
    rs485_command_count = command_count;
    if ((command_table == 0) || (command_count == 0u))
    {
        RS485_logString("Command table empty\r\n");
    }
    else
    {
        RS485_logString("Command table set\r\n");
    }
}

void RS485_process(void)
{
    if (command_pending && ((int32_t)(SYS_getMs() - command_due_tick) >= 0))
    {
        RS485_processCommand();
        rx_index = 0u;
        command_pending = false;
    }
}

static void RS485_initGlobalVariables(void)
{
    node_id = RS485_readNodeId();
    rx_index = 0u;
    command_due_tick = 0u;
    command_pending = false;
    RS485_irqState = RS485_IRQ_WAIT_FOR_ADDRESS;
  #ifdef RS485_LOG_IRQ_COUNTER
    RS485_irqCounter = 0u;
  #endif
    RS485_logString("RS485 globals init\r\n");
}

static bool RS485_isMuteMode(void)
{
    return (usart->CR1 & USART_CR1_WAKE) != 0u;
}

static void RS485_goToMuteMode(void)
{
    RS485_logString("Entering mute mode\r\n");
    usart->CR1 |= USART_CR1_RWU; // go to mute
}

static void RS485_goToActiveMode(void)
{
    RS485_logString("Entering active mode\r\n");
//     usart->CR1 &= ~USART_CR1_UE;                                               // disable USART to change mode
//   #if RS485_PARITY_ENABLE
//     usart->CR1 = (usart->CR1 & ~(USART_CR1_WAKE | USART_CR1_RWU)) | USART_CR1_M;                 // back to active with 9-bit (parity) mode
//   #else
//     usart->CR1 = (usart->CR1 & ~(USART_CR1_WAKE | USART_CR1_RWU));                               // back to active
//   #endif
//     usart->CR1 |= USART_CR1_UE;
}

static uint8_t RS485_readNodeId(void)
{
    uint8_t id = *(volatile uint8_t *)FLASH_ID_ADDRESS;

    if ((id == 0xFFu) || (id == 0x00u))
    {
        return DEFAULT_NODE_ID;
    }

    return id;
}

static void RS485_gpioInit(const RS485_config_t *config)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    GPIO_enableClock(config->txPort);
    GPIO_enableClock(config->rxPort);
    GPIO_enableClock(config->dirPort);

    AFIO->MAPR = (AFIO->MAPR & ~config->usartRemapMask) | config->usartRemap;

    GPIO_configPin(config->txPort, config->txPin, GPIO_CFG_OUTPUT_AF_PP_2MHZ);
    GPIO_configPin(config->rxPort, config->rxPin, GPIO_CFG_INPUT_FLOATING);
    GPIO_configPin(config->dirPort, config->dirPin, GPIO_CFG_OUTPUT_PP_2MHZ);

    RS485_txDisable();
}

static void RS485_txEnable(void)
{
    rs485_config.dirPort->BSRR = (uint32_t)1u << rs485_config.dirPin;
}

static void RS485_txDisable(void)
{
    rs485_config.dirPort->BRR = (uint32_t)1u << rs485_config.dirPin;
}

static uint32_t RS485_getUsartClockHz(void)
{
    uint32_t ppre_bits;
    uint32_t div = 1u;

    /* urcenie, ci je USART na APB2 alebo APB1 */
    if (rs485_config.usartRccReg == &RCC->APB2ENR)
        ppre_bits = (RCC->CFGR >> 11) & 0x7u; /* PPRE2 */
    else
        ppre_bits = (RCC->CFGR >> 8) & 0x7u;  /* PPRE1 */

    /* 0xx: /1, 100:/2, 101:/4, 110:/8, 111:/16 */
    if (ppre_bits >= 4u)
    {
        div = 1u << (ppre_bits - 3u);
    }

    return SystemCoreClock / div;
}

static void RS485_usartInit(void)
{
    uint32_t usart_clk = RS485_getUsartClockHz();

    *rs485_config.usartRccReg |= rs485_config.usartRccBit;
    if (rs485_config.baudrate == 0u)
    {
        return; /* ochrana pred delenim nulou */
    }
    usart->CR1 =
      #if RS485_PARITY_ENABLE
        USART_CR1_PCE |  // Parity control enable, parity selection is EVEN by default
      #endif
      #ifndef RS485_COMMAND_LINE_ENABLE
        USART_CR1_IDLEIE |
      #endif
        USART_CR1_RE |
        USART_CR1_TE |
        // USART_CR1_WAKE | // Wake (0) on idle line
        USART_CR1_RXNEIE;
    /* BRR pre oversampling x16: BRR ~= fCK / baud, so zaokruhlenim */
    usart->BRR = (usart_clk + (rs485_config.baudrate / 2u)) / rs485_config.baudrate;
    usart->CR1 |= USART_CR1_UE;

    NVIC_EnableIRQ(rs485_config.usartIrqn);
}

static void RS485_usartSendChar(char c)
{
    while ((usart->SR & USART_SR_TXE) == 0u) { }
    usart->DR = (uint16_t)c;
}

static void RS485_usartSendString(const char *s)
{
    if (s == 0)
    {
        RS485_logString("TX null response\r\n");
        return;
    }

    RS485_logString("TX response start\r\n");
    RS485_txEnable();

    while (*s != '\0')
    {
        RS485_usartSendChar(*s);
        s++;
    }

    while ((usart->SR & USART_SR_TC) == 0u) { }
    RS485_txDisable();
    RS485_logString("TX response done\r\n");
}

static void RS485_processCommand(void)
{
    const char *command = (const char *)&rx_buffer[0u];

    RS485_logString("Process command\r\n");

    if ((rs485_command_table == 0) || (rs485_command_count == 0u))
    {
        RS485_logString("No command table\r\n");
        RS485_usartSendString("ERR\r\n");
        return;
    }

    for (uint32_t i = 0u; i < rs485_command_count; i++)
    {
        const RS485_command_t *entry = &rs485_command_table[i];

        if ((entry->command == 0) || (entry->callback == 0))
        {
            continue;
        }

        if (strcmp(command, entry->command) == 0)
        {
            RS485_logString("Command match\r\n");
            const char *response = entry->callback();

            if (response != 0)
            {
                RS485_logString("Command callback ok\r\n");
                RS485_usartSendString(response);
            }
            else
            {
                RS485_logString("Command callback null\r\n");
                RS485_usartSendString("ERR\r\n");
            }
            return;
        }
    }

    RS485_logString("--Unknown command--\r\n");
    RS485_usartSendString("ERR\r\n");
}

void RS485_usartIrqHandler(void)
{
    uint8_t data;
    static uint8_t length = 0u;

  #ifdef RS485_LOG_IRQ_COUNTER
    RS485_irqCounter++;
    RS485_logString("IRQ UART count: ");
    RS485_logDec(RS485_irqCounter);
    RS485_logString("\r\n");
  #endif
    if((usart->SR & USART_SR_RXNE) != 0u)
    {
        data = (uint8_t)(usart->DR & 0xFFu);
        RS485_logString("IRQ: RXNE -");
        RS485_logChar((char)data);
        RS485_logString("\r\n");
        if(command_pending)
        {
            RS485_logString("IRQ: command pending, ignore data\r\n");
            return;
        }
        switch(RS485_irqState)
        {
            case RS485_IRQ_WAIT_FOR_ADDRESS:
            {
                if(data == node_id)
                {
                    RS485_irqState = RS485_IRQ_LENGTH;
                    RS485_logString("IRQ: address matched: ");
                    RS485_logHex(data);
                    RS485_logString("\r\n");
                }
                else
                {
                    RS485_logString("IRQ: address mismatch - mute mode\r\n");
                    RS485_goToMuteMode();
                }
                break;
            }
            case RS485_IRQ_LENGTH:
            {
                RS485_irqState = RS485_IRQ_DATA;
                length = data - '0'; // convert ASCII digit to number
                rx_index = 0u;
                RS485_logString("IRQ: length received: ");
                RS485_logDec(length);
                RS485_logString("\r\n");
                break;
            }
            case RS485_IRQ_DATA:
            {
                RS485_logString("IRQ: data received\r\n");

                if(rx_index < RX_BUFFER_SIZE)
                {
                    rx_buffer[rx_index++] = data;
                }
                else
                {
                    RS485_logString("IRQ: data overflow\r\n");
                    RS485_irqState = RS485_IRQ_WAIT_FOR_ADDRESS; // reset state machine on overflow
                }
                length--;
                if(length == 0u)
                {
                    RS485_irqState = RS485_IRQ_CRC;
                    RS485_logString("IRQ: all data received, wait for CRC\r\n");
                }
                break;
            }
            case RS485_IRQ_CRC:
            {
                RS485_logString("IRQ: CRC received: ");
                RS485_logHex(data);
                RS485_logString("\r\n");
                RS485_irqState = RS485_IRQ_WAIT_FOR_ADDRESS; // for simplicity, we just go back to waiting for address after receiving CRC
                rx_buffer[rx_index] = 0u;
                command_due_tick = SYS_getMs() + DELAY_RESPONSE_MS;
                command_pending = true;
                break;
            }
            default:
            {
                RS485_irqState = RS485_IRQ_WAIT_FOR_ADDRESS;
                RS485_logString("IRQ: unexpected state\r\n");
                RS485_goToActiveMode();
                break;
            }
        }
    }

    if ((usart->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE | USART_SR_IDLE)) != 0u)
    {
        RS485_logString("USART error\r\n");
        (void)usart->SR; // clear error flags
        (void)usart->DR;
        RS485_goToMuteMode();
      #ifndef RS485_COMMAND_LINE_ENABLE
        rx_index = 0u;
        RS485_irqState = RS485_IRQ_WAIT_FOR_ADDRESS;
      #endif
        return;
    }
}
