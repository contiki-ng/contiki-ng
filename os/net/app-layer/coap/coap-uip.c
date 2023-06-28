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
#include "tinydtls.h"
#include "dtls.h"
#endif /* WITH_DTLS */

/* sanity check for configured values */
#if COAP_MAX_PACKET_SIZE > (UIP_BUFSIZE - UIP_IPH_LEN - UIP_UDPH_LEN)
#error "UIP_CONF_BUFFER_SIZE too small for COAP_MAX_CHUNK_SIZE"
#endif

#define SERVER_LISTEN_PORT        UIP_HTONS(COAP_DEFAULT_PORT)
#define SERVER_LISTEN_SECURE_PORT UIP_HTONS(COAP_DEFAULT_SECURE_PORT)

#ifdef WITH_DTLS
static dtls_handler_t cb;
static dtls_context_t *dtls_context = NULL;

static const coap_keystore_t *dtls_keystore = NULL;
static struct uip_udp_conn *dtls_conn = NULL;
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
    return 0;
  }
#endif

#ifdef WITH_DTLS
  if(ep != NULL && ep->secure != 0) {
    dtls_peer_t *peer;
    if(dtls_context == NULL) {
      return 0;
    }
    peer = dtls_get_peer(dtls_context, ep);
    if(peer != NULL) {
      /* only if handshake is done! */
      LOG_DBG("DTLS peer state for ");
      LOG_DBG_COAP_EP(ep);
      LOG_DBG_(" is %d (%sconnected)\n", peer->state,
               dtls_peer_is_connected(peer) ? "" : "not ");
      return dtls_peer_is_connected(peer);
    } else {
      LOG_DBG("DTLS did not find peer ");
      LOG_DBG_COAP_EP(ep);
      LOG_DBG_("\n");
      return 0;
    }
  }
#endif /* WITH_DTLS */

  /* Assume connected */
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_connect(coap_endpoint_t *ep)
{
  if(ep->secure == 0) {
    LOG_DBG("connect to ");
    LOG_DBG_COAP_EP(ep);
    LOG_DBG_("\n");
    return 1;
  }

#ifdef WITH_DTLS
  LOG_DBG("DTLS connect to ");
  LOG_DBG_COAP_EP(ep);
  LOG_DBG_("\n");

  /* setup all address info here... should be done to connect */
  if(dtls_context) {
    dtls_connect(dtls_context, ep);
    return 1;
  }
#endif /* WITH_DTLS */

  return 0;
}
/*---------------------------------------------------------------------------*/
void
coap_endpoint_disconnect(coap_endpoint_t *ep)
{
#ifdef WITH_DTLS
  if(ep && ep->secure && dtls_context) {
    dtls_close(dtls_context, ep);
  }
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
  dtls_init();

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

  if(dtls_context) {
    dtls_handle_message(dtls_context, (coap_endpoint_t *)get_src_endpoint(1),
                        uip_appdata, uip_datalen());
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
    if(dtls_context) {
      int ret;

      ret = dtls_write(dtls_context, (session_t *)ep, (uint8_t *)data, length);
      LOG_INFO("sent DTLS to ");
      LOG_INFO_COAP_EP(ep);
      if(ret < 0) {
        LOG_INFO_(" - error %d\n", ret);
      } else {
        LOG_INFO_(" %d/%u bytes\n", ret, length);
      }
      return ret;
    } else {
      LOG_WARN("no DTLS context\n");
      return -1;
    }
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
    dtls_context = dtls_new_context(dtls_conn);
  }
  if(!dtls_context) {
    LOG_WARN("DTLS: cannot create context\n");
  } else {
    dtls_set_handler(dtls_context, &cb);
  }
#endif /* WITH_DTLS */

  while(1) {
    PROCESS_YIELD();

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

/* This is input coming from the DTLS code - e.g. de-crypted input from
   the other side - peer */
static int
input_from_peer(struct dtls_context_t *ctx,
                session_t *session, uint8_t *data, size_t len)
{
  size_t i;

  if(LOG_DBG_ENABLED) {
    LOG_DBG("received DTLS data:");
    for(i = 0; i < len; i++) {
      LOG_DBG_("%c", data[i]);
    }
    LOG_DBG_("\n");
    LOG_DBG("Hex:");
    for(i = 0; i < len; i++) {
      LOG_DBG_("%02x", data[i]);
    }
    LOG_DBG_("\n");
  }

  /* Ensure that the endpoint is tagged as secure */
  session->secure = 1;

  coap_receive(session, data, len);

  return 0;
}

/* This is output from the DTLS code to be sent to peer (encrypted) */
static int
output_to_peer(struct dtls_context_t *ctx,
               session_t *session, uint8_t *data, size_t len)
{
  struct uip_udp_conn *udp_connection = dtls_get_app_data(ctx);
  LOG_DBG("output_to DTLS peer [");
  LOG_DBG_6ADDR(&session->ipaddr);
  LOG_DBG_("]:%u %ld bytes\n", uip_ntohs(session->port), (long)len);
  uip_udp_packet_sendto(udp_connection, data, len,
                        &session->ipaddr, session->port);
  return len;
}

/* This defines the key-store set API since we hookup DTLS here */
void
coap_set_keystore(const coap_keystore_t *keystore)
{
  dtls_keystore = keystore;
}

/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
static int
get_psk_info(struct dtls_context_t *ctx,
             const session_t *session,
             dtls_credentials_type_t type,
             const unsigned char *id, size_t id_len,
             unsigned char *result, size_t result_length)
{
  coap_keystore_psk_entry_t ks;

  if(dtls_keystore == NULL) {
    LOG_DBG("--- No key store available ---\n");
    return 0;
  }

  memset(&ks, 0, sizeof(ks));
  LOG_DBG("---===>>> Getting the Key or ID <<<===---\n");
  switch(type) {
  case DTLS_PSK_IDENTITY:
    if(id && id_len) {
      ks.identity_hint = id;
      ks.identity_hint_len = id_len;
      LOG_DBG("got psk_identity_hint: '");
      LOG_DBG_COAP_STRING((const char *)id, id_len);
      LOG_DBG_("'\n");
    }

    if(dtls_keystore->coap_get_psk_info) {
      /* we know that session is a coap endpoint */
      dtls_keystore->coap_get_psk_info((coap_endpoint_t *)session, &ks);
    }
    if(ks.identity == NULL || ks.identity_len == 0) {
      LOG_DBG("no psk_identity found\n");
      return 0;
    }

    if(result_length < ks.identity_len) {
      LOG_DBG("cannot return psk_identity -- buffer too small\n");
      return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }
    memcpy(result, ks.identity, ks.identity_len);
    LOG_DBG("psk_identity with %u bytes found\n", ks.identity_len);
    return ks.identity_len;

  case DTLS_PSK_KEY:
    if(dtls_keystore->coap_get_psk_info) {
      ks.identity = id;
      ks.identity_len = id_len;
      /* we know that session is a coap endpoint */
      dtls_keystore->coap_get_psk_info((coap_endpoint_t *)session, &ks);
    }
    if(ks.key == NULL || ks.key_len == 0) {
      LOG_DBG("PSK for unknown id requested, exiting\n");
      return dtls_alert_fatal_create(DTLS_ALERT_ILLEGAL_PARAMETER);
    }

    if(result_length < ks.key_len) {
      LOG_DBG("cannot return psk -- buffer too small\n");
      return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }
    memcpy(result, ks.key, ks.key_len);
    LOG_DBG("psk with %u bytes found\n", ks.key_len);
    return ks.key_len;

  default:
    LOG_WARN("unsupported key store request type: %d\n", type);
  }

  return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
}


static dtls_handler_t cb = {
  .write = output_to_peer,
  .read  = input_from_peer,
  .event = NULL,
#ifdef DTLS_PSK
  .get_psk_info = get_psk_info,
#endif /* DTLS_PSK */
#ifdef DTLS_ECC
  /* .get_ecdsa_key = get_ecdsa_key, */
  /* .verify_ecdsa_key = verify_ecdsa_key */
#endif /* DTLS_ECC */
};

#endif /* WITH_DTLS */
/*---------------------------------------------------------------------------*/
/** @} */
/** @} */
