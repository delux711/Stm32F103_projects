#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
    uint32_t magic;
    uint8_t  device_id;
    uint8_t  modbus_slave_addr;
    uint8_t  device_type;
    uint8_t  fw_version;
    uint32_t hw_serial;
} DeviceConfig_t;

#define DEVICE_CONFIG_MAGIC (0xDEADBEEFu)
#define DEVICE_CONFIG_OFFSET (0u)

void DeviceConfig_init(void);
void DeviceConfig_setDefaults(uint8_t device_id, uint8_t modbus_addr, uint8_t dev_type);
DeviceConfig_t *DeviceConfig_get(void);
int DeviceConfig_saveToEEPROM(void);
int DeviceConfig_loadFromEEPROM(void);

#ifdef __cplusplus
}
#endif

#endif
