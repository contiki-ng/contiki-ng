/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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
 */

/**
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <err.h>
#include "border-router.h"
#include "tun6-net.h"

/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "BR"
#define LOG_LEVEL LOG_LEVEL_NONE

extern uint16_t slip_config_basedelay;
/*---------------------------------------------------------------------------*/
#if CLOCK_SECOND != 1000
#error The system clock must be ticking in milliseconds
#endif
static struct timer delay_timer;
/*---------------------------------------------------------------------------*/
static void
tun_input_callback(void)
{
  /* Optional delay between outgoing packets */
  /* Base delay times number of 6lowpan fragments to be sent */
  if(!slip_config_basedelay || timer_expired(&delay_timer)) {
    uip_len = tun6_net_input(uip_buf, sizeof(uip_buf));
    tcpip_input();

    if(slip_config_basedelay) {
      timer_set(&delay_timer, slip_config_basedelay);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
tun_init(void)
{
  timer_set(&delay_timer, 0);

  slip_init();

  if(!tun6_net_init(tun_input_callback)) {
    err(EXIT_FAILURE, "failed to open tun device %s", tun6_net_get_tun_name());
  }
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
static int
output(void)
{
  LOG_DBG("SUT: %u\n", uip_len);
  if(uip_len > 0) {
    return tun6_net_output(uip_buf, uip_len);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface rpl_interface = {
  init, output
};
/*---------------------------------------------------------------------------*/
