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
 *      EDHOC client API [RFC9528] with CoAP Block-Wise Transfer [RFC7959]
 * \author
 *      Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund,
 *      Marco Tiloca, Niclas Finne, and Nicolas Tsiftes
 */

#include "contiki.h"
#include "coap-callback-api.h"
#include "coap-timer.h"
#include "edhoc-client.h"
#include "edhoc-msg-generators.h"
#include "edhoc-msg-handlers.h"
#include "edhoc-trace.h"
#include "lib/memb.h"
#include <assert.h>

#include "sys/log.h"
#define LOG_MODULE "EDHOC"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/*---------------------------------------------------------------------------*/
/* Protocol states (stored in client->state). */
enum edhoc_protocol_state {
  PROTO_IDLE          = 0,
  PROTO_RX_MSG2       = 1,
  PROTO_RX_RSP_MSG3   = 2,
  PROTO_EXP_READY     = 3,
};

/* Event types (stored in edhoc_state.val). */
enum edhoc_event_type {
  EVT_RESTART         = 0,
  EVT_FINISHED        = 1,
  EVT_TIMEOUT         = 2,
  EVT_BLOCK1          = 3,
  EVT_POST            = 4,
  EVT_TRIES_EXPIRE    = 5,
  EVT_BLOCKING        = 6,
};
/*---------------------------------------------------------------------------*/
/* For use of block-wise post and answer */
static coap_callback_request_state_t state;
static uint8_t msg_num;
static size_t send_sz;
static uint8_t *rx_ptr;
static size_t rx_sz;
static edhoc_client_t *client;
static coap_timer_t timer;
static edhoc_data_event_t edhoc_state;
static process_event_t edhoc_event = PROCESS_EVENT_NONE;

static uint8_t attempt = 0;
static int err = 0;
static edhoc_msg_2_t msg2;
/*---------------------------------------------------------------------------*/
PROCESS(edhoc_client, "EDHOC Client");
/*---------------------------------------------------------------------------*/
#if EDHOC_TEST == EDHOC_TEST_VECTOR_TRACE_DH
/* Hard-wired ephemeral keys from RFC 9529 for verifying that all operations yield the same
   intermediate results as the test vectors. */

/* Initiator's ephemeral public key, 'x'-coordinate. G_X */
static const uint8_t eph_pub_x_i[ECC_KEY_LEN] = { 0x8a, 0xf6, 0xf4, 0x30, 0xeb, 0xe1, 0x8d, 0x34, 0x18, 0x40, 0x17, 0xa9, 0xa1, 0x1b, 0xf5, 0x11, 0xc8, 0xdf, 0xf8, 0xf8, 0x34, 0x73, 0x0b,
                                                  0x96, 0xc1, 0xb7, 0xc8, 0xdb, 0xca, 0x2f, 0xc3, 0xb6 };

/* Initiator's ephemeral public key, one 'y'-coordinate. */
static const uint8_t eph_pub_y_i[ECC_KEY_LEN] = { 0x51, 0xe8, 0xaf, 0x6c, 0x6e, 0xdb, 0x78, 0x16, 0x01, 0xad, 0x1d, 0x9c, 0x5f, 0xa8, 0xbf, 0x7a, 0xa1, 0x57, 0x16, 0xc7, 0xc0, 0x6a, 0x5d,
                                                  0x03, 0x85, 0x03, 0xc6, 0x14, 0xff, 0x80, 0xc9, 0xb3 };

/* Initiator's ephemeral private key. X */
static const uint8_t eph_private_i[ECC_KEY_LEN] = { 0x36, 0x8e, 0xc1, 0xf6, 0x9a, 0xeb, 0x65, 0x9b, 0xa3, 0x7d, 0x5a, 0x8d, 0x45, 0xb2, 0x1b, 0xdc, 0x02, 0x99, 0xdc, 0xea, 0xa8, 0xef, 0x23,
                                                    0x5f, 0x3c, 0xa4, 0x2c, 0xe3, 0x53, 0x0f, 0x95, 0x25 };
#endif
/*---------------------------------------------------------------------------*/
int8_t
edhoc_client_callback(process_event_t ev, void *data)
{
  if(ev == edhoc_event && data == &edhoc_state) {
    LOG_DBG("client callback: ev=%d, state=%d\n", ev, edhoc_state.val);
    if(edhoc_state.val == EVT_FINISHED) {
      LOG_INFO("client callback: CL_FINISHED\n");
      return 1;
    }
    if(edhoc_state.val == EVT_TRIES_EXPIRE) {
      LOG_WARN("client callback: CL_TRIES_EXPIRE\n");
      return -1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
edhoc_client_run(void)
{
  process_start(&edhoc_client, NULL);
}
/*---------------------------------------------------------------------------*/
void
edhoc_client_set_ad_1(const void *data_buffer, uint8_t buffer_size)
{
  if(buffer_size > EDHOC_MAX_AD_SZ) {
    LOG_ERR("AD_1 size (%d) exceeds maximum allowed (%d)\n", buffer_size, EDHOC_MAX_AD_SZ);
    return;
  }
  if(data_buffer == NULL) {
    LOG_ERR("Invalid buffer pointer for AD_1\n");
    return;
  }
  memcpy(edhoc_state.ad.ad_1, data_buffer, buffer_size);
  edhoc_state.ad.ad_1_sz = buffer_size;
}
/*---------------------------------------------------------------------------*/
void
edhoc_client_set_ad_3(const void *data_buffer, uint8_t buffer_size)
{
  if(buffer_size > EDHOC_MAX_AD_SZ) {
    LOG_ERR("AD_3 size (%d) exceeds maximum allowed (%d)\n", buffer_size, EDHOC_MAX_AD_SZ);
    return;
  }
  if(data_buffer == NULL) {
    LOG_ERR("Invalid buffer pointer for AD_3\n");
    return;
  }
  memcpy(edhoc_state.ad.ad_3, data_buffer, buffer_size);
  edhoc_state.ad.ad_3_sz = buffer_size;
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_client_get_ad_2(char *buf, size_t buf_sz)
{
  if(buf == NULL) {
    LOG_ERR("Invalid buffer pointer for getting AD_2\n");
    return 0;
  }
  if(edhoc_state.ad.ad_2_sz > buf_sz) {
    LOG_ERR("Destination buffer size (%zu) too small for AD_2 (%d)\n", buf_sz, edhoc_state.ad.ad_2_sz);
    return 0;
  }
  memcpy(buf, edhoc_state.ad.ad_2, edhoc_state.ad.ad_2_sz);
  return edhoc_state.ad.ad_2_sz;
}
/*---------------------------------------------------------------------------*/
static void
client_timeout_callback(coap_timer_t *timer)
{
  LOG_ERR("EDHOC client timeout: no response received\n");
  coap_timer_stop(timer);
  edhoc_state.val = EVT_TIMEOUT;
  process_post(&edhoc_client, edhoc_event, &edhoc_state);
}
/*---------------------------------------------------------------------------*/
MEMB(edhoc_client_storage, edhoc_client_t, 1);
/*---------------------------------------------------------------------------*/
static int handle_exp_ready(edhoc_client_t *client);
/*---------------------------------------------------------------------------*/
static inline edhoc_client_t *
client_context_new(void)
{
  edhoc_client_t *c = memb_alloc(&edhoc_client_storage);
  if(c) {
    memset(c, 0, sizeof(edhoc_client_t));
  }
  return c;
}
/*---------------------------------------------------------------------------*/
static inline void
client_context_free(edhoc_client_t *c)
{
  memset(c, 0, sizeof(edhoc_client_t));
  memb_free(&edhoc_client_storage, c);
}
/*---------------------------------------------------------------------------*/
static int
client_block2_handler(coap_message_t *response, uint8_t *target,
                      size_t *len, size_t max_len)
{
  const uint8_t *payload = NULL;
  int pay_len = coap_get_payload(response, &payload);

  if(response->block2_offset + pay_len > max_len) {
    LOG_ERR("EDHOC message size (%d) exceeds max buffer (%d)\n", (int)pay_len, EDHOC_MAX_BUFFER);
    coap_status_code = REQUEST_ENTITY_TOO_LARGE_4_13;
    coap_error_message = "Message too big";
    return -1;
  }

  if(target && len) {
    memcpy(target + response->block2_offset, payload, pay_len);
    *len = response->block2_offset + pay_len;
    LOG_DBG_BYTES((uint8_t *)payload, (unsigned long)pay_len);
    LOG_DBG_("\n");
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static bool
validate_response(coap_callback_request_state_t *callback_state)
{
  if(callback_state->state.response == NULL) {
    LOG_WARN("Request timed out\n");
    return false;
  }

  if(memcmp(callback_state->state.request->token,
            callback_state->state.response->token,
            callback_state->state.request->token_len) != 0) {
    LOG_ERR("Response token mismatch\n");
  } else if(memcmp(&client->server_ep.ipaddr,
                    &callback_state->state.remote_endpoint->ipaddr,
                    sizeof(uip_ipaddr_t)) != 0) {
    LOG_ERR("Response from unexpected server\n");
  } else {
    return true;
  }

  edhoc_state.val = EVT_RESTART;
  coap_timer_stop(&timer);
  process_post(&edhoc_client, edhoc_event, &edhoc_state);
  return false;
}
/*---------------------------------------------------------------------------*/
static void
client_response_handler(coap_callback_request_state_t *callback_state)
{
  if(!validate_response(callback_state)) {
    return;
  }

  if(callback_state->state.response->code != CHANGED_2_04) {
    LOG_WARN("Response code is not CHANGED_2_04\n");
  }

  coap_set_option(callback_state->state.response, COAP_OPTION_BLOCK2);

  client_block2_handler(callback_state->state.response,
                        rx_ptr, &rx_sz, EDHOC_MAX_PAYLOAD_LEN);
  if(!callback_state->state.more) {
    edhoc_ctx->buffers.rx_sz = rx_sz;
    edhoc_state.val = EVT_BLOCKING;
    process_post(PROCESS_BROADCAST, edhoc_event, &edhoc_state);
  }
}
/*---------------------------------------------------------------------------*/
static void
client_chunk_handler(coap_callback_request_state_t *callback_state)
{
  if(!validate_response(callback_state)) {
    return;
  }

  edhoc_state.val = EVT_BLOCK1;
  process_post(&edhoc_client, edhoc_event, &edhoc_state);
}
/*---------------------------------------------------------------------------*/
static void
edhoc_client_post(void)
{
  coap_init_message(state.state.request, COAP_TYPE_CON, COAP_POST, 0);
  coap_set_header_uri_path(state.state.request, EDHOC_COAP_URI_PATH);

  send_sz = 0;
  msg_num = 0;
  state.state.block_num = 0;
}
/*---------------------------------------------------------------------------*/
static int
edhoc_client_post_blocks(void)
{
  if(edhoc_ctx->buffers.tx_sz - send_sz > COAP_MAX_CHUNK_SIZE) {
    coap_set_payload(state.state.request,
                     (uint8_t *)edhoc_ctx->buffers.msg_tx + send_sz,
                     COAP_MAX_CHUNK_SIZE);
    coap_set_header_block1(state.state.request, msg_num,
                           1, COAP_MAX_CHUNK_SIZE);
    msg_num++;
    send_sz += COAP_MAX_CHUNK_SIZE;
    coap_send_request(&state, state.state.remote_endpoint,
                      state.state.request, client_chunk_handler);
    return 0;
  } else if(edhoc_ctx->buffers.tx_sz < COAP_MAX_CHUNK_SIZE) {
    coap_set_payload(state.state.request,
                     (uint8_t *)edhoc_ctx->buffers.msg_tx,
                     edhoc_ctx->buffers.tx_sz);
    rx_ptr = edhoc_ctx->buffers.msg_rx;
    rx_sz = 0;
    state.state.block_num = 0;
    coap_send_request(&state, state.state.remote_endpoint,
                      state.state.request, client_response_handler);
    return 1;
  } else {
    coap_set_payload(state.state.request,
                     (uint8_t *)edhoc_ctx->buffers.msg_tx + send_sz,
                     edhoc_ctx->buffers.tx_sz - send_sz);
    coap_set_header_block1(state.state.request, msg_num, 0,
                           COAP_MAX_CHUNK_SIZE);
    send_sz += (edhoc_ctx->buffers.tx_sz - send_sz);
    rx_ptr = edhoc_ctx->buffers.msg_rx;
    rx_sz = 0;
    coap_send_request(&state, state.state.remote_endpoint,
                      state.state.request, client_response_handler);
    return 1;
  }
}
/*---------------------------------------------------------------------------*/
static int
generate_msg3_response(void)
{
  EDHOC_TRACE_STEP("message_3 generation");
  edhoc_error_t result = edhoc_generate_message_3(edhoc_ctx, (uint8_t *)edhoc_state.ad.ad_3,
                                                   edhoc_state.ad.ad_3_sz);
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to generate MSG3: %s\n", edhoc_error_string(result));
    return -1;
  }
  edhoc_trace_message(3, edhoc_ctx->buffers.msg_tx, edhoc_ctx->buffers.tx_sz, true);
  return 0;
}

static void
handle_msg2_error(int err)
{
  LOG_ERR("Client: Send MSG error with code (%d)\n", err);
  edhoc_ctx->buffers.tx_sz = edhoc_generate_error_message(edhoc_ctx->buffers.msg_tx,
                                                           EDHOC_MAX_PAYLOAD_LEN,
                                                           edhoc_ctx, err);
  if(edhoc_ctx->buffers.tx_sz == 0) {
    LOG_ERR("Failed to generate error message\n");
  }
  client->state = PROTO_IDLE;
  edhoc_client_post();
  edhoc_client_post_blocks();
}

/*---------------------------------------------------------------------------*/
static int
edhoc_send_msg1(uint8_t *ad, uint8_t ad_sz, bool suite_array)
{
  EDHOC_TRACE_STEP("message_1 generation");
  edhoc_error_t result = edhoc_generate_message_1(edhoc_ctx, ad, ad_sz, suite_array);
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to generate MSG1: %s\n", edhoc_error_string(result));
    return -1;
  }
  edhoc_client_post();
  client->state = PROTO_RX_MSG2;
  return edhoc_client_post_blocks();
}
/*---------------------------------------------------------------------------*/
/* Protocol state handlers */
static int
handle_rx_msg2(edhoc_client_t *client, edhoc_msg_2_t *msg2)
{
  edhoc_trace_message(2, edhoc_ctx->buffers.msg_rx, edhoc_ctx->buffers.rx_sz, false);

  err = edhoc_handler_msg_2(msg2, edhoc_ctx, edhoc_ctx->buffers.msg_rx,
                            edhoc_ctx->buffers.rx_sz);

  if(err == EDHOC_ERR_SEQUENCE_ERROR) {
    edhoc_send_msg1((uint8_t *)edhoc_state.ad.ad_1, edhoc_state.ad.ad_1_sz, true);
    return 0;
  }

  if(err > 0) {
    assert(msg2->gy_ciphertext_2_sz >= ECC_KEY_LEN);
    assert(msg2->gy_ciphertext_2_sz - ECC_KEY_LEN <= EDHOC_MAX_BUFFER);
    err = edhoc_authenticate_msg(edhoc_ctx, (uint8_t *)edhoc_state.ad.ad_2, true);
  }

  if(err == EDHOC_ERR_MSG_MALFORMED) {
    LOG_ERR("error code (%d)\n", err);
    return -1;
  } else if(err < EDHOC_ERR_MSG_MALFORMED) {
    handle_msg2_error(err);
    return -1;
  }

  /* Handle AD_2 */
  edhoc_state.ad.ad_2_sz = err;
  if(edhoc_state.ad.ad_2_sz > 0) {
    EDHOC_DBG_VALUE("AD_2", edhoc_state.ad.ad_2, edhoc_state.ad.ad_2_sz);
  }

  /* Generate MSG3 */
  if(generate_msg3_response() < 0) {
    client->state = PROTO_IDLE;
    return -1;
  }

  client->rx_msg2 = true;
  client->state = PROTO_RX_RSP_MSG3;
  client->tx_msg3 = true;
  edhoc_client_post();
  edhoc_client_post_blocks();
  return 0;
}

static int
handle_rx_response_msg3(edhoc_client_t *client)
{
  if(edhoc_ctx->buffers.rx_sz > 0) {
    edhoc_error_t error = edhoc_check_err_rx_msg(3, edhoc_ctx->buffers.msg_rx,
                                                  edhoc_ctx->buffers.rx_sz);
    if(error != EDHOC_SUCCESS) {
      edhoc_state.val = EVT_RESTART;
      client->state = PROTO_IDLE;
      coap_timer_stop(&timer);
      process_post(&edhoc_client, edhoc_event, &edhoc_state);
      return -1;
    }
  }

  /* Check every protocol step successfully */
  if(client->tx_msg1 && client->rx_msg2) {
    client->rx_msg3_response = true;
  } else {
    LOG_ERR("The EDHOC process escape steps\n");
    edhoc_state.val = EVT_RESTART;
    coap_timer_stop(&timer);
    process_post(&edhoc_client, edhoc_event, &edhoc_state);
    return -1;
  }

  /* Fall through to PROTO_EXP_READY - execute it immediately */
  client->state = PROTO_EXP_READY;
  return handle_exp_ready(client);
}

static int
handle_exp_ready(edhoc_client_t *client)
{
  EDHOC_TRACE_STEP("key export ready");
  edhoc_trace_session_summary(edhoc_ctx);
  edhoc_state.val = EVT_FINISHED;
  coap_timer_stop(&timer);
  process_post(PROCESS_BROADCAST, edhoc_event, &edhoc_state);
  return 0;
}

/*---------------------------------------------------------------------------*/
static void
edhoc_client_protocol_run(void)
{
  switch(client->state) {
  case PROTO_RX_MSG2:
    handle_rx_msg2(client, &msg2);
    break;
  case PROTO_RX_RSP_MSG3:
    handle_rx_response_msg3(client);
    break;
  case PROTO_EXP_READY:
    handle_exp_ready(client);
    break;
  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
static void
edhoc_client_init(void)
{
  if(edhoc_event == PROCESS_EVENT_NONE) {
    edhoc_event = process_alloc_event();
  }
  client = client_context_new();
  if(client == NULL) {
    LOG_ERR("Failed to allocate client context\n");
    return;
  }
  edhoc_storage_init();
  edhoc_ctx = edhoc_new();
  if(edhoc_ctx == NULL) {
    LOG_ERR("Failed to create EDHOC context\n");
    return;
  }
  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &client->server_ep);
  state.state.request = client->request;
  state.state.response = client->response;
  state.state.remote_endpoint = &client->server_ep;
}
/*---------------------------------------------------------------------------*/
static int
edhoc_client_start(uint8_t *ad, uint8_t ad_sz)
{
  client->tx_msg3 = false;
  client->rx_msg3_response = false;
  client->tx_msg1 = true;

  coap_timer_set_callback(&timer, client_timeout_callback);
  coap_timer_set(&timer, CL_TIMEOUT_VAL);

  return edhoc_send_msg1(ad, ad_sz, false);
}
/*---------------------------------------------------------------------------*/
static void
generate_ephemeral_key(uint8_t curve_id, uint8_t *pub_x,
                       uint8_t *pub_y, uint8_t *priv)
{
  if(!ecdh_generate_keypair(curve_id, pub_x, pub_y, priv)) {
    LOG_ERR("Failed to generate ephemeral key pair\n");
    return;
  }

#if EDHOC_TEST == EDHOC_TEST_VECTOR_TRACE_DH
  memcpy(edhoc_ctx->creds.ephemeral_key.pub.x, eph_pub_x_i, ECC_KEY_LEN);
  memcpy(edhoc_ctx->creds.ephemeral_key.pub.y, eph_pub_y_i, ECC_KEY_LEN);
  memcpy(edhoc_ctx->creds.ephemeral_key.priv, eph_private_i, ECC_KEY_LEN);
#endif

  edhoc_trace_ephemeral_key("Initiator",
                            edhoc_ctx->creds.ephemeral_key.pub.x,
                            edhoc_ctx->creds.ephemeral_key.pub.y,
                            edhoc_ctx->creds.ephemeral_key.priv);
}
/*---------------------------------------------------------------------------*/
/* Unified retry handler */
static void
handle_retry(struct etimer *wait_timer, uint8_t *attempt)
{
  if(*attempt < EDHOC_CONF_ATTEMPTS) {
    LOG_INFO("Attempt %d\n", *attempt);
    etimer_set(wait_timer, CLOCK_SECOND * (CL_TIMEOUT_VAL / 1000));
    (*attempt)++;
  } else {
    LOG_ERR("Expire EDHOC client attempts\n");
    edhoc_state.val = EVT_TRIES_EXPIRE;
    process_post(PROCESS_BROADCAST, edhoc_event, &edhoc_state);
  }
}


/*---------------------------------------------------------------------------*/
void
edhoc_client_close(void)
{
  coap_timer_stop(&timer);
  client_context_free(client);
  edhoc_finalize(edhoc_ctx);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(edhoc_client, ev, data)
{
  PROCESS_BEGIN();

  static struct etimer wait_timer;
  edhoc_client_init();

  if(!edhoc_initialize_context(edhoc_ctx)) {
    PROCESS_EXIT();
  }

  /* Generate ephemeral key */
  generate_ephemeral_key(edhoc_ctx->config.ecdh_curve,
                         edhoc_ctx->creds.ephemeral_key.pub.x,
                         edhoc_ctx->creds.ephemeral_key.pub.y,
                         edhoc_ctx->creds.ephemeral_key.priv);

  edhoc_client_start((uint8_t *)edhoc_state.ad.ad_1, edhoc_state.ad.ad_1_sz);

  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == edhoc_event && data == &edhoc_state) {
      if(edhoc_state.val == EVT_RESTART || edhoc_state.val == EVT_TIMEOUT) {
        LOG_ERR("%s\n", edhoc_state.val == EVT_RESTART ? "Error" : "Expire timeout");
        handle_retry(&wait_timer, &attempt);
        if(attempt < EDHOC_CONF_ATTEMPTS) {
          PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
          etimer_stop(&wait_timer);
          edhoc_client_start((uint8_t *)edhoc_state.ad.ad_1,
                             edhoc_state.ad.ad_1_sz);
        } else {
          break;
        }
      } else if(edhoc_state.val == EVT_FINISHED) {
        LOG_INFO("Client has finished\n");
        break;
      } else if(edhoc_state.val == EVT_BLOCK1) {
        edhoc_client_post_blocks();
      } else if(edhoc_state.val == EVT_POST) {
        edhoc_client_post();
        edhoc_client_post_blocks();
      } else if(edhoc_state.val == EVT_BLOCKING) {
        edhoc_client_protocol_run();
      } else if(edhoc_state.val == EVT_TRIES_EXPIRE) {
        break;
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
