/*
 * Copyright (c) 2001-2003, Adam Dunkels.
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
 * This file is part of the uIP TCP/IP stack.
 *
 */

/**
 * \addtogroup uip
 * @{
 */

/**
 * \file
 *    ICMPv6 (RFC 4443) implementation, with message and error handling
 * \author Julien Abeille <jabeille@cisco.com>
 * \author Mathilde Durvy <mdurvy@cisco.com>
 */

#include <string.h>
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-icmp6.h"
#include "contiki-default-conf.h"
#include "net/routing/routing.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "ICMPv6"
#define LOG_LEVEL LOG_LEVEL_IPV6

#define UIP_ICMP6_ERROR_BUF  ((struct uip_icmp6_error *)UIP_ICMP_PAYLOAD)

/** \brief temporary IP address */
static uip_ipaddr_t tmp_ipaddr;

LIST(echo_reply_callback_list);
/*---------------------------------------------------------------------------*/
/* List of input handlers */
LIST(input_handler_list);
/*---------------------------------------------------------------------------*/
static uip_icmp6_input_handler_t *
input_handler_lookup(uint8_t type, uint8_t icode)
{
  uip_icmp6_input_handler_t *handler = NULL;

  for(handler = list_head(input_handler_list);
      handler != NULL;
      handler = list_item_next(handler)) {
    if(handler->type == type &&
       (handler->icode == icode ||
        handler->icode == UIP_ICMP6_HANDLER_CODE_ANY)) {
      return handler;
    }
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
uint8_t
uip_icmp6_input(uint8_t type, uint8_t icode)
{
  uip_icmp6_input_handler_t *handler = input_handler_lookup(type, icode);

  if(handler == NULL) {
    return UIP_ICMP6_INPUT_ERROR;
  }

  if(handler->handler == NULL) {
    return UIP_ICMP6_INPUT_ERROR;
  }

  handler->handler();
  return UIP_ICMP6_INPUT_SUCCESS;
}
/*---------------------------------------------------------------------------*/
void
uip_icmp6_register_input_handler(uip_icmp6_input_handler_t *handler)
{
  list_add(input_handler_list, handler);
}
/*---------------------------------------------------------------------------*/
static void
echo_request_input(void)
{
  /*
   * we send an echo reply. It is trivial if there was no extension
   * headers in the request otherwise we need to remove the extension
   * headers and change a few fields
   */
  LOG_INFO("Received Echo Request from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_("\n");

  /* IP header */
  UIP_IP_BUF->ttl = uip_ds6_if.cur_hop_limit;

  if(uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)){
    uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &UIP_IP_BUF->srcipaddr);
    uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);
  } else {
    uip_ipaddr_copy(&tmp_ipaddr, &UIP_IP_BUF->srcipaddr);
    uip_ipaddr_copy(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);
    uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &tmp_ipaddr);
  }

  uip_remove_ext_hdr();

  /* Below is important for the correctness of UIP_ICMP_BUF and the
   * checksum
   */

  /* Note: now UIP_ICMP_BUF points to the beginning of the echo reply */
  UIP_ICMP_BUF->type = ICMP6_ECHO_REPLY;
  UIP_ICMP_BUF->icode = 0;
  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();

  LOG_INFO("Sending Echo Reply to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(" from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_("\n");
  UIP_STAT(++uip_stat.icmp.sent);
  return;
}
/*---------------------------------------------------------------------------*/
void
uip_icmp6_error_output(uint8_t type, uint8_t code, uint32_t param)
{
  /* check if originating packet is not an ICMP error */
  uint16_t shift;

  if(uip_last_proto == UIP_PROTO_ICMP6 && UIP_ICMP_BUF->type < 128) {
    uipbuf_clear();
    return;
  }

  /* the source should not be unspecified nor multicast */
  if(uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr) ||
     uip_is_addr_mcast(&UIP_IP_BUF->srcipaddr)) {
    uipbuf_clear();
    return;
  }

  /* Remove all extension headers related to the routing protocol in place.
   * Keep all other extension headers, so as to match original packet. */
  if(NETSTACK_ROUTING.ext_header_remove() == 0) {
    LOG_WARN("Unable to remove ext header before sending ICMPv6 ERROR message\n");
  }

  /* remember data of original packet before shifting */
  uip_ipaddr_copy(&tmp_ipaddr, &UIP_IP_BUF->destipaddr);

  /* The ICMPv6 error message contains as much of possible of the invoking packet
   * (see RFC 4443 section 3). Make space for the additional IPv6 and
   * ICMPv6 headers here and move payload to the "right". What we move includes
    * extension headers */
  shift = UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ICMP6_ERROR_LEN;
  uip_len += shift;
  uip_len = MIN(uip_len, UIP_LINK_MTU);
  uip_ext_len = 0;
  memmove(uip_buf + shift, (void *)UIP_IP_BUF, uip_len - shift);

  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = uip_ds6_if.cur_hop_limit;

  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &UIP_IP_BUF->srcipaddr);

  if(uip_is_addr_mcast(&tmp_ipaddr)){
    if(type == ICMP6_PARAM_PROB && code == ICMP6_PARAMPROB_OPTION){
      uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &tmp_ipaddr);
    } else {
      uipbuf_clear();
      return;
    }
  } else {
    /* need to pick a source that corresponds to this node */
    uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &tmp_ipaddr);
  }

  UIP_ICMP_BUF->type = type;
  UIP_ICMP_BUF->icode = code;
  UIP_ICMP6_ERROR_BUF->param = uip_htonl(param);
  uipbuf_set_len_field(UIP_IP_BUF, uip_len - UIP_IPH_LEN);
  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();

  UIP_STAT(++uip_stat.icmp.sent);

  LOG_WARN("Sending ICMPv6 ERROR message type %d code %d to ", type, code);
  LOG_WARN_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_WARN_(" from ");
  LOG_WARN_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_WARN_("\n");
  return;
}

/*---------------------------------------------------------------------------*/
void
uip_icmp6_send(const uip_ipaddr_t *dest, int type, int code, int payload_len)
{
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = uip_ds6_if.cur_hop_limit;
  uipbuf_set_len_field(UIP_IP_BUF, UIP_ICMPH_LEN + payload_len);

  if(dest == NULL) {
    LOG_ERR("invalid argument; dest is NULL\n");
    return;
  }

  memcpy(&UIP_IP_BUF->destipaddr, dest, sizeof(*dest));
  uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);

  UIP_ICMP_BUF->type = type;
  UIP_ICMP_BUF->icode = code;

  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();

  uip_len = UIP_IPH_LEN + UIP_ICMPH_LEN + payload_len;

  UIP_STAT(++uip_stat.icmp.sent);
  UIP_STAT(++uip_stat.ip.sent);

  LOG_INFO("Sending ICMPv6 packet to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(", type %u, code %u, len %u\n", type, code, payload_len);

  tcpip_ipv6_output();
}
/*---------------------------------------------------------------------------*/
static void
echo_reply_input(void)
{
  int ttl;
  uip_ipaddr_t sender;

  LOG_INFO("Received Echo Reply from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_("\n");

  uip_ipaddr_copy(&sender, &UIP_IP_BUF->srcipaddr);
  ttl = UIP_IP_BUF->ttl;

  uip_remove_ext_hdr();

  /* Call all registered applications to let them know an echo reply
     has been received. */
  {
    struct uip_icmp6_echo_reply_notification *n;
    for(n = list_head(echo_reply_callback_list);
        n != NULL;
        n = list_item_next(n)) {
      if(n->callback != NULL) {
        n->callback(&sender, ttl,
                    (uint8_t *)UIP_ICMP_PAYLOAD,
                    uip_len - sizeof(struct uip_icmp_hdr) - UIP_IPH_LEN);
      }
    }
  }

  uipbuf_clear();
  return;
}
/*---------------------------------------------------------------------------*/
void
uip_icmp6_echo_reply_callback_add(struct uip_icmp6_echo_reply_notification *n,
                                  uip_icmp6_echo_reply_callback_t c)
{
  if(n != NULL && c != NULL) {
    n->callback = c;
    list_add(echo_reply_callback_list, n);
  }
}
/*---------------------------------------------------------------------------*/
void
uip_icmp6_echo_reply_callback_rm(struct uip_icmp6_echo_reply_notification *n)
{
  list_remove(echo_reply_callback_list, n);
}
/*---------------------------------------------------------------------------*/
UIP_ICMP6_HANDLER(echo_request_handler, ICMP6_ECHO_REQUEST,
                  UIP_ICMP6_HANDLER_CODE_ANY, echo_request_input);
UIP_ICMP6_HANDLER(echo_reply_handler, ICMP6_ECHO_REPLY,
                  UIP_ICMP6_HANDLER_CODE_ANY, echo_reply_input);
/*---------------------------------------------------------------------------*/
void
uip_icmp6_init()
{
  /* Register Echo Request and Reply handlers */
  uip_icmp6_register_input_handler(&echo_request_handler);
  uip_icmp6_register_input_handler(&echo_reply_handler);
}
/*---------------------------------------------------------------------------*/
/** @} */
