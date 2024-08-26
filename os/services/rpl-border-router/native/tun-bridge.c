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
static uint16_t delaymsec = 0;
static uint32_t delaystartsec, delaystartmsec;
/*---------------------------------------------------------------------------*/
static void
tun_input_callback(void)
{
  /* Optional delay between outgoing packets */
  /* Base delay times number of 6lowpan fragments to be sent */
  /* delaymsec = 10; */
  if(delaymsec) {
    struct timeval tv;
    int dmsec;
    gettimeofday(&tv, NULL);
    dmsec = (tv.tv_sec - delaystartsec) * 1000 + tv.tv_usec / 1000 - delaystartmsec;
    if(dmsec < 0) {
      delaymsec = 0;
    }
    if(dmsec > delaymsec) {
      delaymsec = 0;
    }
  }

  if(delaymsec == 0) {
    int size = tun6_net_input(uip_buf, sizeof(uip_buf));
    uip_len = size;
    tcpip_input();

    if(slip_config_basedelay) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      delaymsec = slip_config_basedelay;
      delaystartsec = tv.tv_sec;
      delaystartmsec = tv.tv_usec / 1000;
    }
  }
}
/*---------------------------------------------------------------------------*/
const char *
tun_get_prefix(void)
{
  return tun6_net_get_prefix();
}
/*---------------------------------------------------------------------------*/
void
tun_init(void)
{
  slip_init();

  if(!tun6_net_init(tun_input_callback)) {
    err(1, "failed to open tun device %s", tun6_net_get_tun_name());
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
