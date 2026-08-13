/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * \file
 *         Standalone native NAT64 translator: lets a single node reach IPv4
 *         hosts through NAT64 without a border router or tun device.
 *
 *         NAT64 normally lives in a border router, which provides the uIP
 *         fallback interface and initializes the translator. This module is a
 *         native-platform stand-in: a fallback interface that hands the node's
 *         own 64:ff9b::/96 datagrams to the NAT64 service, which translates
 *         them over host IPv4 sockets. Its module-macros.h wires
 *         UIP_FALLBACK_INTERFACE to this interface, so building the module is
 *         all an application has to do.
 *
 * \author Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */
#include "contiki.h"
#include "net/ipv6/uip.h"
#include "nat64.h"
#include "nat64-platform.h"
#include "nat64-standalone.h"

#include "sys/log.h"
#define LOG_MODULE "NAT64-SA"
#define LOG_LEVEL LOG_LEVEL_INFO

/*---------------------------------------------------------------------------*/
/*
 * Called once at startup via UIP_FALLBACK_INTERFACE.init(). Bring up the
 * socket translator when NAT64 is enabled. There is no default route to undo
 * here: module-macros.h sets NATIVE_WITH_IPV6_DEFAULT_ROUTE=0, so the platform
 * never adds one.
 */
static void
init(void)
{
  if(nat64_is_enabled()) {
    nat64_platform_init();
    LOG_INFO("standalone NAT64 translator active (no border router, no tun)\n");
  }
}
/*---------------------------------------------------------------------------*/
/*
 * uIP hands packets with no matching route here. Translate NAT64-prefix
 * destinations through the NAT64 service; a lone node can reach nothing else,
 * so drop the rest.
 */
static int
output(void)
{
  if(uip_len == 0) {
    return 0;
  }
  if(nat64_is_ip64_addr(&UIP_IP_BUF->destipaddr)) {
    return nat64_output(uip_buf, uip_len);
  }
  LOG_DBG("dropping off-link packet to non-NAT64 destination ");
  LOG_DBG_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_DBG_("\n");
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface nat64_standalone_interface = { init, output };
/*---------------------------------------------------------------------------*/
