# uart

Low-level UART driver for STM32F103 USART peripherals.

## API

- `UART_init` configures the USART, TX/RX pins, remap, baudrate, data bits, parity and IRQ.
- `UART_setRxCallback` registers the callback called from the USART IRQ for each received byte.
- `UART_send` sends a byte buffer and waits until the final byte is complete.
- `UART_goToMuteMode` enables USART mute mode.
- `UART_irqHandler` processes RX and USART error flags and is called by the application IRQ handler.

`rs485.c` uses this driver for the UART transport and retains ownership of the RS-485 DIR pin. Existing `RS485_config_t` and RS485 functions remain the public configuration interface for applications. `rs485_command.c` continues to use the RS485 transport API, so its UART access is provided by this driver through `rs485.c`.
