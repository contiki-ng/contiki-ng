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
#include "contiki.h"
#include "sys/energest.h"
#include "cc2420.h"
#include "dev/ds2411/ds2411.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "dev/uart1.h"
#include "dev/watchdog.h"
#include "dev/xmem.h"
#include "lib/random.h"
#include "net/netstack.h"
#include "net/mac/framer/frame802154.h"
#include "net/mac/tsch/tsch.h"

#if NETSTACK_CONF_WITH_IPV6
#include "net/ipv6/uip-ds6.h"
#endif /* NETSTACK_CONF_WITH_IPV6 */

#include "sys/node-id.h"
#include "cfs-coffee-arch.h"
#include "cfs/cfs-coffee.h"

#if DCOSYNCH_CONF_ENABLED
static struct timer mgt_timer;
#endif
extern int msp430_dco_required;

#define UIP_OVER_MESH_CHANNEL 8

#ifdef EXPERIMENT_SETUP
#include "experiment-setup.h"
#endif

void init_platform(void);
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Sky"
#define LOG_LEVEL LOG_LEVEL_MAIN
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
void uip_log(char *msg) { puts(msg); }

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
  memcpy(addr.u8, ds2411_id, sizeof(addr.u8));
#else
  int i;
  for(i = 0; i < sizeof(linkaddr_t); ++i) {
    addr.u8[i] = ds2411_id[7 - i];
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
  uart1_init(BAUD2UBR(115200)); /* Must come before first printf */

  leds_on(LEDS_GREEN);
  ds2411_init();

  /* XXX hack: Fix it so that the 802.15.4 MAC address is compatible
     with an Ethernet MAC address - byte 0 (byte 2 in the DS ID)
     cannot be odd. */
  ds2411_id[2] &= 0xfe;

  leds_on(LEDS_BLUE);
  xmem_init();

  leds_off(LEDS_RED);
  /*
   * Hardware initialization done!
   */

  random_init(ds2411_id[0]);

  leds_off(LEDS_BLUE);

  set_lladdr();

  /*
   * main() will turn the radio on inside netstack_init(). The CC2420
   * must already be initialised by that time, so we do this here early.
   * Later on in stage three we set correct values for PANID and radio
   * short/long address.
   */
  cc2420_init();
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three(void)
{
  uint8_t longaddr[8];
  uint16_t shortaddr;

  init_platform();

  shortaddr = (linkaddr_node_addr.u8[0] << 8) + linkaddr_node_addr.u8[1];
  memset(longaddr, 0, sizeof(longaddr));
  linkaddr_copy((linkaddr_t *)&longaddr, &linkaddr_node_addr);

  cc2420_set_pan_addr(IEEE802154_PANID, shortaddr, longaddr);

  LOG_INFO("CC2420 CCA threshold %i\n", CC2420_CONF_CCA_THRESH);

#if !NETSTACK_CONF_WITH_IPV6
  uart1_set_input(serial_line_input_byte);
  serial_line_init();
#endif

  leds_off(LEDS_GREEN);

#if TIMESYNCH_CONF_ENABLED
  timesynch_init();
  timesynch_set_authority_level((linkaddr_node_addr.u8[0] << 4) + 16);
#endif /* TIMESYNCH_CONF_ENABLED */

  /*
   * This is the scheduler loop.
   */
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
  int s = splhigh();		/* Disable interrupts. */
  /* uart1_active is for avoiding LPM3 when still sending or receiving */
  if(process_nevents() != 0 || uart1_active()) {
    splx(s);			/* Re-enable interrupts. */
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
