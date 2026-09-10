# uart

Low-level UART driver for STM32F103 USART peripherals.

## API

- `UART_init` configures the USART, TX/RX pins, remap, baudrate, data bits, parity and IRQ.
- `UART_setRxCallback` registers the callback called from the USART IRQ for each received byte.
- `UART_setTxCallback` registers the callback called after the last transmitted byte is complete.
- `UART_send` sends a byte buffer and waits until the final byte is complete.
- `UART_goToMuteMode` enables USART mute mode.
- `UART_irqHandler` processes RX and USART error flags and is called directly by the application USART IRQ handler.

`UART_config_t` contains both `rxCallback` and `txCallback` fields. They can also be changed at runtime with the setter functions. `rs485.c` uses this driver for the UART transport and retains ownership of the RS-485 DIR pin. Existing `RS485_config_t` and RS485 functions remain the public configuration interface for applications.
