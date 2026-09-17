#include "app_config.h"
#include "device_config.h"
#include "eeprom.h"
#include "debug.h"
#include "debug_rtt_levels.h"

static DeviceConfig_t device_cfg;
static int cfg_loaded = 0;

void DeviceConfig_init(void)
{
    EEPROM_init();
    DeviceConfig_setDefaults(0u, 1u, 0u);
}

void DeviceConfig_setDefaults(uint8_t device_id, uint8_t modbus_addr, uint8_t dev_type)
{
    device_cfg.magic = DEVICE_CONFIG_MAGIC;
    device_cfg.device_id = device_id;
    device_cfg.modbus_slave_addr = modbus_addr;
    device_cfg.device_type = dev_type;
    device_cfg.fw_version = 1u;
    device_cfg.hw_serial = 0x12345678u;
    cfg_loaded = 1;
}

DeviceConfig_t *DeviceConfig_get(void)
{
    if (cfg_loaded == 0)
    {
        DeviceConfig_loadFromEEPROM();
    }
    return &device_cfg;
}

int DeviceConfig_loadFromEEPROM(void)
{
    EEPROM_status_t st;

    st = EEPROM_read(DEVICE_CONFIG_OFFSET, (uint8_t *)&device_cfg, (uint16_t)sizeof(DeviceConfig_t));

    if ((st != EEPROM_OK) || (device_cfg.magic != DEVICE_CONFIG_MAGIC))
    {
        DeviceConfig_setDefaults(0u, 1u, 0u);
        rtt_error("DeviceConfig load failed, using defaults\r\n");
        return -1;
    }

    cfg_loaded = 1;
    return 0;
}

int DeviceConfig_saveToEEPROM(void)
{
    EEPROM_status_t st;

    device_cfg.magic = DEVICE_CONFIG_MAGIC;
    st = EEPROM_write(DEVICE_CONFIG_OFFSET, (uint8_t *)&device_cfg, (uint16_t)sizeof(DeviceConfig_t));

    return (st == EEPROM_OK) ? 0 : -1;
}
