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
 *         A HEX text transport for CoAP
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "coap.h"
#include "coap-endpoint.h"
#include "coap-engine.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINTEP(ep) coap_endpoint_print(ep)
#else
#define PRINTF(...)
#define PRINTEP(ep)
#endif

#ifdef WITH_DTLS
#include "tinydtls.h"
#include "dtls.h"
#include "dtls_debug.h"
#endif /* WITH_DTLS */

#define BUFSIZE 1280

typedef union {
  uint32_t u32[(BUFSIZE + 3) / 4];
  uint8_t u8[BUFSIZE];
} coap_buf_t;

static coap_endpoint_t last_source;
static coap_buf_t coap_aligned_buf;
static uint16_t coap_buf_len;

#ifdef WITH_DTLS
#define PSK_DEFAULT_IDENTITY "Client_identity"
#define PSK_DEFAULT_KEY      "secretPSK"

static dtls_handler_t cb;
static dtls_context_t *dtls_context = NULL;

/* The PSK information for DTLS */
#define PSK_ID_MAXLEN 256
#define PSK_MAXLEN 256
static unsigned char psk_id[PSK_ID_MAXLEN];
static size_t psk_id_length = 0;
static unsigned char psk_key[PSK_MAXLEN];
static size_t psk_key_length = 0;
#endif /* WITH_DTLS */

/*---------------------------------------------------------------------------*/
static const coap_endpoint_t *
coap_src_endpoint(void)
{
  return &last_source;
}
/*---------------------------------------------------------------------------*/
void
coap_endpoint_copy(coap_endpoint_t *destination, const coap_endpoint_t *from)
{
  memcpy(destination, from, sizeof(coap_endpoint_t));
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_cmp(const coap_endpoint_t *e1, const coap_endpoint_t *e2)
{
  return e1->addr == e2->addr;
}
/*---------------------------------------------------------------------------*/
void
coap_endpoint_print(const coap_endpoint_t *ep)
{
  printf("%u", ep->addr);
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_parse(const char *text, size_t size, coap_endpoint_t *ep)
{
  /* Hex based CoAP has no addresses, just writes data to standard out */
  ep->addr = last_source.addr;
#ifdef WITH_DTLS
  ep->secure = 1;
#endif /* WITH_DTLS */
  return 1;
}
/*---------------------------------------------------------------------------*/
uint8_t *
coap_databuf(void)
{
  return coap_aligned_buf.u8;
}
/*---------------------------------------------------------------------------*/
uint16_t
coap_datalen()
{
  return coap_buf_len;
}
/*---------------------------------------------------------------------------*/
static int
hextod(char c)
{
  if(c >= '0' && c <= '9') {
    return c - '0';
  }
  if(c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if(c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
static void
stdin_callback(const char *line)
{
  uint8_t *buf;
  int i, len, llen, v1, v2;

  if(strncmp("COAPHEX:", line, 8) != 0) {
    /* Not a CoAP message */
    return;
  }

  line += 8;
  llen = strlen(line);
  if((llen & 1) != 0) {
    /* Odd number of characters - not hex */
    fprintf(stderr, "ERROR: %s\n", line);
    return;
  }

  buf = coap_databuf();
  for(i = 0, len = 0; i < llen; i += 2, len++) {
    v1 = hextod(line[i]);
    v2 = hextod(line[i + 1]);
    if(v1 < 0 || v2 < 0) {
      /* Not hex */
      fprintf(stderr, "ERROR: %s\n", line);
      return;
    }
    buf[len] = (uint8_t)(((v1 << 4) | v2) & 0xff);
  }

  PRINTF("RECV from ");
  PRINTEP(&last_source);
  PRINTF(" %u bytes\n", len);
  coap_buf_len = len;

  if(DEBUG) {
    int i;
    uint8_t *data;
    data = coap_databuf();
    printf("Received:");
    for(i = 0; i < len; i++) {
      printf("%02x", data[i]);
    }
    printf("\n");
  }

#ifdef WITH_DTLS
  /* DTLS receive??? */
  last_source.secure = 1;
  dtls_handle_message(dtls_context, (coap_endpoint_t *) coap_src_endpoint(), coap_databuf(), coap_datalen());
#else
  coap_receive(coap_src_endpoint(), coap_databuf(), coap_datalen());
#endif /* WITH_DTLS */
}
/*---------------------------------------------------------------------------*/
void
coap_transport_init(void)
{
  select_set_stdin_callback(stdin_callback);

  printf("CoAP listening on standard in\n");

#ifdef WITH_DTLS
  /* create new contet with app-data - no real app-data... */
  dtls_context = dtls_new_context(&last_source);
  if (!dtls_context) {
    PRINTF("DTLS: cannot create context\n");
    exit(-1);
  }

#ifdef DTLS_PSK
  psk_id_length = strlen(PSK_DEFAULT_IDENTITY);
  psk_key_length = strlen(PSK_DEFAULT_KEY);
  memcpy(psk_id, PSK_DEFAULT_IDENTITY, psk_id_length);
  memcpy(psk_key, PSK_DEFAULT_KEY, psk_key_length);
#endif /* DTLS_PSK */
  PRINTF("Setting DTLS handler\n");
  dtls_set_handler(dtls_context, &cb);
#endif /* WITH_DTLS */

}
/*---------------------------------------------------------------------------*/
void
coap_send_message(const coap_endpoint_t *ep, const uint8_t *data, uint16_t len)
{
  if(!coap_endpoint_is_connected(ep)) {
    PRINTF("CoAP endpoint not connected\n");
    return;
  }

#ifdef WITH_DTLS
  if(coap_endpoint_is_secure(ep)) {
    dtls_write(dtls_context, (session_t *)ep, (uint8_t *)data, len);
    return;
  }
#endif /* WITH_DTLS */

  int i;
  printf("COAPHEX:");
  for(i = 0; i < len; i++) {
    printf("%02x", data[i]);
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_connect(coap_endpoint_t *ep)
{
  if(ep->secure == 0) {
    return 1;
  }
#ifdef WITH_DTLS
  PRINTF("DTLS EP:");
  PRINTEP(ep);
  PRINTF(" len:%d\n", ep->size);

  /* setup all address info here... should be done to connect */

  dtls_connect(dtls_context, ep);
#endif /* WITH_DTLS */
  return 1;
}
/*---------------------------------------------------------------------------*/
void
coap_endpoint_disconnect(coap_endpoint_t *ep)
{
#ifdef WITH_DTLS
  dtls_close(dtls_context, ep);
#endif /* WITH_DTLS */
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
  if(ep->secure) {
#ifdef WITH_DTLS
    dtls_peer_t *peer;
    peer = dtls_get_peer(dtls_context, ep);
    if(peer != NULL) {
      /* only if handshake is done! */
      PRINTF("peer state for ");
      PRINTEP(ep);
      PRINTF(" is %d %d\n", peer->state, dtls_peer_is_connected(peer));
      return dtls_peer_is_connected(peer);
    } else {
      PRINTF("Did not find peer ");
      PRINTEP(ep);
      PRINTF("\n");
    }
#endif /* WITH_DTLS */
    return 0;
  }
  /* Assume that the UDP socket is already up... */
  return 1;
}

/* DTLS */
#ifdef WITH_DTLS

/* This is input coming from the DTLS code - e.g. de-crypted input from
   the other side - peer */
static int
input_from_peer(struct dtls_context_t *ctx,
                session_t *session, uint8_t *data, size_t len)
{
  size_t i;
  dtls_peer_t *peer;

  printf("received data:");
  for (i = 0; i < len; i++)
    printf("%c", data[i]);
  printf("\nHex:");
  for (i = 0; i < len; i++)
    printf("%02x", data[i]);
  printf("\n");

  /* Send this into coap-input */
  memmove(coap_databuf(), data, len);
  coap_buf_len = len;

  peer = dtls_get_peer(ctx, session);
  /* If we have a peer then ensure that the endpoint is tagged as secure */
  if(peer) {
    session->secure = 1;
  }

  coap_receive(session, coap_databuf(), coap_datalen());

  return 0;
}

/* This is output from the DTLS code to be sent to peer (encrypted) */
static int
output_to_peer(struct dtls_context_t *ctx,
               session_t *session, uint8_t *data, size_t len)
{
  int fd = *(int *)dtls_get_app_data(ctx);
  printf("output_to_peer len:%d %d (s-size: %d)\n", (int)len, fd,
         session->size);


  int i;
  printf("COAPHEX:");
  for(i = 0; i < len; i++) {
    printf("%02x", data[i]);
  }
  printf("\n");

  return len;
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
  PRINTF("---===>>> Getting the Key or ID <<<===---\n");
  switch (type) {
  case DTLS_PSK_IDENTITY:
    if (id_len) {
      dtls_debug("got psk_identity_hint: '%.*s'\n", id_len, id);
    }

    if (result_length < psk_id_length) {
      dtls_warn("cannot set psk_identity -- buffer too small\n");
      return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }

    memcpy(result, psk_id, psk_id_length);
    return psk_id_length;
  case DTLS_PSK_KEY:
    if (id_len != psk_id_length || memcmp(psk_id, id, id_len) != 0) {
      dtls_warn("PSK for unknown id requested, exiting\n");
      return dtls_alert_fatal_create(DTLS_ALERT_ILLEGAL_PARAMETER);
    } else if (result_length < psk_key_length) {
      dtls_warn("cannot set psk -- buffer too small\n");
      return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
    }

    memcpy(result, psk_key, psk_key_length);
    return psk_key_length;
  default:
    dtls_warn("unsupported request type: %d\n", type);
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
