#include "rs485_command.h"

#include <string.h>
#include "systick.h"
#include "debug.h"
#include "rs485.h"

#define FLASH_ID_ADDRESS        (0x0800FC00u)
#define DEFAULT_NODE_ID         ('1')
#define DEFAULT_RESPONSE_DELAY  (3u)

enum RS485_commandIrqState_t {
    RS485_COMMAND_IRQ_WAIT_FOR_ADDRESS = 0,
    RS485_COMMAND_IRQ_LENGTH,
    RS485_COMMAND_IRQ_DATA,
    RS485_COMMAND_IRQ_CRC
};

static uint8_t command_buffer[RS485_COMMAND_BUFFER_SIZE + 1u];
static uint8_t command_length = 0u;
static volatile uint32_t command_due_tick = 0u;
static volatile bool command_pending = false;
static const RS485_command_t *rs485_command_table = 0;
static uint32_t rs485_command_count = 0u;
static uint8_t command_node_id = DEFAULT_NODE_ID;
static uint32_t command_response_delay_ms = DEFAULT_RESPONSE_DELAY;
static volatile enum RS485_commandIrqState_t command_irq_state = RS485_COMMAND_IRQ_WAIT_FOR_ADDRESS;
static volatile uint8_t command_rx_index = 0u;
static volatile uint8_t command_rx_length = 0u;

static uint8_t RS485_commandReadNodeId(void);
static void RS485_commandSendString(const char *s);
static void RS485_commandOnRxByte(uint8_t data);
static void RS485_commandLogHex(uint32_t value);
static void RS485_commandLogDec(uint32_t value);

static void RS485_commandLog(const char *msg)
{
    DEBUG_writeString(msg);
}

static void RS485_commandLogHex(uint32_t value)
{
    char buffer[11u];
    uint32_t i;

    buffer[0] = '0';
    buffer[1] = 'x';
    for (i = 0u; i < 8u; i++)
    {
        uint8_t nibble = (uint8_t)((value >> ((7u - i) * 4u)) & 0x0Fu);
        uint8_t digit;
        if (nibble < 10u)
        {
            digit = (uint8_t)('0' + nibble);
        }
        else
        {
            digit = (uint8_t)('A' + (nibble - 10u));
        }
        buffer[2u + i] = (char)digit;
    }
    buffer[10u] = '\0';
    RS485_commandLog(buffer);
}

static void RS485_commandLogDec(uint32_t value)
{
    char buffer[12u];
    char tmp[10u];
    uint32_t i = 0u;
    uint32_t j;

    if (value == 0u)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        RS485_commandLog(buffer);
        return;
    }

    while ((value > 0u) && (i < (uint32_t)sizeof(tmp)))
    {
        tmp[i++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    for (j = 0u; j < i; j++)
    {
        buffer[j] = tmp[i - 1u - j];
    }
    buffer[i] = '\0';

    RS485_commandLog(buffer);
}

static uint8_t RS485_commandReadNodeId(void)
{
    uint8_t id = *(volatile uint8_t *)FLASH_ID_ADDRESS;

    if ((id == 0xFFu) || (id == 0x00u))
    {
        return DEFAULT_NODE_ID;
    }

    return id;
}

static void RS485_commandSendString(const char *s)
{
    if (s == 0)
    {
        return;
    }

    RS485_send((const uint8_t *)s, (uint16_t)strlen(s));
}

void RS485_commandInit(const RS485_commandConfig_t *config)
{
    command_node_id = RS485_commandReadNodeId();
    command_response_delay_ms = DEFAULT_RESPONSE_DELAY;
    if (config != 0)
    {
        if (config->node_id != 0u)
        {
            command_node_id = config->node_id;
        }
        command_response_delay_ms = config->response_delay_ms;
    }

    command_length = 0u;
    command_due_tick = 0u;
    command_pending = false;
    rs485_command_table = 0;
    rs485_command_count = 0u;
    command_irq_state = RS485_COMMAND_IRQ_WAIT_FOR_ADDRESS;
    command_rx_index = 0u;
    command_rx_length = 0u;
    command_buffer[0u] = 0u;

    RS485_setRxCallback(RS485_commandOnRxByte);
}

void RS485_commandSetTable(const RS485_command_t *command_table, uint32_t command_count)
{
    rs485_command_table = command_table;
    rs485_command_count = command_count;

    if ((command_table == 0) || (command_count == 0u))
    {
        RS485_commandLog("Command table empty\r\n");
    }
    else
    {
        RS485_commandLog("Command table set\r\n");
    }
}

static void RS485_commandOnRxByte(uint8_t data)
{
    if (command_pending)
    {
        RS485_commandLog("IRQ: command pending, ignore data\r\n");
        return;
    }

    switch (command_irq_state)
    {
        case RS485_COMMAND_IRQ_WAIT_FOR_ADDRESS:
        {
            if (data == command_node_id)
            {
                command_irq_state = RS485_COMMAND_IRQ_LENGTH;
                RS485_commandLog("IRQ: address matched: ");
                RS485_commandLogHex(data);
                RS485_commandLog("\r\n");
            }
            else {
                RS485_commandLog("IRQ: address mismatch: ");
                RS485_commandLogHex(data);
                RS485_commandLog("\r\n");
                RS485_goToMuteMode();
            }
            break;
        }
        case RS485_COMMAND_IRQ_LENGTH:
        {
            command_rx_length = (uint8_t)(data - '0');
            command_rx_index = 0u;
            command_irq_state = RS485_COMMAND_IRQ_DATA;
            RS485_commandLog("IRQ: length received: ");
            RS485_commandLogDec(command_rx_length);
            RS485_commandLog("\r\n");
            break;
        }
        case RS485_COMMAND_IRQ_DATA:
        {
            if (command_rx_index < RS485_COMMAND_BUFFER_SIZE)
            {
                command_buffer[command_rx_index++] = data;
            }
            else
            {
                command_irq_state = RS485_COMMAND_IRQ_WAIT_FOR_ADDRESS;
                RS485_commandLog("IRQ: data overflow\r\n");
                return;
            }

            if (command_rx_length > 0u)
            {
                command_rx_length--;
            }

            if (command_rx_length == 0u)
            {
                command_irq_state = RS485_COMMAND_IRQ_CRC;
            }
            break;
        }
        case RS485_COMMAND_IRQ_CRC:
        {
            command_buffer[command_rx_index] = 0u;
            command_length = command_rx_index;
            command_due_tick = SYS_getMs() + command_response_delay_ms;
            command_pending = true;
            command_irq_state = RS485_COMMAND_IRQ_WAIT_FOR_ADDRESS;
            command_rx_index = 0u;
            break;
        }
        default:
        {
            command_irq_state = RS485_COMMAND_IRQ_WAIT_FOR_ADDRESS;
            break;
        }
    }
}

void RS485_commandProcess(void)
{
    if ((!command_pending) || ((int32_t)(SYS_getMs() - command_due_tick) < 0))
    {
        return;
    }

    command_pending = false;

    if (command_length == 0u)
    {
        return;
    }

    RS485_commandLog("Process command\r\n");

    if ((rs485_command_table == 0) || (rs485_command_count == 0u))
    {
        RS485_commandLog("No command table\r\n");
        RS485_commandSendString("ERR\r\n");
        return;
    }

    for (uint32_t i = 0u; i < rs485_command_count; i++)
    {
        const RS485_command_t *entry = &rs485_command_table[i];

        if ((entry->command == 0) || (entry->callback == 0))
        {
            continue;
        }

        if (strcmp((const char *)command_buffer, entry->command) == 0)
        {
            const char *response = entry->callback();

            if (response != 0)
            {
                RS485_commandSendString(response);
            }
            else
            {
                RS485_commandSendString("ERR\r\n");
            }
            return;
        }
    }

    RS485_commandSendString("ERR\r\n");
}