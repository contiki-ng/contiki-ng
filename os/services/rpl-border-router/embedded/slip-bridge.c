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
 *         Slip fallback interface
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Joel Hoglund <joel@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */
/*---------------------------------------------------------------------------*/
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/slip.h"
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "SLIP"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
void set_prefix_64(uip_ipaddr_t *);

static uip_ipaddr_t last_sender;

/*---------------------------------------------------------------------------*/
void
request_prefix(void)
{
  /* mess up uip_buf with a dirty request... */
  uip_buf[0] = '?';
  uip_buf[1] = 'P';
  uip_len = 2;
  slip_write(uip_buf, uip_len);
  uipbuf_clear();
}
/*---------------------------------------------------------------------------*/
static void
slip_input_callback(void)
{
  LOG_DBG("SIN: %u\n", uip_len);
  if(uip_buf[0] == '!') {
    LOG_INFO("Got configuration message of type %c\n",
             uip_buf[1]);
    if(uip_buf[1] == 'P') {
      uip_ipaddr_t prefix;
      /* Here we set a prefix !!! */
      memset(&prefix, 0, 16);
      memcpy(&prefix, &uip_buf[2], 8);

      uipbuf_clear();

      LOG_INFO("Setting prefix ");
      LOG_INFO_6ADDR(&prefix);
      LOG_INFO_("\n");
      set_prefix_64(&prefix);
    }
    uipbuf_clear();

  } else if(uip_buf[0] == '?') {
    LOG_INFO("Got request message of type %c\n", uip_buf[1]);
    if(uip_buf[1] == 'M') {
      char *hexchar = "0123456789abcdef";
      int j;
      /* this is just a test so far... just to see if it works */
      uip_buf[0] = '!';
      for(j = 0; j < UIP_LLADDR_LEN; j++) {
        uip_buf[2 + j * 2] = hexchar[uip_lladdr.addr[j] >> 4];
        uip_buf[3 + j * 2] = hexchar[uip_lladdr.addr[j] & 15];
      }
      uip_len = 18;
      slip_write(uip_buf, uip_len);
    }
    uipbuf_clear();
  } else {
    /* Save the last sender received over SLIP to avoid bouncing the
       packet back if no route is found */
    uip_ipaddr_copy(&last_sender, &UIP_IP_BUF->srcipaddr);
  }
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  slip_arch_init();
  process_start(&slip_process, NULL);
  slip_set_input_callback(slip_input_callback);
}
/*---------------------------------------------------------------------------*/
static int
output(void)
{
  if(uip_ipaddr_cmp(&last_sender, &UIP_IP_BUF->srcipaddr)) {
    /* Do not bounce packets back over SLIP if the packet was received
       over SLIP */
    LOG_ERR("slip-bridge: Destination off-link but no route src=");
    LOG_ERR_6ADDR(&UIP_IP_BUF->srcipaddr);
    LOG_ERR_(" dst=");
    LOG_ERR_6ADDR(&UIP_IP_BUF->destipaddr);
    LOG_ERR_("\n");
  } else {
    LOG_DBG("SUT: %u\n", uip_len);
    slip_send();
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface rpl_interface = {
  init, output
};
/*---------------------------------------------------------------------------*/
