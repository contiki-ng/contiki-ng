/*---------------------------------------------------------------------------*/
#ifndef NRF52840_CONF_H_
#define NRF52840_CONF_H_
/*---------------------------------------------------------------------------*/
#define NETSTACK_CONF_RADIO        nrf52840_ieee_driver
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
/*
 * If debugging and SLIP use the same peripheral, this will be 1. Don't modify
 * this
 */
#if SLIP_ARCH_CONF_ENABLED
#define DBG_CONF_SLIP_MUX (SLIP_ARCH_CONF_USB == DBG_CONF_USB)
#endif
/*---------------------------------------------------------------------------*/
#endif /* NRF52840_CONF_H_ */
/*---------------------------------------------------------------------------*/
