#include <stddef.h>
#include "device_registry.h"
#include "debug.h"

static const RegistryEntry_t *registry_table = NULL;

void DeviceRegistry_init(const RegistryEntry_t *entries)
{
    registry_table = entries;
}

const RegistryEntry_t *DeviceRegistry_findByAddress(uint16_t addr)
{
    if (registry_table == NULL)
    {
        return NULL;
    }

    for (const RegistryEntry_t *entry = registry_table; entry->address != 0xFFFFu; entry++)
    {
        if (entry->address == addr)
        {
            return entry;
        }
    }

    return NULL;
}

uint16_t DeviceRegistry_read(uint16_t addr)
{
    const RegistryEntry_t *entry = DeviceRegistry_findByAddress(addr);

    if (entry == NULL)
    {
        return 0u;
    }

    if (entry->read == NULL)
    {
        return 0u;
    }

    return entry->read();
}

void DeviceRegistry_write(uint16_t addr, uint16_t value)
{
    const RegistryEntry_t *entry = DeviceRegistry_findByAddress(addr);

    if (entry == NULL)
    {
        return;
    }

    if (entry->type != REGISTRY_TYPE_RW)
    {
        return;
    }

    if (entry->write == NULL)
    {
        return;
    }

    entry->write(value);
}
