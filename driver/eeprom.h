#ifndef EEPROM_H
#define EEPROM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32f10x.h"

#define EEPROM_PAGE_SIZE      (2048u)
#define EEPROM_MAX_PAGES      (2u)
#define EEPROM_TOTAL_SIZE     (EEPROM_PAGE_SIZE * EEPROM_MAX_PAGES)
#define EEPROM_START_ADDR     (0x08000000u + 64u * 1024u - EEPROM_TOTAL_SIZE)

typedef enum
{
    EEPROM_OK = 0,
    EEPROM_ERR_LOCKED,
    EEPROM_ERR_INVALID_ADDR,
    EEPROM_ERR_WRITE_FAILED
} EEPROM_status_t;

void EEPROM_init(void);
EEPROM_status_t EEPROM_read(uint16_t offset, uint8_t *data, uint16_t len);
EEPROM_status_t EEPROM_write(uint16_t offset, const uint8_t *data, uint16_t len);
EEPROM_status_t EEPROM_erase(uint16_t offset, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
