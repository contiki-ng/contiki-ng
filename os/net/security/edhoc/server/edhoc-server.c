/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB
 * Copyright (c) 2020, Industrial Systems Institute (ISI), Patras, Greece
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
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
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
 *      EDHOC server API [RFC9528] with CoAP Block-Wise Transfer [RFC7959]
 * \author
 *      Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund, Marco Tiloca, Niclas Finne <niclas.finne@ri.se>, Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "edhoc-server.h"
#include "edhoc-msg-generators.h"
#include "edhoc-msg-handlers.h"
#include "edhoc-trace.h"

#include "sys/log.h"
#define LOG_MODULE "EDHOC"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/* EDHOC Client protocol states */
#define NON_MSG 0
#define RX_MSG1 1
#define RX_MSG3 2
#define TX_MSG_ERR 3
#define EXP_READY 4
#define RESTART 5

static coap_timer_t timer;
static uint8_t msg_rx[EDHOC_MAX_PAYLOAD_LEN];
static size_t msg_rx_len;
static edhoc_server_t server;
static edhoc_server_t *serv;
static process_event_t new_ecc_event;
static ecc_data_event_t new_ecc;

static coap_message_t *request;
static coap_message_t *response;
static int err = 0;
static edhoc_msg_3_t msg3;

#if EDHOC_TEST == EDHOC_TEST_VECTOR_TRACE_DH
/* Hard-wired ephemeral keys from RFC 9529 for verifying that all operations yield the same
   intermediate results as the test vectors. */
static const uint8_t eph_pub_x_r[ECC_KEY_LEN] = { 0x41, 0x97, 0x01, 0xd7, 0xf0, 0x0a, 0x26, 0xc2, 0xdc, 0x58, 0x7a, 0x36, 0xdd, 0x75, 0x25, 0x49, 0xf3, 0x37, 0x63, 0xc8, 0x93, 0x42, 0x2c,
                                                  0x8e, 0xa0, 0xf9, 0x55, 0xa1, 0x3a, 0x4f, 0xf5, 0xd5 };

static const uint8_t eph_pub_y_r[ECC_KEY_LEN] = { 0x5e, 0x4f, 0x0d, 0xd8, 0xa3, 0xda, 0x0b, 0xaa, 0x16, 0xb9, 0xd3, 0xad, 0x56, 0xa0, 0xc1, 0x86, 0x0a, 0x94, 0x0a, 0xf8, 0x59, 0x14, 0x91,
                                                  0x5e, 0x25, 0x01, 0x9b, 0x40, 0x24, 0x17, 0xe9, 0x9d };

static const uint8_t eph_private_r[ECC_KEY_LEN] = { 0xe2, 0xf4, 0x12, 0x67, 0x77, 0x20, 0x5e, 0x85, 0x3b, 0x43, 0x7d, 0x6e, 0xac, 0xa1, 0xe1, 0xf7, 0x53, 0xcd, 0xcc, 0x3e, 0x2c, 0x69, 0xfa,
                                                    0x88, 0x4b, 0x0a, 0x1a, 0x64, 0x09, 0x77, 0xe4, 0x18 };
#endif
/*---------------------------------------------------------------------------*/
static void
generate_ephemeral_key(uint8_t curve_id, uint8_t *pub_x, uint8_t *pub_y, uint8_t *priv)
{
  if(!ecdh_generate_keypair(curve_id, pub_x, pub_y, priv)) {
    LOG_ERR("Failed to generate ephemeral key pair\n");
    return;
  }

#if EDHOC_TEST == EDHOC_TEST_VECTOR_TRACE_DH
  if(edhoc_ctx != NULL) {
    memcpy(edhoc_ctx->creds.ephemeral_key.pub.x, eph_pub_x_r, ECC_KEY_LEN);
    memcpy(edhoc_ctx->creds.ephemeral_key.pub.y, eph_pub_y_r, ECC_KEY_LEN);
    memcpy(edhoc_ctx->creds.ephemeral_key.priv, eph_private_r, ECC_KEY_LEN);
  } else {
    LOG_ERR("Test vector key copy failed - invalid context\n");
  }
#endif

  edhoc_trace_ephemeral_key("Responder",
                            edhoc_ctx->creds.ephemeral_key.pub.x,
                            edhoc_ctx->creds.ephemeral_key.pub.y,
                            edhoc_ctx->creds.ephemeral_key.priv);
}
/*---------------------------------------------------------------------------*/
int8_t
edhoc_server_callback(process_event_t ev, void *data)
{
  if(ev == new_ecc_event && new_ecc.val == SERV_HANDSHAKE_COMPLETE) {
    return SERV_HANDSHAKE_COMPLETE;
  }

  if(ev == new_ecc_event && new_ecc.val == SERV_HANDSHAKE_RESET) {
    LOG_DBG("server callback: SERV_HANDSHAKE_RESET\n");
    return SERV_HANDSHAKE_RESET;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
edhoc_server_set_ad_2(const void *buf, uint8_t buf_sz)
{
  if(buf_sz > EDHOC_MAX_AD_SZ) {
    LOG_ERR("AD_2 size (%u) exceeds maximum AD size (%d)\n", buf_sz, EDHOC_MAX_AD_SZ);
    new_ecc.ad.ad_2_sz = 0;
    return;
  }
  memcpy(new_ecc.ad.ad_2, (void *)buf, buf_sz);
  new_ecc.ad.ad_2_sz = buf_sz;
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_server_get_ad_1(char *buf)
{
  memcpy(buf, (void *)new_ecc.ad.ad_1, new_ecc.ad.ad_1_sz);
  return new_ecc.ad.ad_1_sz;
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_server_get_ad_3(char *buf)
{
  memcpy(buf, (void *)new_ecc.ad.ad_3, new_ecc.ad.ad_3_sz);
  return new_ecc.ad.ad_3_sz;
}
/*---------------------------------------------------------------------------*/
static void
reset_handshake_with_error(void)
{
  coap_timer_stop(&timer);
  edhoc_server_close();
  new_ecc.val = SERV_HANDSHAKE_RESET;
  process_post(PROCESS_BROADCAST, new_ecc_event, &new_ecc);
}
/*---------------------------------------------------------------------------*/
static void
server_timeout_callback(coap_timer_t *timer)
{
  LOG_ERR("Timeout\n");
  reset_handshake_with_error();
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static int
handle_msg1_state(void)
{
  edhoc_trace_message(1, msg_rx, msg_rx_len, false);

  err = edhoc_handler_msg_1(edhoc_ctx, msg_rx, msg_rx_len,
                           (uint8_t *)new_ecc.ad.ad_1);

  if(err == EDHOC_ERR_MSG_MALFORMED) {
    LOG_WARN("error code (%d)\n", err);
    serv->state = NON_MSG;
    reset_handshake_with_error();
    return -1;
  }

  if(err < EDHOC_ERR_MSG_MALFORMED) {
    LOG_WARN("Send MSG error with code (%d)\n", err);
    edhoc_ctx->buffers.tx_sz =
      edhoc_generate_error_message(edhoc_ctx->buffers.msg_tx, EDHOC_MAX_PAYLOAD_LEN,
                          edhoc_ctx, err);
    if(edhoc_ctx->buffers.tx_sz == 0) {
      LOG_ERR("Failed to generate error message\n");
    }
    serv->state = TX_MSG_ERR;
    if(err != EDHOC_ERR_SUITE_NOT_SUPPORTED) {
      reset_handshake_with_error();
    }
    return -1;
  }

  memcpy(&serv->con_ipaddr, &request->src_ep->ipaddr, sizeof(uip_ipaddr_t));
  new_ecc.ad.ad_1_sz = err;
  if(new_ecc.ad.ad_1_sz > 0) {
    EDHOC_DBG_VALUE("AD_1", new_ecc.ad.ad_1, new_ecc.ad.ad_1_sz);
  }
  serv->rx_msg1 = true;

  EDHOC_TRACE_STEP("message_2 generation");
  generate_ephemeral_key(edhoc_ctx->config.ecdh_curve,
                         edhoc_ctx->creds.ephemeral_key.pub.x,
                         edhoc_ctx->creds.ephemeral_key.pub.y,
                         edhoc_ctx->creds.ephemeral_key.priv);
  edhoc_error_t result = edhoc_generate_message_2(edhoc_ctx, (uint8_t *)new_ecc.ad.ad_2,
                  new_ecc.ad.ad_2_sz);
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to generate MSG2: %s\n", edhoc_error_string(result));
    reset_handshake_with_error();
    return -1;
  }
  edhoc_trace_message(2, edhoc_ctx->buffers.msg_tx, edhoc_ctx->buffers.tx_sz, true);
  serv->state = RX_MSG3;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
handle_msg3_state(void)
{
  edhoc_trace_message(3, msg_rx, msg_rx_len, false);

  err = edhoc_handler_msg_3(&msg3, edhoc_ctx, msg_rx, msg_rx_len);

  if(err > 0) {
    err = edhoc_authenticate_msg(edhoc_ctx, (uint8_t *)new_ecc.ad.ad_3, false);
  }

  if(err == EDHOC_ERR_MSG_MALFORMED) {
    serv->state = NON_MSG;
    reset_handshake_with_error();
    return -1;
  }

  if(err < EDHOC_ERR_MSG_MALFORMED) {
    edhoc_ctx->buffers.tx_sz =
      edhoc_generate_error_message(edhoc_ctx->buffers.msg_tx, EDHOC_MAX_PAYLOAD_LEN,
                          edhoc_ctx, err);
    if(edhoc_ctx->buffers.tx_sz == 0) {
      LOG_ERR("Failed to generate error message\n");
    }
    reset_handshake_with_error();
    serv->state = TX_MSG_ERR;
    return -1;
  }

  new_ecc.ad.ad_3_sz = err;
  if(new_ecc.ad.ad_3_sz > 0) {
    EDHOC_DBG_VALUE("AD_3", new_ecc.ad.ad_3, new_ecc.ad.ad_3_sz);
  }

  serv->state = EXP_READY;
  serv->rx_msg3 = true;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
handle_exp_ready_state(void)
{
  if(serv->rx_msg1 && serv->rx_msg3) {
    EDHOC_TRACE_STEP("key export ready");
    edhoc_trace_session_summary(edhoc_ctx);
    edhoc_ctx->buffers.tx_sz = 0;
    new_ecc.val = SERV_HANDSHAKE_COMPLETE;
    coap_timer_stop(&timer);
    process_post(PROCESS_BROADCAST, new_ecc_event, &new_ecc);
    return 0;
  } else {
    LOG_ERR("Protocol step missed\n");
    serv->state = NON_MSG;
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
static void
setup_coap_response(void)
{
  if(serv->state == NON_MSG) {
    LOG_ERR("RX MSG ERROR response\n");
    coap_set_payload(response, NULL, 0);
    coap_set_status_code(response, DELETED_2_02);
    return;
  }

  response->payload = (uint8_t *)edhoc_ctx->buffers.msg_tx;
  response->payload_len = edhoc_ctx->buffers.tx_sz;
  coap_set_status_code(response, CHANGED_2_04);

  memset(&(response->options), 0, sizeof(response->options));

  if(response->payload_len == 0) {
    memset(&(request->options), 0, sizeof(request->options));
  } else {
    coap_set_header_block2(response, 0,
                           edhoc_ctx->buffers.tx_sz > COAP_MAX_CHUNK_SIZE ? 1 : 0,
                           COAP_MAX_CHUNK_SIZE);
  }

  if(serv->state == TX_MSG_ERR) {
    serv->state = NON_MSG;
  }

  EDHOC_DBG_VALUE("Block1 info", (uint8_t*)&response->block1_num, 0);
  EDHOC_DBG_VALUE("Block2 info", (uint8_t*)&response->block2_num, 0);
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_server_reset_handshake(void)
{
  serv->con_num = 0;
  serv->state = 0;
  serv->rx_msg1 = false;
  serv->rx_msg3 = false;
  serv->state = NON_MSG;
  if(!edhoc_setup_suites(edhoc_ctx)) {
    return 0;
  }
  return edhoc_initialize_context(edhoc_ctx);
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_server_start(void)
{
  LOG_INFO("SERVER: EDHOC new\n");
  edhoc_ctx = edhoc_new();
  if(edhoc_ctx == NULL) {
    LOG_ERR("Failed to create EDHOC context\n");
    return 0;
  }
  serv = &server;
  return edhoc_server_reset_handshake();
}
/*---------------------------------------------------------------------------*/
void
edhoc_server_init(void)
{
  LOG_INFO("SERVER: CoAP active resource\n");
  coap_activate_resource(&res_edhoc, EDHOC_COAP_URI_PATH);
  new_ecc_event = process_alloc_event();
}
/*---------------------------------------------------------------------------*/
void
edhoc_server_close(void)
{
  edhoc_finalize(edhoc_ctx);
}
/*---------------------------------------------------------------------------*/
void
edhoc_server_process(coap_message_t *req, coap_message_t *res,
                     edhoc_server_t *ser, uint8_t *msg, size_t len)
{
  if(len == 0) {
    LOG_ERR("Message length is zero\n");
    coap_set_payload(res, NULL, 0);
    coap_set_status_code(res, BAD_REQUEST_4_00);
    return;
  }
  if(len > EDHOC_MAX_PAYLOAD_LEN) {
    LOG_ERR("Message too large: %zu bytes (max %u)\n", len, EDHOC_MAX_PAYLOAD_LEN);
    coap_set_payload(res, NULL, 0);
    coap_set_status_code(res, BAD_REQUEST_4_00);
    return;
  }

  memcpy(msg_rx, msg, len);
  msg_rx_len = len;
  request = req;
  response = res;
  serv = ser;

  if(serv->state == EXP_READY) {
    EDHOC_TRACE_STATE("EXP_READY", "EXIT");
  }

  if(serv->state != NON_MSG &&
     memcmp(&serv->con_ipaddr, &request->src_ep->ipaddr, sizeof(uip_ipaddr_t)) != 0) {
    LOG_ERR("rx request from an error ipaddr\n");
    coap_set_payload(response, NULL, 0);
    coap_set_status_code(response, BAD_REQUEST_4_00);
    return;
  }

  switch(serv->state) {
  case NON_MSG:
    coap_timer_set_callback(&timer, server_timeout_callback);
    coap_timer_set(&timer, SERV_TIMEOUT_VAL);
    /* fallthrough to handle MSG1 */
  case RX_MSG1:
    if(handle_msg1_state() < 0) {
      break;
    }
    break;

  case RX_MSG3:
    if(handle_msg3_state() < 0) {
      break;
    }
    /* fallthrough to check if ready */
  case EXP_READY:
    handle_exp_ready_state();
    break;

  default:
    LOG_ERR("Unknown server state: %d\n", serv->state);
    serv->state = NON_MSG;
    break;
  }

  setup_coap_response();
}
