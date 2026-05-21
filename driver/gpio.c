#include "gpio.h"

void GPIO_enableClock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    }
    else if (port == GPIOB)
    {
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    }
    else if (port == GPIOC)
    {
        RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    }
    else if (port == GPIOD)
    {
        RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
    }
}

void GPIO_configPin(GPIO_TypeDef *port, uint8_t pin, gpio_config_t config)
{
    volatile uint32_t *reg;
    uint32_t shift;
    uint32_t value;

    if (pin > 15u)
    {
        return;
    }

    if (pin < 8u)
    {
        reg = &port->CRL;
        shift = (uint32_t)pin * 4u;
    }
    else
    {
        reg = &port->CRH;
        shift = (uint32_t)(pin - 8u) * 4u;
    }

    value = *reg;
    value &= ~(0xFu << shift);
    value |= (config << shift);
    *reg = value;
}
