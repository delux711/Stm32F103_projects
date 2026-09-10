#ifndef DEVICE_REGISTRY_H
#define DEVICE_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef uint16_t (*DeviceRegistry_readFunc_t)(void);
typedef void (*DeviceRegistry_writeFunc_t)(uint16_t value);

typedef enum
{
    REGISTRY_TYPE_RO = 0,
    REGISTRY_TYPE_RW
} RegistryType_t;

typedef struct
{
    uint16_t address;
    const char *name;
    RegistryType_t type;
    DeviceRegistry_readFunc_t read;
    DeviceRegistry_writeFunc_t write;
} RegistryEntry_t;

#define REGISTRY_END {0xFFFFu, NULL, 0, NULL, NULL}

void DeviceRegistry_init(const RegistryEntry_t *entries);
const RegistryEntry_t *DeviceRegistry_findByAddress(uint16_t addr);
uint16_t DeviceRegistry_read(uint16_t addr);
void DeviceRegistry_write(uint16_t addr, uint16_t value);

#ifdef __cplusplus
}
#endif

#endif
