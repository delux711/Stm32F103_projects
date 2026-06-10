#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#include "stm32f10x.h"

/*
    PP = Push-Pull
    OD = Open-Drain
*/
typedef enum
{
	GPIO_CFG_INPUT_ANALOG       = 0x0u,
	GPIO_CFG_INPUT_FLOATING     = 0x4u,
	GPIO_CFG_INPUT_PULL         = 0x8u,
	GPIO_CFG_OUTPUT_PP_10MHZ    = 0x1u,
	GPIO_CFG_OUTPUT_PP_2MHZ     = 0x2u,
	GPIO_CFG_OUTPUT_PP_50MHZ    = 0x3u,
	GPIO_CFG_OUTPUT_OD_10MHZ    = 0x5u,
	GPIO_CFG_OUTPUT_OD_2MHZ     = 0x6u,
	GPIO_CFG_OUTPUT_OD_50MHZ    = 0x7u,
	GPIO_CFG_OUTPUT_AF_PP_10MHZ = 0x9u,
	GPIO_CFG_OUTPUT_AF_PP_2MHZ  = 0xAu,
	GPIO_CFG_OUTPUT_AF_PP_50MHZ = 0xBu,
	GPIO_CFG_OUTPUT_AF_OD_10MHZ = 0xDu,
	GPIO_CFG_OUTPUT_AF_OD_2MHZ  = 0xEu,
	GPIO_CFG_OUTPUT_AF_OD_50MHZ = 0xFu
} gpio_config_t;

void GPIO_enableClock(GPIO_TypeDef *port);
void GPIO_configPin(GPIO_TypeDef *port, uint8_t pin, gpio_config_t config);

#endif
