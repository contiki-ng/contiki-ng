/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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

/**
 * \file
 *         COOJA Contiki mote main file.
 * \author
 *         Fredrik Osterlind <fros@sics.se>
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "sys/cc.h"
#include "sys/cooja_mt.h"
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Cooja"
#define LOG_LEVEL LOG_LEVEL_MAIN

#include "lib/random.h"
#include "lib/simEnvChange.h"

#include "net/netstack.h"
#include "net/queuebuf.h"

#include "dev/eeprom.h"
#include "dev/serial-line.h"
#include "dev/button-sensor.h"
#include "dev/pir-sensor.h"
#include "dev/vib-sensor.h"
#include "dev/moteid.h"
#include "dev/button-hal.h"
#include "dev/gpio-hal.h"

#if NETSTACK_CONF_WITH_IPV6
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#endif /* NETSTACK_CONF_WITH_IPV6 */

/* Sensors */
SENSORS(&pir_sensor, &vib_sensor);

/*---------------------------------------------------------------------------*/
/* Needed since the new LEDs API does not provide this prototype */
void leds_arch_init(void);
/*---------------------------------------------------------------------------*/
char simDontFallAsleep = 0;

int simProcessRunValue;
int simEtimerPending;
clock_time_t simEtimerNextExpirationTime;
/*---------------------------------------------------------------------------*/
static void
set_lladdr(void)
{
  linkaddr_t addr;

  memset(&addr, 0, sizeof(linkaddr_t));
#if NETSTACK_CONF_WITH_IPV6
  for(size_t i = 0; i < sizeof(uip_lladdr.addr); i += 2) {
    addr.u8[i + 1] = simMoteID & 0xff;
    addr.u8[i + 0] = simMoteID >> 8;
  }
#else /* NETSTACK_CONF_WITH_IPV6 */
  addr.u8[0] = simMoteID & 0xff;
  addr.u8[1] = simMoteID >> 8;
#endif /* NETSTACK_CONF_WITH_IPV6 */
  linkaddr_set_node_addr(&addr);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_one()
{
  gpio_hal_init();
  leds_arch_init();
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two()
{
  set_lladdr();
  button_hal_init();
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three()
{
  /* Initialize eeprom */
  eeprom_init();
  /* Start serial process */
  serial_line_init();
}
/*---------------------------------------------------------------------------*/
void
platform_main_loop()
{
  while(1) {
    simProcessRunValue = process_run();
    while(simProcessRunValue-- > 0) {
      process_run();
    }
    simProcessRunValue = process_nevents();

    /* Check if we must stay awake */
    if(simDontFallAsleep) {
      simDontFallAsleep = 0;
      simProcessRunValue = 1;
    }

    /* Return to COOJA */
    cooja_mt_yield();
  }
}
/*---------------------------------------------------------------------------*/
