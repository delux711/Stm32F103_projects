#include <stdint.h>

extern int main(void);
extern void SystemInit(void);
extern void __libc_init_array(void);

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

void Reset_Handler(void);
void Default_Handler(void);

#define IRQ_WEAK_DEFAULT(handler_name) \
    void handler_name(void) __attribute__((weak, alias("Default_Handler")))

IRQ_WEAK_DEFAULT(NMI_Handler);
IRQ_WEAK_DEFAULT(HardFault_Handler);
IRQ_WEAK_DEFAULT(MemManage_Handler);
IRQ_WEAK_DEFAULT(BusFault_Handler);
IRQ_WEAK_DEFAULT(UsageFault_Handler);
IRQ_WEAK_DEFAULT(SVC_Handler);
IRQ_WEAK_DEFAULT(DebugMon_Handler);
IRQ_WEAK_DEFAULT(PendSV_Handler);
IRQ_WEAK_DEFAULT(SysTick_Handler);

IRQ_WEAK_DEFAULT(WWDG_IRQHandler);
IRQ_WEAK_DEFAULT(PVD_IRQHandler);
IRQ_WEAK_DEFAULT(TAMPER_IRQHandler);
IRQ_WEAK_DEFAULT(RTC_IRQHandler);
IRQ_WEAK_DEFAULT(FLASH_IRQHandler);
IRQ_WEAK_DEFAULT(RCC_IRQHandler);
IRQ_WEAK_DEFAULT(EXTI0_IRQHandler);
IRQ_WEAK_DEFAULT(EXTI1_IRQHandler);
IRQ_WEAK_DEFAULT(EXTI2_IRQHandler);
IRQ_WEAK_DEFAULT(EXTI3_IRQHandler);
IRQ_WEAK_DEFAULT(EXTI4_IRQHandler);
IRQ_WEAK_DEFAULT(DMA1_Channel1_IRQHandler);
IRQ_WEAK_DEFAULT(DMA1_Channel2_IRQHandler);
IRQ_WEAK_DEFAULT(DMA1_Channel3_IRQHandler);
IRQ_WEAK_DEFAULT(DMA1_Channel4_IRQHandler);
IRQ_WEAK_DEFAULT(DMA1_Channel5_IRQHandler);
IRQ_WEAK_DEFAULT(DMA1_Channel6_IRQHandler);
IRQ_WEAK_DEFAULT(DMA1_Channel7_IRQHandler);
IRQ_WEAK_DEFAULT(ADC1_2_IRQHandler);
IRQ_WEAK_DEFAULT(USB_HP_CAN1_TX_IRQHandler);
IRQ_WEAK_DEFAULT(USB_LP_CAN1_RX0_IRQHandler);
IRQ_WEAK_DEFAULT(CAN1_RX1_IRQHandler);
IRQ_WEAK_DEFAULT(CAN1_SCE_IRQHandler);
IRQ_WEAK_DEFAULT(EXTI9_5_IRQHandler);
IRQ_WEAK_DEFAULT(TIM1_BRK_IRQHandler);
IRQ_WEAK_DEFAULT(TIM1_UP_IRQHandler);
IRQ_WEAK_DEFAULT(TIM1_TRG_COM_IRQHandler);
IRQ_WEAK_DEFAULT(TIM1_CC_IRQHandler);
IRQ_WEAK_DEFAULT(TIM2_IRQHandler);
IRQ_WEAK_DEFAULT(TIM3_IRQHandler);
IRQ_WEAK_DEFAULT(TIM4_IRQHandler);
IRQ_WEAK_DEFAULT(I2C1_EV_IRQHandler);
IRQ_WEAK_DEFAULT(I2C1_ER_IRQHandler);
IRQ_WEAK_DEFAULT(I2C2_EV_IRQHandler);
IRQ_WEAK_DEFAULT(I2C2_ER_IRQHandler);
IRQ_WEAK_DEFAULT(SPI1_IRQHandler);
IRQ_WEAK_DEFAULT(SPI2_IRQHandler);
IRQ_WEAK_DEFAULT(USART1_IRQHandler);
IRQ_WEAK_DEFAULT(USART2_IRQHandler);
IRQ_WEAK_DEFAULT(USART3_IRQHandler);
IRQ_WEAK_DEFAULT(EXTI15_10_IRQHandler);
IRQ_WEAK_DEFAULT(RTCAlarm_IRQHandler);
IRQ_WEAK_DEFAULT(USBWakeUp_IRQHandler);

__attribute__((section(".isr_vector")))
void (*const g_pfnVectors[])(void) = {
    (void (*)(void))(&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMPER_IRQHandler,
    RTC_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Channel1_IRQHandler,
    DMA1_Channel2_IRQHandler,
    DMA1_Channel3_IRQHandler,
    DMA1_Channel4_IRQHandler,
    DMA1_Channel5_IRQHandler,
    DMA1_Channel6_IRQHandler,
    DMA1_Channel7_IRQHandler,
    ADC1_2_IRQHandler,
    USB_HP_CAN1_TX_IRQHandler,
    USB_LP_CAN1_RX0_IRQHandler,
    CAN1_RX1_IRQHandler,
    CAN1_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_IRQHandler,
    TIM1_UP_IRQHandler,
    TIM1_TRG_COM_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler,
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,
    RTCAlarm_IRQHandler,
    USBWakeUp_IRQHandler,
    0,
    0,
    0,
    Reset_Handler
};

void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    dst = &_sbss;
    while (dst < &_ebss)
    {
        *dst++ = 0u;
    }

    SystemInit();
    __libc_init_array();
    (void)main();

    while (1)
    {
    }
}

void Default_Handler(void)
{
    while (1)
    {
    }
}