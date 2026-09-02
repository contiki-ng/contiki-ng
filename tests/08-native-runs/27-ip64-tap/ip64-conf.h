/*
 * ip64 configuration for the TAP test. The same shape as
 * arch/platform/zoul/orion/ip64-conf.h, with the ENC28J60 driver replaced by
 * a driver that puts the IPv4 side on a host TAP device.
 */

#ifndef IP64_CONF_H
#define IP64_CONF_H

#include "ip64/ip64-eth-interface.h"
#define IP64_CONF_UIP_FALLBACK_INTERFACE ip64_eth_interface
#define IP64_CONF_INPUT                  ip64_eth_interface_input

#include "ip64-tap-driver.h"
#define IP64_CONF_ETH_DRIVER             ip64_tap_driver

/* The test runs the node both ways: with a fixed address, and with one
   leased from a DHCP server on the host. */
#ifndef IP64_CONF_DHCP
#define IP64_CONF_DHCP 0
#endif

#endif /* IP64_CONF_H */
