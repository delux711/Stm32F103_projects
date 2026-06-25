#include "app_config.h"
#include "eeprom.h"
#include "debug.h"

#ifndef EEPROM_DBG
#  define EEPROM_DBG(msg) DEBUG_writeString(msg)
#endif

#define FLASH_KEY1 (0x45670123u)
#define FLASH_KEY2 (0xCDEF89ABu)

static void EEPROM_unlock(void)
{
    if ((FLASH->CR & FLASH_CR_LOCK) != 0u)
    {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void EEPROM_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

void EEPROM_init(void)
{
    EEPROM_DBG("EEPROM init\r\n");
}

EEPROM_status_t EEPROM_read(uint16_t offset, uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint32_t addr;

    if ((data == 0) || (len == 0u) || ((offset + len) > EEPROM_TOTAL_SIZE))
    {
        return EEPROM_ERR_INVALID_ADDR;
    }

    addr = EEPROM_START_ADDR + offset;

    for (i = 0u; i < len; i++)
    {
        data[i] = *(uint8_t *)addr;
        addr++;
    }

    return EEPROM_OK;
}

EEPROM_status_t EEPROM_write(uint16_t offset, const uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint32_t addr;

    if ((data == 0) || (len == 0u) || ((offset + len) > EEPROM_TOTAL_SIZE))
    {
        return EEPROM_ERR_INVALID_ADDR;
    }

    addr = EEPROM_START_ADDR + offset;

    EEPROM_unlock();

    for (i = 0u; i < len; i++)
    {
        while ((FLASH->SR & FLASH_SR_BSY) != 0u) { }

        if ((FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) != 0u)
        {
            FLASH->SR = FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
        }

        FLASH->CR |= FLASH_CR_PG;
        *(volatile uint16_t *)addr = (uint16_t)data[i] | ((uint16_t)data[i + 1u] << 8u);
        addr += 2u;
        i++;

        while ((FLASH->SR & FLASH_SR_BSY) != 0u) { }

        if ((FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) != 0u)
        {
            FLASH->CR &= ~FLASH_CR_PG;
            EEPROM_lock();
            return EEPROM_ERR_WRITE_FAILED;
        }

        FLASH->CR &= ~FLASH_CR_PG;
    }

    EEPROM_lock();
    return EEPROM_OK;
}

EEPROM_status_t EEPROM_erase(uint16_t offset, uint16_t len)
{
    uint32_t addr;
    uint16_t page_offset;

    if ((len == 0u) || ((offset + len) > EEPROM_TOTAL_SIZE))
    {
        return EEPROM_ERR_INVALID_ADDR;
    }

    addr = EEPROM_START_ADDR + offset;
    page_offset = offset % EEPROM_PAGE_SIZE;

    if (page_offset != 0u)
    {
        return EEPROM_ERR_INVALID_ADDR;
    }

    EEPROM_unlock();

    while (len > 0u)
    {
        while ((FLASH->SR & FLASH_SR_BSY) != 0u) { }

        FLASH->CR |= FLASH_CR_PER;
        FLASH->AR = addr;
        FLASH->CR |= FLASH_CR_STRT;

        while ((FLASH->SR & FLASH_SR_BSY) != 0u) { }

        FLASH->CR &= ~FLASH_CR_PER;

        if ((FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) != 0u)
        {
            FLASH->SR = FLASH_SR_PGERR | FLASH_SR_WRPRTERR;
            EEPROM_lock();
            return EEPROM_ERR_WRITE_FAILED;
        }

        addr += EEPROM_PAGE_SIZE;
        len = (len > EEPROM_PAGE_SIZE) ? (len - EEPROM_PAGE_SIZE) : 0u;
    }

    EEPROM_lock();
    return EEPROM_OK;
}
