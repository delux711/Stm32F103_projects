#include "app_config.h"
#include "stm32f10x.h"
#include "system_stm32f10x.h"
#include "rs485.h"

static const RS485_config_t rs485_config = {
    .txPort      = GPIOB,
    .txPin       = 6u,
    .rxPort      = GPIOB,
    .rxPin       = 7u,
    .dirPort     = GPIOB,
    .dirPin      = 8u,
    .usartRemapMask = AFIO_MAPR_USART1_REMAP,
    .usartRemap     = AFIO_MAPR_USART1_REMAP,
    .usart          = USART1,
    .usartIrqn      = USART1_IRQn,
    .usartRccReg    = &RCC->APB2ENR,
    .usartRccBit    = RCC_APB2ENR_USART1EN
};

int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();

    RS485_init(&rs485_config);

    while (1)
    {
        RS485_process();
        __WFI();
    }
}
