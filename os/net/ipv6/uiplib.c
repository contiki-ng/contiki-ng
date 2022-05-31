/*
 * Copyright (c) 2004, Adam Dunkels and the Swedish Institute of
 * Computer Science.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack and the Contiki operating system.
 *
 *
 */

/**
 * \addtogroup uip-addr-lib
 * @{
 *
 * \file
 *         Implementation of the IP address manipulation library
 * \author
 *         Nicolas Tsiftes <nvt@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/ip64-addr.h"
#include <string.h>
#include <stdio.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "uiplib"
#define LOG_LEVEL  LOG_LEVEL_NONE

/*---------------------------------------------------------------------------*/
int
uiplib_ip6addrconv(const char *addrstr, uip_ip6addr_t *ipaddr)
{
  uint16_t value;
  int tmp, zero;
  unsigned int len;
  char c = 0;  //gcc warning if not initialized

  value = 0;
  zero = -1;
  if(*addrstr == '[') addrstr++;

  for(len = 0; len < sizeof(uip_ip6addr_t) - 1; addrstr++) {
    c = *addrstr;
    if(c == ':' || c == '\0' || c == ']' || c == '/') {
      ipaddr->u8[len] = (value >> 8) & 0xff;
      ipaddr->u8[len + 1] = value & 0xff;
      len += 2;
      value = 0;

      if(c == '\0' || c == ']' || c == '/') {
        break;
      }

      if(*(addrstr + 1) == ':') {
        /* Zero compression */
        if(zero < 0) {
          zero = len;
        }
        addrstr++;
      }
    } else {
      if(c >= '0' && c <= '9') {
        tmp = c - '0';
      } else if(c >= 'a' && c <= 'f') {
        tmp = c - 'a' + 10;
      } else if(c >= 'A' && c <= 'F') {
        tmp = c - 'A' + 10;
      } else {
        LOG_ERR("illegal char: '%c'\n", c);
        return 0;
      }
      value = (value << 4) + (tmp & 0xf);
    }
  }
  if(c != '\0' && c != ']' && c != '/') {
    LOG_ERR("too large address\n");
    return 0;
  }
  if(len < sizeof(uip_ip6addr_t)) {
    if(zero < 0) {
      LOG_ERR("too short address\n");
      return 0;
    }
    memmove(&ipaddr->u8[zero + sizeof(uip_ip6addr_t) - len],
            &ipaddr->u8[zero], len - zero);
    memset(&ipaddr->u8[zero], 0, sizeof(uip_ip6addr_t) - len);
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
/* Parse a IPv4-address from a string. Returns the number of characters read
 * for the address. */
int
uiplib_ip4addrconv(const char *addrstr, uip_ip4addr_t *ipaddr)
{
  unsigned char tmp;
  char c;
  unsigned char i, j;
  uint8_t charsread = 0;

  tmp = 0;

  for(i = 0; i < 4; ++i) {
    j = 0;
    do {
      c = *addrstr;
      ++j;
      if(j > 4) {
        return 0;
      }
      if(c == '.' || c == 0 || c == ' ') {
        ipaddr->u8[i] = tmp;
        tmp = 0;
      } else if(c >= '0' && c <= '9') {
      	tmp = (tmp * 10) + (c - '0');
      } else {
        return 0;
      }
      ++addrstr;
      ++charsread;
    } while(c != '.' && c != 0 && c != ' ');

  }
  return charsread - 1;
}
/*---------------------------------------------------------------------------*/
void
uiplib_ipaddr_print(const uip_ipaddr_t *addr)
{
  char buf[UIPLIB_IPV6_MAX_STR_LEN];
  uiplib_ipaddr_snprint(buf, sizeof(buf), addr);
  printf("%s", buf);
}
/*---------------------------------------------------------------------------*/
int
uiplib_ipaddr_snprint(char *buf, size_t size, const uip_ipaddr_t *addr)
{
  unsigned int n = 0;

  if(size == 0) {
    return 0;
  }

  if(addr == NULL) {
    return snprintf(buf, size, "(NULL IP addr)");
  } else if(ip64_addr_is_ipv4_mapped_addr(addr)) {
    /*
     * Printing IPv4-mapped addresses is done according to RFC 4291 [1]
     *
     *     "An alternative form that is sometimes more
     *     convenient when dealing with a mixed environment
     *     of IPv4 and IPv6 nodes is x:x:x:x:x:x:d.d.d.d,
     *     where the 'x's are the hexadecimal values of the
     *     six high-order 16-bit pieces of the address, and
     *     the 'd's are the decimal values of the four
     *     low-order 8-bit pieces of the address (standard
     *     IPv4 representation)."
     *
     * [1] https://tools.ietf.org/html/rfc4291#page-4
     */
    return snprintf(buf, size, "::FFFF:%u.%u.%u.%u", addr->u8[12],
                    addr->u8[13], addr->u8[14], addr->u8[15]);
  } else {
    int f = 0;
    for(size_t i = 0; i < sizeof(uip_ipaddr_t); i += 2) {
      uint16_t a = (addr->u8[i] << 8) + addr->u8[i + 1];
      if(a == 0 && f >= 0) {
        if(f++ == 0) {
          n += snprintf(buf+n, size-n, "::");
          if(n >= size) {
            return n;
          }
        }
      } else {
        if(f > 0) {
          f = -1;
        } else if(i > 0) {
          n += snprintf(buf+n, size-n, ":");
          if(n >= size) {
            return n;
          }
        }
        n += snprintf(buf+n, size-n, "%x", a);
        if(n >= size) {
          return n;
        }
      }
    }
  }
  return n;
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
