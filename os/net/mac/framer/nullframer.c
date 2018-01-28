/*
 * Copyright (c) 2009, Swedish Institute of Computer Science.
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
 *         MAC framer that does nothing...
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */
#include "net/mac/framer/framer.h"
#include "net/packetbuf.h"

#ifdef NULLFRAMER_CONF_PARSE_802154
#define  NULLFRAMER_PARSE_802154  NULLFRAMER_CONF_PARSE_802154
#else
/* defaults to parsing of the 802154 header as that is used for Slip-Radio */
#define  NULLFRAMER_PARSE_802154 1
#endif

/*---------------------------------------------------------------------------*/
static int
is_broadcast_addr(uint8_t mode, uint8_t *addr)
{
  int i = mode == FRAME802154_SHORTADDRMODE ? 2 : 8;
  while(i-- > 0) {
    if(addr[i] != 0xff) {
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
hdr_length(void)
{
  /* never adds any header */
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
create(void)
{
  /* nothing extra... */
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
parse(void)
{
#if NULLFRAMER_PARSE_802154
  frame802154_t frame;
  int len;
  len = packetbuf_datalen();
  if(frame802154_parse(packetbuf_dataptr(), len, &frame)) {
    if(frame.fcf.dest_addr_mode) {
      if(!is_broadcast_addr(frame.fcf.dest_addr_mode, frame.dest_addr)) {
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, (linkaddr_t *)&frame.dest_addr);
      }
    }
    packetbuf_set_addr(PACKETBUF_ADDR_SENDER, (linkaddr_t *)&frame.src_addr);
    packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, frame.seq);
    return 0;
  }
#endif
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct framer no_framer = {
  hdr_length,
  create,
  parse
};
