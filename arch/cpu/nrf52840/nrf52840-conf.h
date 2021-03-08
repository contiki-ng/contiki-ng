/*---------------------------------------------------------------------------*/
#ifndef NRF52840_CONF_H_
#define NRF52840_CONF_H_
/*---------------------------------------------------------------------------*/
#define NETSTACK_CONF_RADIO        nrf52840_ieee_driver
/*---------------------------------------------------------------------------*/
#ifndef UART0_CONF_BAUD_RATE
#define UART0_CONF_BAUD_RATE       NRF_UART_BAUDRATE_115200
#endif
/*---------------------------------------------------------------------------*/
#if NRF52840_NATIVE_USB

#ifndef DBG_CONF_USB
#define DBG_CONF_USB 1
#endif

#ifndef SLIP_ARCH_CONF_USB
#define SLIP_ARCH_CONF_USB 1
#endif

#endif
/*---------------------------------------------------------------------------*/
#endif /* NRF52840_CONF_H_ */
/*---------------------------------------------------------------------------*/
