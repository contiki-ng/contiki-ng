/*
 * Copyright (c) 2017, RISE SICS, Yanzi Networks
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
 * 3. The name of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 *
 */
#include "contiki.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uipbuf.h"
#include <string.h>

/*---------------------------------------------------------------------------*/

static uint16_t uipbuf_attrs[UIPBUF_ATTR_MAX];
static uint16_t uipbuf_default_attrs[UIPBUF_ATTR_MAX];

/*---------------------------------------------------------------------------*/
void
uipbuf_clear(void)
{
  uip_len = 0;
  uip_ext_len = 0;
  uip_last_proto = 0;
  uipbuf_clear_attr();
}
/*---------------------------------------------------------------------------*/
bool
uipbuf_add_ext_hdr(int16_t len)
{
  if(len + uip_len <= UIP_LINK_MTU && len + uip_len >= 0 && len + uip_ext_len >= 0) {
    uip_ext_len += len;
    uip_len += len;
    return true;
  } else {
    return false;
  }
}
/*---------------------------------------------------------------------------*/
bool
uipbuf_set_len(uint16_t len)
{
  if(len <= UIP_LINK_MTU) {
    uip_len = len;
    return true;
  } else {
    return false;
  }
}
/*---------------------------------------------------------------------------*/
void
uipbuf_set_len_field(struct uip_ip_hdr *hdr, uint16_t len)
{
  hdr->len[0] = (len >> 8);
  hdr->len[1] = (len & 0xff);
}
/*---------------------------------------------------------------------------*/
uint16_t
uipbuf_get_len_field(struct uip_ip_hdr *hdr)
{
  return ((uint16_t)(hdr->len[0]) << 8) + hdr->len[1];
}
/*---------------------------------------------------------------------------*/
/* Get the next header given the buffer - start indicates that this is
   start of the IPv6 header - needs to be set to 0 when in an ext hdr */
uint8_t *
uipbuf_get_next_header(uint8_t *buffer, uint16_t size, uint8_t *protocol, bool start)
{
  int curr_hdr_len = 0;
  int next_hdr_len = 0;
  uint8_t *next_header = NULL;
  struct uip_ip_hdr *ipbuf = NULL;
  struct uip_ext_hdr *curr_ext = NULL;
  struct uip_ext_hdr *next_ext = NULL;

  if(start) {
    /* protocol in the IP buffer */
    ipbuf = (struct uip_ip_hdr *)buffer;
    *protocol = ipbuf->proto;
    curr_hdr_len = UIP_IPH_LEN;
  } else {
    /* protocol in the Ext hdr */
    curr_ext = (struct uip_ext_hdr *)buffer;
    *protocol = curr_ext->next;
    /* This is just an ext header */
    curr_hdr_len = (curr_ext->len << 3) + 8;
  }

  /* Check first if enough space for current header */
  if(curr_hdr_len > size) {
    return NULL;
  }
  next_header = buffer + curr_hdr_len;

  /* Check if the buffer is large enough for the next header */
  if(uip_is_proto_ext_hdr(*protocol)) {
    next_ext = (struct uip_ext_hdr *)next_header;
    next_hdr_len = (next_ext->len << 3) + 8;
  } else {
    if(*protocol == UIP_PROTO_TCP) {
      next_hdr_len = UIP_TCPH_LEN;
    } else if(*protocol == UIP_PROTO_UDP) {
      next_hdr_len = UIP_UDPH_LEN;
    } else if(*protocol == UIP_PROTO_ICMP6) {
      next_hdr_len = UIP_ICMPH_LEN;
    }
  }

  /* Size must be enough to hold both the current and next header */
  if(next_hdr_len == 0 || curr_hdr_len + next_hdr_len > size) {
    return NULL;
  }

  return next_header;
}
/*---------------------------------------------------------------------------*/
/* Get the final header given the buffer - that is assumed to be at start
   of an IPv6 header */
uint8_t *
uipbuf_get_last_header(uint8_t *buffer, uint16_t size, uint8_t *protocol)
{
  uint8_t *nbuf;

  nbuf = uipbuf_get_next_header(buffer, size, protocol, true);
  while(nbuf != NULL && uip_is_proto_ext_hdr(*protocol)) {
    /* move to the ext hdr */
    nbuf = uipbuf_get_next_header(nbuf, size - (nbuf - buffer), protocol, false);
  }

  /* In case the buffer wasn't large enough for all headers, return NULL */
  return nbuf;
}
/*---------------------------------------------------------------------------*/
uint8_t *
uipbuf_search_header(uint8_t *buffer, uint16_t size, uint8_t protocol)
{
  uint8_t *nbuf;
  uint8_t next_proto;

  nbuf = uipbuf_get_next_header(buffer, size, &next_proto, true);
  while(nbuf != NULL && next_proto != protocol && uip_is_proto_ext_hdr(next_proto)) {
    /* move to the ext hdr */
    nbuf = uipbuf_get_next_header(nbuf, size - (nbuf - buffer), &next_proto, false);
  }

  if(next_proto == protocol) {
    return nbuf;
  } else {
    return NULL;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * Common functions for uipbuf (attributes, etc).
 *
 */
/*---------------------------------------------------------------------------*/
uint16_t
uipbuf_get_attr(uint8_t type)
{
  if(type < UIPBUF_ATTR_MAX) {
    return uipbuf_attrs[type];
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
uipbuf_set_attr(uint8_t type, uint16_t value)
{
  if(type < UIPBUF_ATTR_MAX) {
    uipbuf_attrs[type] = value;
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
uipbuf_set_default_attr(uint8_t type, uint16_t value)
{
  if(type < UIPBUF_ATTR_MAX) {
    uipbuf_default_attrs[type] = value;
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
uipbuf_clear_attr(void)
{
  /* set everything to "defaults" */
  memcpy(uipbuf_attrs, uipbuf_default_attrs, sizeof(uipbuf_attrs));
}
/*---------------------------------------------------------------------------*/
void
uipbuf_set_attr_flag(uint16_t flag)
{
  /* Assume only 16-bits for flags now */
  uipbuf_attrs[UIPBUF_ATTR_FLAGS] |= flag;
}
/*---------------------------------------------------------------------------*/
void
uipbuf_clr_attr_flag(uint16_t flag)
{
  uipbuf_attrs[UIPBUF_ATTR_FLAGS] &= ~flag;
}
/*---------------------------------------------------------------------------*/
uint16_t
uipbuf_is_attr_flag(uint16_t flag)
{
  return (uipbuf_attrs[UIPBUF_ATTR_FLAGS] & flag) == flag;
}
/*---------------------------------------------------------------------------*/
void
uipbuf_init(void)
{
  memset(uipbuf_default_attrs, 0, sizeof(uipbuf_default_attrs));
  /* And initialize anything that should be initialized */
  uipbuf_set_default_attr(UIPBUF_ATTR_MAX_MAC_TRANSMISSIONS,
                          UIP_MAX_MAC_TRANSMISSIONS_UNDEFINED);
  /* set the not-set default value - this will cause the MAC layer to
     configure its default */
  uipbuf_set_default_attr(UIPBUF_ATTR_LLSEC_LEVEL,
                          UIPBUF_ATTR_LLSEC_LEVEL_MAC_DEFAULT);
}

/*---------------------------------------------------------------------------*/
