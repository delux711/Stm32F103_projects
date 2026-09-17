# uart_sw

Timer-driven software UART for STM32F103. It supports 4 to 32 data bits, no/even/odd parity and one or two stop bits. The timer runs at eight interrupts per bit and is used for both transmit and receive timing.

## Configuration

`UART_SW_config_t` requires TX/RX GPIOs, a free hardware timer, its RCC register and bit, the timer IRQ number, and the baudrate. The timer must not be shared with another driver.

The selected timer IRQ must call `UART_SW_irqHandler()`. The BSP provides optional handlers when one of these application macros is defined:

- `DRIVER_UART_SW_TIMER2_USE`
- `DRIVER_UART_SW_TIMER3_USE`
- `DRIVER_UART_SW_TIMER4_USE`

Also define `DRIVER_UART_SW_USE` when using the driver from `bsp_irq.c`.

## API

- `UART_SW_init` configures GPIO and starts the selected timer.
- `UART_SW_send` sends a buffer of `uint32_t` words synchronously; callbacks execute from interrupt context.
- `UART_SW_setRxCallback` receives each valid data byte.
- `UART_SW_setTxCallback` runs after the complete buffer has been transmitted.
- `UART_SW_irqHandler` must be called from the selected timer IRQ handler.

The receive callback receives a `uint32_t` value. Only the configured least-significant data bits are transmitted or returned. For example, a 7-bit even-parity link uses `UART_SW_DATA_BITS_7`, `UART_SW_PARITY_EVEN` and `UART_SW_STOP_BITS_1`.