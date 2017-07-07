/*
 * Copyright (c) 2017, RISE SICS.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Header file for the logging system
 * \author
 *         Simon Duquennoy <simon.duquennoy@ri.se>
 */

/** \addtogroup sys
 * @{ */

/**
 * \defgroup log Per-module, per-level logging
 * @{
 *
 * The log module performs per-module, per-level logging
 *
 */

#if NETSTACK_CONF_WITH_IPV6

#include "sys/log.h"
#include "net/ip/ip64-addr.h"

int curr_log_level = LOG_LEVEL_DBG;

/*---------------------------------------------------------------------------*/
void
log_6addr(const uip_ipaddr_t *ipaddr)
{
  uint16_t a;
  unsigned int i;
  int f;

  if(ipaddr == NULL) {
    LOG_OUTPUT("(NULL IP addr)");
    return;
  }

  if(ip64_addr_is_ipv4_mapped_addr(ipaddr)) {
    /* Printing IPv4-mapped addresses is done according to RFC 4291 */
    LOG_OUTPUT("::FFFF:%u.%u.%u.%u", ipaddr->u8[12], ipaddr->u8[13], ipaddr->u8[14], ipaddr->u8[15]);
  } else {
    for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
      a = (ipaddr->u8[i] << 8) + ipaddr->u8[i + 1];
      if(a == 0 && f >= 0) {
        if(f++ == 0) {
          LOG_OUTPUT("::");
        }
      } else {
        if(f > 0) {
          f = -1;
        } else if(i > 0) {
          LOG_OUTPUT(":");
        }
        LOG_OUTPUT("%x", a);
      }
	  }
  }
}
/*---------------------------------------------------------------------------*/
void
log_6addr_compact(const uip_ipaddr_t *ipaddr)
{
  if(ipaddr == NULL) {
    LOG_OUTPUT("6A-NULL");
  } else if(uip_is_addr_mcast(ipaddr)) {
    LOG_OUTPUT("6M-%04x", UIP_HTONS(ipaddr->u16[sizeof(uip_ipaddr_t)/2-1]));
  } else if(uip_is_addr_linklocal(ipaddr)) {
    LOG_OUTPUT("6L-%04x", UIP_HTONS(ipaddr->u16[sizeof(uip_ipaddr_t)/2-1]));
  } else {
    LOG_OUTPUT("6G-%04x", UIP_HTONS(ipaddr->u16[sizeof(uip_ipaddr_t)/2-1]));
  }
}

#endif /* NETSTACK_CONF_WITH_IPV6 */

/*---------------------------------------------------------------------------*/
void
log_lladdr(const linkaddr_t *lladdr)
{
  if(lladdr == NULL) {
    LOG_OUTPUT("(NULL LL addr)");
    return;
  } else {
    unsigned int i;
    for(i = 0; i < LINKADDR_SIZE; i++) {
      if(i > 0 && i % 2 == 0) {
        LOG_OUTPUT(".");
      }
      LOG_OUTPUT("%02x", lladdr->u8[i]);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
log_lladdr_compact(const linkaddr_t *lladdr)
{
  if(lladdr == NULL || linkaddr_cmp(lladdr, &linkaddr_null)) {
    LOG_OUTPUT("LL-NULL");
  } else {
    LOG_OUTPUT("LL-%04x", UIP_HTONS(lladdr->u16[LINKADDR_SIZE/2-1]));
  }
}
/*---------------------------------------------------------------------------*/
void
log_set_level(int level)
{
  if(level >= LOG_LEVEL_NONE && level <= LOG_LEVEL_DBG) {
    curr_log_level = level;
  }
}
/*---------------------------------------------------------------------------*/
int
log_get_level(void)
{
  return curr_log_level;
}
/*---------------------------------------------------------------------------*/
const char *
log_level_to_str(int level)
{
  switch(level) {
    case LOG_LEVEL_NONE:
      return "None";
    case LOG_LEVEL_ERR:
      return "Errors";
    case LOG_LEVEL_WARN:
      return "Warnings";
    case LOG_LEVEL_INFO:
      return "Info";
    case LOG_LEVEL_DBG:
      return "Debug";
    default:
      return "N/A";
  }
}
/** @} */
/** @} */
