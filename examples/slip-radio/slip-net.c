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

#include "contiki.h"
#include "net/netstack.h"
#include "net/ipv6/uip.h"
#include "net/packetbuf.h"
#include "dev/slip.h"
#include "os/sys/log.h"

#include <stdio.h>

#define LOG_MODULE "slip-net"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335
/*---------------------------------------------------------------------------*/
static void
slipnet_init(void)
{
}
/*---------------------------------------------------------------------------*/
static void
slipnet_input(void)
{
  int i;
  /* radio should be configured for filtering so this should be simple */
  /* this should be sent over SLIP! */
  /* so just copy into uip-but and send!!! */
  /* Format: !R<data> ? */
  uip_len = packetbuf_datalen();
  i = packetbuf_copyto(uip_buf);

  LOG_DBG("Slipnet got input of len: %d, copied: %d\n",
          packetbuf_datalen(), i);

  for(i = 0; i < uip_len; i++) {
    LOG_DBG_("%02x", (unsigned char)uip_buf[i]);
    if((i & 15) == 15) {
      LOG_DBG_("\n");
    } else if((i & 7) == 7) {
      LOG_DBG_(" ");
    }
  }
  LOG_DBG_("\n");

  slip_write(uip_buf, uip_len);
}
/*---------------------------------------------------------------------------*/
static uint8_t
slipnet_output(const linkaddr_t *localdest)
{
  /* do nothing... */
  return 1;
}
/*---------------------------------------------------------------------------*/
const struct network_driver slipnet_driver = {
  "slipnet",
  slipnet_init,
  slipnet_input,
  /* output is likely never called - or at least not used as no IP packets
     should be produced in slip-radio */
  slipnet_output
};
/*---------------------------------------------------------------------------*/
