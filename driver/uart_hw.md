# uart_hw

Low-level UART driver for the STM32F103 hardware USART peripheral.

The driver configures one USART instance, its TX/RX GPIO pins, optional AFIO remapping, baudrate, parity and USART interrupt. It supports the data widths provided by the STM32F103 USART hardware:

- `UART_HW_DATA_BITS_8`
- `UART_HW_DATA_BITS_9`

For 4 to 32 data bits use [uart_sw.md](uart_sw.md), which implements a timer-driven software UART.

## Configuration

`UART_HW_config_t` fields:

- `txPort`, `txPin`: USART TX GPIO.
- `rxPort`, `rxPin`: USART RX GPIO.
- `usartRemapMask`, `usartRemap`: AFIO remap configuration.
- `usart`: USART instance, for example `USART1`.
- `usartIrqn`: corresponding IRQ, for example `USART1_IRQn`.
- `usartRccReg`, `usartRccBit`: RCC register and enable bit.
- `baudrate`: USART baudrate in bits per second.
- `dataBits`: 8 or 9 data bits.
- `parity`: `UART_HW_PARITY_NONE`, `UART_HW_PARITY_EVEN` or `UART_HW_PARITY_ODD`.
- `rxCallback`, `txCallback`: optional callbacks.

Example:

```c
static const UART_HW_config_t uart_config = {
	.txPort         = GPIOA,
	.txPin          = 9u,
	.rxPort         = GPIOA,
	.rxPin          = 10u,
	.usartRemapMask = 0u,
	.usartRemap     = 0u,
	.usart          = USART1,
	.usartIrqn      = USART1_IRQn,
	.usartRccReg    = &RCC->APB2ENR,
	.usartRccBit    = RCC_APB2ENR_USART1EN,
	.baudrate       = 9600u,
	.dataBits       = UART_HW_DATA_BITS_8,
	.parity         = UART_HW_PARITY_NONE,
	.rxCallback     = 0,
	.txCallback     = 0
};s
```

## API

- `UART_HW_init` configures the USART and enables its interrupt.
- `UART_HW_setRxCallback` registers a callback for each received byte.
- `UART_HW_setTxCallback` registers a callback after the final transmitted byte is complete.
- `UART_HW_send` sends a byte buffer synchronously and waits for transmission completion.
- `UART_HW_goToMuteMode` enables USART mute mode.
- `UART_HW_irqHandler` processes RX data and USART status flags. Call it from the selected USART IRQ handler.

The RX callback receives one `uint8_t` value. The TX callback and RX callback execute from interrupt/application context according to the calling API. `RS485` uses this driver internally while retaining ownership of the RS-485 direction pin.
