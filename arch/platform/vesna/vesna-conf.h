#ifndef VESNA_CONF_H_
#define VESNA_CONF_H_

// Enable/disable hardware watchdog. Default: Disabled
#ifndef WATCHDOG_CONF_ENABLED
#define WATCHDOG_ENABLED    (0)
#else
#define WATCHDOG_ENABLED    (WATCHDOG_CONF_ENABLED)
#endif

// Enable/disable UART*. Default: Enabled
#ifndef UART1_CONF_ENABLED
#define UART1_ENABLED    (1)
#else
#define UART1_ENABLED    (UART1_CONF_ENABLED)
#endif

#ifndef SLIP_CONF_ENABLED
#define SLIP_ENABLED    (0)
#else
#define SLIP_ENABLED    (SLIP_CONF_ENABLED)
#endif

// Set default baud rate for UART1. Default: 115200
#ifndef UART1_CONF_BAUDRATE
#define UART1_BAUDRATE      (115200)
#else
#define UART1_BAUDRATE      (UART1_CONF_BAUDRATE)
#endif

#endif