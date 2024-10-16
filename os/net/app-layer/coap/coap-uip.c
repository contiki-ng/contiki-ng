/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
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
 * \file
 *         CoAP transport implementation for uIPv6
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

/**
 * \addtogroup coap-transport
 * @{
 *
 * \defgroup coap-uip CoAP transport implementation for uIP
 * @{
 *
 * This is an implementation of CoAP transport and CoAP endpoint over uIP
 * with DTLS support.
 */

#include "contiki.h"
#include "net/ipv6/uip-udp-packet.h"
#include "net/ipv6/uiplib.h"
#include "net/routing/routing.h"
#include "coap.h"
#include "coap-engine.h"
#include "coap-endpoint.h"
#include "coap-transport.h"
#include "coap-transactions.h"
#include "coap-constants.h"
#include "coap-keystore.h"
#include "coap-keystore-simple.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "coap-uip"
#define LOG_LEVEL  LOG_LEVEL_COAP

#ifdef WITH_DTLS
#include "mbedtls-support/mbedtls-support.h"
#endif /* WITH_DTLS */

/* sanity check for configured values */
#if COAP_MAX_PACKET_SIZE > (UIP_BUFSIZE - UIP_IPH_LEN - UIP_UDPH_LEN)
#error "UIP_CONF_BUFFER_SIZE too small for COAP_MAX_CHUNK_SIZE"
#endif

#define SERVER_LISTEN_PORT        UIP_HTONS(COAP_DEFAULT_PORT)
#define SERVER_LISTEN_SECURE_PORT UIP_HTONS(COAP_DEFAULT_SECURE_PORT)

#ifdef WITH_DTLS
static struct uip_udp_conn *dtls_conn = NULL;
static const coap_keystore_t *dtls_keystore = NULL;
#ifdef COAP_DTLS_CONF_WITH_PSK
static int coap_ep_get_dtls_psk_info(const coap_endpoint_t *ep,
    coap_keystore_psk_entry_t *info);
#endif /* COAP_DTLS_CONF_WITH_PSK */
#ifdef COAP_DTLS_CONF_WITH_CERT
static int coap_ep_get_dtls_cert_info(const coap_endpoint_t *ep,
    coap_keystore_cert_entry_t *info);
#endif /* COAP_DTLS_CONF_WITH_CERT */
#endif /* WITH_DTLS */


PROCESS(coap_engine, "CoAP Engine");

static struct uip_udp_conn *udp_conn = NULL;

/*---------------------------------------------------------------------------*/
void
coap_endpoint_log(const coap_endpoint_t *ep)
{
  if(ep == NULL) {
    LOG_OUTPUT("(NULL EP)");
    return;
  }
  if(ep->secure) {
    LOG_OUTPUT("coaps://[");
  } else {
    LOG_OUTPUT("coap://[");
  }
  log_6addr(&ep->ipaddr);
  LOG_OUTPUT("]:%u", uip_ntohs(ep->port));
}
/*---------------------------------------------------------------------------*/
void
coap_endpoint_print(const coap_endpoint_t *ep)
{
  if(ep == NULL) {
    printf("(NULL EP)");
    return;
  }
  if(ep->secure) {
    printf("coaps://[");
  } else {
    printf("coap://[");
  }
  uiplib_ipaddr_print(&ep->ipaddr);
  printf("]:%u", uip_ntohs(ep->port));
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_snprint(char *buf, size_t size, const coap_endpoint_t *ep)
{
  int n;
  if(buf == NULL || size == 0) {
    return 0;
  }
  if(ep == NULL) {
    n = snprintf(buf, size - 1, "(NULL EP)");
  } else {
    if(ep->secure) {
      n = snprintf(buf, size - 1, "coaps://[");
    } else {
      n = snprintf(buf, size - 1, "coap://[");
    }
    if(n < size - 1) {
      n += uiplib_ipaddr_snprint(&buf[n], size - n - 1, &ep->ipaddr);
    }
    if(n < size - 1) {
      n += snprintf(&buf[n], size -n - 1, "]:%u", uip_ntohs(ep->port));
    }
  }
  if(n >= size - 1) {
    buf[size - 1] = '\0';
  }
  return n;
}
/*---------------------------------------------------------------------------*/
void
coap_endpoint_copy(coap_endpoint_t *destination,
                   const coap_endpoint_t *from)
{
  uip_ipaddr_copy(&destination->ipaddr, &from->ipaddr);
  destination->port = from->port;
  destination->secure = from->secure;
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_cmp(const coap_endpoint_t *e1, const coap_endpoint_t *e2)
{
  if(!uip_ipaddr_cmp(&e1->ipaddr, &e2->ipaddr)) {
    return 0;
  }
  return e1->port == e2->port && e1->secure == e2->secure;
}
/*---------------------------------------------------------------------------*/
static int
index_of(const char *data, int offset, int len, uint8_t c)
{
  if(offset < 0) {
    return offset;
  }
  for(; offset < len; offset++) {
    if(data[offset] == c) {
      return offset;
    }
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
static int
get_port(const char *inbuf, size_t len, uint32_t *value)
{
  int i;
  *value = 0;
  for(i = 0; i < len; i++) {
    if(inbuf[i] >= '0' && inbuf[i] <= '9') {
      *value = *value * 10 + (inbuf[i] - '0');
    } else {
      break;
    }
  }
  return i;
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_parse(const char *text, size_t size, coap_endpoint_t *ep)
{
  /* Only IPv6 supported */
  int start = index_of(text, 0, size, '[');
  int end = index_of(text, start, size, ']');
  uint32_t port;

  ep->secure = strncmp(text, "coaps:", 6) == 0;
  if(start >= 0 && end > start &&
     uiplib_ipaddrconv(&text[start], &ep->ipaddr)) {
    if(text[end + 1] == ':' &&
       get_port(text + end + 2, size - end - 2, &port)) {
      ep->port = UIP_HTONS(port);
    } else if(ep->secure) {
      /* Use secure CoAP port by default for secure endpoints. */
      ep->port = SERVER_LISTEN_SECURE_PORT;
    } else {
      ep->port = SERVER_LISTEN_PORT;
    }
    return 1;
  } else if(size < UIPLIB_IPV6_MAX_STR_LEN) {
    char buf[UIPLIB_IPV6_MAX_STR_LEN];
    memcpy(buf, text, size);
    buf[size] = '\0';
    if(uiplib_ipaddrconv(buf, &ep->ipaddr)) {
      ep->port = SERVER_LISTEN_PORT;
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static const coap_endpoint_t *
get_src_endpoint(uint8_t secure)
{
  static coap_endpoint_t src;
  uip_ipaddr_copy(&src.ipaddr, &UIP_IP_BUF->srcipaddr);
  src.port = UIP_UDP_BUF->srcport;
  src.secure = secure;
  return &src;
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_is_secure(const coap_endpoint_t *ep)
{
  return ep->secure;
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_is_connected(const coap_endpoint_t *ep)
{
#ifndef CONTIKI_TARGET_NATIVE
  if(!uip_is_addr_linklocal(&ep->ipaddr)
     && NETSTACK_ROUTING.node_is_reachable() == 0) {
    return false;
  }
#endif

#ifdef WITH_DTLS
  if(coap_ep_is_dtls_peer(ep)) {
    /* only if handshake is done! */
    LOG_DBG("DTLS peer state for ");
    LOG_DBG_COAP_EP(ep);
    LOG_DBG_(" is %sconnected\n", coap_ep_is_dtls_connected(ep) ? "" : "not ");
    return coap_ep_is_dtls_connected(ep);
  } else {
    LOG_DBG("DTLS did not find peer ");
    LOG_DBG_COAP_EP(ep);
    LOG_DBG_("\n");
    return false;
  }
#endif /* WITH_DTLS */

  /* Assume connected */
  return true;
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_connect(coap_endpoint_t *ep)
{
#ifdef WITH_DTLS
#ifdef COAP_DTLS_CONF_WITH_CLIENT
#ifdef COAP_DTLS_CONF_WITH_PSK
  static coap_keystore_psk_entry_t psk_info;
#endif /* COAP_DTLS_CONF_WITH_PSK */
#ifdef COAP_DTLS_CONF_WITH_CERT
  static coap_keystore_cert_entry_t cert_info;
#endif /* COAP_DTLS_CONF_WITH_CERT */
#endif /* COAP_DTLS_CONF_WITH_CLIENT */
#endif /* WITH_DTLS */

  if(!ep->secure) {
    LOG_DBG("connect to ");
    LOG_DBG_COAP_EP(ep);
    LOG_DBG_("\n");
    return 1;
  }

#ifdef WITH_DTLS
  LOG_DBG("DTLS connect to ");
  LOG_DBG_COAP_EP(ep);
  LOG_DBG_("\n");

#ifdef COAP_DTLS_CONF_WITH_CLIENT
#ifdef COAP_DTLS_CONF_WITH_PSK
  if(coap_ep_get_dtls_psk_info(ep, &psk_info) == 1) {
    return coap_ep_dtls_connect(ep, COAP_DTLS_SEC_MODE_PSK, &psk_info);
  }
#endif /* COAP_DTLS_CONF_WITH_PSK */
#ifdef COAP_DTLS_CONF_WITH_CERT
  if(coap_ep_get_dtls_cert_info(ep, &cert_info) == 1) {
    return coap_ep_dtls_connect(ep, COAP_DTLS_SEC_MODE_CERT, &cert_info);
  }
#endif /* COAP_DTLS_CONF_WITH_CERT */
  LOG_ERR("Unable to retrieve DTLS authorization info for \n");
  LOG_ERR_COAP_EP(ep);
  LOG_ERR_("\n");
#endif /* COAP_DTLS_CONF_WITH_CLIENT */
#endif /* WITH_DTLS */

  return 0;
}
/*---------------------------------------------------------------------------*/
#ifdef WITH_DTLS
#ifdef COAP_DTLS_CONF_WITH_SERVER
int
coap_secure_server_setup(void)
{
  coap_endpoint_t ep;
#ifdef COAP_DTLS_CONF_WITH_CERT
  static coap_keystore_cert_entry_t cert_info;
#endif /* COAP_DTLS_CONF_WITH_CERT */
#ifdef COAP_DTLS_CONF_WITH_PSK
  static coap_keystore_psk_entry_t psk_info;
#endif /* COAP_DTLS_CONF_WITH_PSK */

#ifdef COAP_DTLS_CONF_WITH_PSK
  if(coap_ep_get_dtls_psk_info(&ep, &psk_info) == 1) {
    return coap_dtls_server_setup(COAP_DTLS_SEC_MODE_PSK, &psk_info);
  }
#endif /* COAP_DTLS_CONF_WITH_PSK */

#ifdef COAP_DTLS_CONF_WITH_CERT
  if(coap_ep_get_dtls_cert_info(&ep, &cert_info) == 1) {
    return coap_dtls_server_setup(COAP_DTLS_SEC_MODE_CERT, &cert_info);
  }
#endif /* COAP_DTLS_CONF_WITH_CERT */
  return 0;
}
#endif /* COAP_DTLS_CONF_WITH_SERVER */
#endif /* WITH_DTLS */
/*---------------------------------------------------------------------------*/
void
coap_endpoint_disconnect(coap_endpoint_t *ep)
{
#ifdef WITH_DTLS
  coap_ep_dtls_disconnect(ep);
#endif /* WITH_DTLS */
}
/*---------------------------------------------------------------------------*/
uint8_t *
coap_databuf(void)
{
  return uip_appdata;
}
/*---------------------------------------------------------------------------*/
void
coap_transport_init(void)
{
  process_start(&coap_engine, NULL);
#ifdef WITH_DTLS
  coap_dtls_init();

#if COAP_DTLS_KEYSTORE_CONF_WITH_SIMPLE
  coap_keystore_simple_init();
#endif /* COAP_DTLS_KEYSTORE_CONF_WITH_SIMPLE */

#endif /* WITH_DTLS */
}
/*---------------------------------------------------------------------------*/
#ifdef WITH_DTLS
static void
process_secure_data(void)
{
  LOG_INFO("receiving secure UDP datagram from [");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_("]:%u\n", uip_ntohs(UIP_UDP_BUF->srcport));
  LOG_INFO("  Length: %u\n", uip_datalen());

  int ret;

  if((ret = coap_ep_dtls_handle_message(
        (const coap_endpoint_t *) get_src_endpoint(1))) > 0) {
    coap_receive(get_src_endpoint(1), uip_appdata, ret);
  }
}
#endif /* WITH_DTLS */
/*---------------------------------------------------------------------------*/
static void
process_data(void)
{
  LOG_INFO("receiving UDP datagram from [");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_("]:%u\n", uip_ntohs(UIP_UDP_BUF->srcport));
  LOG_INFO("  Length: %u\n", uip_datalen());

  coap_receive(get_src_endpoint(0), uip_appdata, uip_datalen());
}
/*---------------------------------------------------------------------------*/
int
coap_sendto(const coap_endpoint_t *ep, const uint8_t *data, uint16_t length)
{
  if(ep == NULL) {
    LOG_WARN("failed to send - no endpoint\n");
    return -1;
  }

  if(!coap_endpoint_is_connected(ep)) {
    LOG_WARN("endpoint ");
    LOG_WARN_COAP_EP(ep);
    LOG_WARN_(" not connected - dropping packet\n");
    return -1;
  }

#ifdef WITH_DTLS
  if(coap_endpoint_is_secure(ep)) {
    int ret;
    ret = coap_ep_dtls_write(ep, (unsigned char *) data, length);
    LOG_INFO("sent DTLS to ");
    LOG_INFO_COAP_EP(ep);
    if(ret < 0) {
      LOG_INFO_(" - error %d\n", ret);
    } else {
      LOG_INFO_(" %d/%u bytes\n", ret, length);
    }
    return ret;
  }
#endif /* WITH_DTLS */

  uip_udp_packet_sendto(udp_conn, data, length, &ep->ipaddr, ep->port);
  LOG_INFO("sent to ");
  LOG_INFO_COAP_EP(ep);
  LOG_INFO_(" %u bytes\n", length);
  return length;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coap_engine, ev, data)
{
  PROCESS_BEGIN();

  /* new connection with remote host */
  udp_conn = udp_new(NULL, 0, NULL);
  if(udp_conn == NULL) {
    LOG_ERR("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }

  udp_bind(udp_conn, SERVER_LISTEN_PORT);
  LOG_INFO("Listening on port %u\n", uip_ntohs(udp_conn->lport));

#ifdef WITH_DTLS
  /* create new context with app-data */
  dtls_conn = udp_new(NULL, 0, NULL);
  if(dtls_conn != NULL) {
    udp_bind(dtls_conn, SERVER_LISTEN_SECURE_PORT);
    LOG_INFO("DTLS listening on port %u\n", uip_ntohs(dtls_conn->lport));
    coap_dtls_conn_init(dtls_conn, PROCESS_CURRENT());
  }
#endif /* WITH_DTLS */

  while(1) {
    PROCESS_YIELD();
#ifdef WITH_DTLS
    if(ev == PROCESS_EVENT_POLL || ev == PROCESS_EVENT_TIMER) {
      coap_dtls_event_handler();
      continue;
    }
#endif /* WITH_DTLS */
    if(ev == tcpip_event) {
      if(uip_newdata()) {
#ifdef WITH_DTLS
        if(uip_udp_conn == dtls_conn) {
          process_secure_data();
          continue;
        }
#endif /* WITH_DTLS */
        process_data();
      }
    }
  } /* while (1) */

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/* DTLS */
#ifdef WITH_DTLS
/* This defines the key-store set API since we hookup DTLS here */
void
coap_set_keystore(const coap_keystore_t *keystore)
{
  dtls_keystore = keystore;
}
/*---------------------------------------------------------------------------*/
#ifdef COAP_DTLS_CONF_WITH_PSK
static int
coap_ep_get_dtls_psk_info(const coap_endpoint_t *ep,
    coap_keystore_psk_entry_t *info)
{
  if(NULL != dtls_keystore) {
    if(dtls_keystore->coap_get_psk_info) {
      /* Get identity first */
      dtls_keystore->coap_get_psk_info(ep, info);
      /* Get key */
      dtls_keystore->coap_get_psk_info(ep, info);
      return 1;
    }
  }
  return 0;
}
#endif /* COAP_DTLS_CONF_WITH_PSK */
/*---------------------------------------------------------------------------*/
#ifdef COAP_DTLS_CONF_WITH_CERT
static int
coap_ep_get_dtls_cert_info(const coap_endpoint_t *ep,
    coap_keystore_cert_entry_t *info)
{
  if(dtls_keystore != NULL && dtls_keystore->coap_get_cert_info != NULL) {
    return dtls_keystore->coap_get_cert_info(ep, info);
  }
  return 0;
}
#endif /* COAP_DTLS_CONF_WITH_CERT */

/*---------------------------------------------------------------------------*/
#endif /* WITH_DTLS */
/*---------------------------------------------------------------------------*/
/** @} */
/** @} */
