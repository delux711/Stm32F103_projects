#ifndef RS485_COMMAND_H
#define RS485_COMMAND_H

#include <stdint.h>
#include <stdbool.h>

#define RS485_COMMAND_BUFFER_SIZE (64u)

typedef const char *(*RS485_commandCallback_t)(void);

typedef struct
{
    const char *command;
    RS485_commandCallback_t callback;
} RS485_command_t;

typedef struct
{
    uint8_t node_id;
    uint32_t response_delay_ms;
} RS485_commandConfig_t;

void RS485_commandInit(const RS485_commandConfig_t *config);
void RS485_commandSetTable(const RS485_command_t *command_table, uint32_t command_count);
void RS485_commandProcess(void);

#endif