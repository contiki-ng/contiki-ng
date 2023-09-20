/*
 * Copyright (c) 2006, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/radio/cc2420/cc2420.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "dev/uart0.h"
#include "dev/watchdog.h"
#include "dev/xmem.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/mac/framer/frame802154.h"
#include "dev/adxl345.h"
#include "sys/clock.h"
#include "sys/energest.h"

#if NETSTACK_CONF_WITH_IPV6
#include "net/ipv6/uip-ds6.h"
#endif /* NETSTACK_CONF_WITH_IPV6 */

#include "node-id-z1.h"
#include "cfs-coffee-arch.h"
#include "cfs/cfs-coffee.h"

extern unsigned char node_mac[8];

#if DCOSYNCH_CONF_ENABLED
static struct timer mgt_timer;
#endif
extern int msp430_dco_required;

#define UIP_OVER_MESH_CHANNEL 8
#if NETSTACK_CONF_WITH_IPV4
static uint8_t is_gateway;
#endif /* NETSTACK_CONF_WITH_IPV4 */

#ifdef EXPERIMENT_SETUP
#include "experiment-setup.h"
#endif

/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Z1"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/
#ifdef UART0_CONF_BAUD_RATE
#define UART0_BAUD_RATE UART0_CONF_BAUD_RATE
#else
#define UART0_BAUD_RATE 115200
#endif
/*---------------------------------------------------------------------------*/
#if 0
int
force_float_inclusion()
{
  extern int __fixsfsi;
  extern int __floatsisf;
  extern int __mulsf3;
  extern int __subsf3;

  return __fixsfsi + __floatsisf + __mulsf3 + __subsf3;
}
#endif
/*---------------------------------------------------------------------------*/
void
uip_log(char *msg)
{
  puts(msg);
}
/*---------------------------------------------------------------------------*/
#if 0
void
force_inclusion(int d1, int d2)
{
  snprintf(NULL, 0, "%d", d1 % d2);
}
#endif
/*---------------------------------------------------------------------------*/
static void
set_lladdr(void)
{
  linkaddr_t addr;

  memset(&addr, 0, sizeof(linkaddr_t));
#if NETSTACK_CONF_WITH_IPV6
  memcpy(addr.u8, node_mac, sizeof(addr.u8));
#else
  if(node_id == 0) {
    int i;
    for(i = 0; i < sizeof(linkaddr_t); ++i) {
      addr.u8[i] = node_mac[7 - i];
    }
  } else {
    addr.u8[0] = node_id & 0xff;
    addr.u8[1] = node_id >> 8;
  }
#endif
  linkaddr_set_node_addr(&addr);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_one(void)
{
  /*
   * Initalize hardware.
   */
  msp430_cpu_init();

  leds_init();
  leds_on(LEDS_RED);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two(void)
{
  clock_wait(100);

  uart0_init(BAUD2UBR(UART0_BAUD_RATE)); /* Must come before first printf */

  xmem_init();

  leds_off(LEDS_RED);
  /*
   * Hardware initialization done!
   */

    /* Restore node id if such has been stored in external mem */
  node_id_z1_restore();

  /* If no MAC address was burned, we use the node id or the Z1 product ID */
  if(!(node_mac[0] | node_mac[1] | node_mac[2] | node_mac[3] |
       node_mac[4] | node_mac[5] | node_mac[6] | node_mac[7])) {

#ifdef SERIALNUM
    if(!node_id) {
      LOG_INFO("Node id is not set, using Z1 product ID\n");
      node_id = SERIALNUM;
    }
#endif
    node_mac[0] = 0xc1;  /* Hardcoded for Z1 */
    node_mac[1] = 0x0c;  /* Hardcoded for Revision C */
    node_mac[2] = 0x00;  /* Hardcoded to arbitrary even number so that
                            the 802.15.4 MAC address is compatible with
                            an Ethernet MAC address - byte 0 (byte 2 in
                            the DS ID) */
    node_mac[3] = 0x00;  /* Hardcoded */
    node_mac[4] = 0x00;  /* Hardcoded */
    node_mac[5] = 0x00;  /* Hardcoded */
    node_mac[6] = node_id >> 8;
    node_mac[7] = node_id & 0xff;
  }

  /* Overwrite node MAC if desired at compile time */
#ifdef MACID
#warning "***** CHANGING DEFAULT MAC *****"
  node_mac[0] = 0xc1;  /* Hardcoded for Z1 */
  node_mac[1] = 0x0c;  /* Hardcoded for Revision C */
  node_mac[2] = 0x00;  /* Hardcoded to arbitrary even number so that
                          the 802.15.4 MAC address is compatible with
                          an Ethernet MAC address - byte 0 (byte 2 in
                          the DS ID) */
  node_mac[3] = 0x00;  /* Hardcoded */
  node_mac[4] = 0x00;  /* Hardcoded */
  node_mac[5] = 0x00;  /* Hardcoded */
  node_mac[6] = MACID >> 8;
  node_mac[7] = MACID & 0xff;
#endif

#ifdef IEEE_802154_MAC_ADDRESS
  /* for setting "hardcoded" IEEE 802.15.4 MAC addresses */
  {
    uint8_t ieee[] = IEEE_802154_MAC_ADDRESS;
    memcpy(node_mac, ieee, sizeof(uip_lladdr.addr));
    node_mac[7] = node_id & 0xff;
  }
#endif /* IEEE_802154_MAC_ADDRESS */

  random_init(node_mac[6] + node_mac[7]);

  set_lladdr();

  /*
   * main() will turn the radio on inside netstack_init(). The CC2420
   * must already be initialised by that time, so we do this here early.
   * Later on in stage three we set correct values for PANID and radio
   * short/long address.
   */
  cc2420_init();

  SENSORS_ACTIVATE(adxl345);

  leds_off(LEDS_ALL);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three(void)
{
  uint8_t longaddr[8];
  uint16_t shortaddr;

  process_start(&sensors_process, NULL);

  shortaddr = (linkaddr_node_addr.u8[0] << 8) + linkaddr_node_addr.u8[1];
  memset(longaddr, 0, sizeof(longaddr));
  linkaddr_copy((linkaddr_t *)&longaddr, &linkaddr_node_addr);

  cc2420_set_pan_addr(IEEE802154_PANID, shortaddr, longaddr);

  LOG_INFO("CC2420 CCA threshold %i\n", CC2420_CONF_CCA_THRESH);

#if DCOSYNCH_CONF_ENABLED
  timer_set(&mgt_timer, DCOSYNCH_PERIOD * CLOCK_SECOND);
#endif
}
/*---------------------------------------------------------------------------*/  
void
platform_idle(void)
{
  /*
   * Idle processing.
   */
  int s = splhigh();    /* Disable interrupts. */
  /* uart0_active is for avoiding LPM3 when still sending or receiving */
  if(process_nevents() != 0 || uart0_active()) {
    splx(s);      /* Re-enable interrupts. */
  } else {
#if DCOSYNCH_CONF_ENABLED
    /* before going down to sleep possibly do some management */
    if(timer_expired(&mgt_timer)) {
      watchdog_periodic();
      timer_reset(&mgt_timer);
      msp430_sync_dco();
#if CC2420_CONF_SFD_TIMESTAMPS
      cc2420_arch_sfd_init();
#endif /* CC2420_CONF_SFD_TIMESTAMPS */
    }
#endif

    /* Re-enable interrupts and go to sleep atomically. */
    ENERGEST_SWITCH(ENERGEST_TYPE_CPU, ENERGEST_TYPE_LPM);
    watchdog_stop();
    /* check if the DCO needs to be on - if so - only LPM 1 */
    if (msp430_dco_required) {
      _BIS_SR(GIE | CPUOFF); /* LPM1 sleep for DMA to work!. */
    } else {
      _BIS_SR(GIE | SCG0 | SCG1 | CPUOFF); /* LPM3 sleep. This
						statement will block
						until the CPU is
						woken up by an
						interrupt that sets
						the wake up flag. */
    }
    watchdog_start();
    ENERGEST_SWITCH(ENERGEST_TYPE_LPM, ENERGEST_TYPE_CPU);
  }
}
/*---------------------------------------------------------------------------*/
