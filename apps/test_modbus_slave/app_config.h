#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define DRIVER_SYSTICK_USE       1
#define DRIVER_DEBUG_USE         1
#define DRIVER_RS485_USE         1
#if !defined(DRIVER_RS485_UART_SW_USE)
#define DRIVER_RS485_UART_HW_USE 1
#else
#define DRIVER_UART_SW_USE 1
#define DRIVER_UART_SW_TIMER4_USE 1
#endif
#define DRIVER_DEVICE_CONFIG_USE 1
#define DRIVER_DEVICE_REGISTRY_USE 1
#define DRIVER_MODBUS_SLAVE_USE  1

#endif
