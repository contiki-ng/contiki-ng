/*
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
 */

/**
 * \file
 *      EDHOC client API [draft-ietf-lake-edhoc-01] with CoAP Block-Wise Transfer [RFC7959]
 * \author
 *      Lidia Pocero <pocero@isi.gr>
 */

#include "edhoc-client-API.h"
#include "lib/memb.h"
#include "contiki-lib.h"
#include "sys/timer.h"
#include "sys/rtimer.h"

/* EDHOC Client protocol states */
#define NON_MSG 0
#define RX_MSG2 4
#define RX_RESPONSE_MSG3 5
#define EXP_READY 6

/* EDHOC process states */
#define CL_TIMEOUT 2
#define CL_RESTART 0
#define CL_FINISHED 1
#define CL_BLOCK1 3
#define CL_POST 4
#define CL_TRIES_EXPIRE 5
#define CL_PROTOCOL 6
#define CL_BLOCKING 7

/*For use of block-wise post and answer */
static coap_callback_request_state_t state;
static uint8_t msg_num;
static uint8_t send_sz;
static uint8_t *rx_ptr;
static size_t rx_sz;
static int pro;
static edhoc_client_t *cli;
static coap_timer_t timer;
static edhoc_data_even_t edhoc_state;
static process_event_t edhoc_event;

static rtimer_clock_t time;
static rtimer_clock_t time_total;
static uint8_t attempt = 0;
static int er = 0;
static cose_key_t key;
static uint8_t *pt = NULL;
static edhoc_msg_2 msg2;
PROCESS(edhoc_client, "Edhoc Client");
PROCESS(edhoc_client_protocol, "Edhoc Client Protocol");

#if TEST == TEST_VECTOR
uint8_t eph_pub_x_i[ECC_KEY_BYTE_LENGHT] = { 0x47, 0x57, 0x76, 0xf8, 0x44, 0x97, 0x9a, 0xd0, 0xb4, 0x63, 0xc5, 0xa6, 0xa4, 0x34, 0x3a, 0x66, 0x3d, 0x17, 0xa3, 0xa8, 0x0e, 0x38, 0xa8, 0x1d,
                                             0x3e, 0x34, 0x96, 0xf6, 0x06, 0x1f, 0xd7, 0x16 };
uint8_t eph_pub_y_i[ECC_KEY_BYTE_LENGHT] = { 0x7b, 0x21, 0x4f, 0x54, 0xa2, 0x3e, 0xd2, 0x05, 0x39, 0x5b, 0x6a, 0xe3, 0xb6, 0xa7, 0x4f, 0xcf, 0xe9, 0xb3, 0xe4, 0x0a, 0xcf, 0x89, 0xf5, 0x24,
                                             0xd4, 0xa0, 0x6e, 0x0c, 0x8d, 0x91, 0xfb, 0x69 };
uint8_t eph_private_i[ECC_KEY_BYTE_LENGHT] = { 0x0a, 0xe7, 0x99, 0x77, 0x5c, 0xb1, 0x51, 0xbf, 0xc2, 0x54, 0x87, 0x35, 0xf4, 0x4a, 0xcf, 0x1d, 0x94, 0x29, 0xcf, 0x9a, 0x95, 0xdd, 0xcd, 0x2a,
                                               0x13, 0x9e, 0x3a, 0x28, 0xd8, 0x63, 0xa0, 0x81 };
#endif

int8_t
edhoc_client_callback(process_event_t ev, void *data)
{
  if((ev == edhoc_event) && (edhoc_state.val == CL_FINISHED)) {
    LOG_DBG("client callback: CL_FINISHED\n");
    return 1;
  }
  if((ev == edhoc_event) && (edhoc_state.val == CL_TRIES_EXPIRE)) {
    LOG_WARN("client callback: CL_TRIES_EXPIRE\n");
    return -1;
  }
  return 0;
}
void
edhoc_client_run()
{
  process_start(&edhoc_client, NULL);
}
void
edhoc_server_set_ad_1(const void *buff, uint8_t buff_sz)
{
  memcpy(edhoc_state.ad.ad_1, (void *)buff, buff_sz);
  edhoc_state.ad.ad_1_sz = buff_sz;
}
void
edhoc_server_set_ad_3(const void *buff, uint8_t buff_sz)
{
  memcpy(edhoc_state.ad.ad_3, (void *)buff, buff_sz);
  edhoc_state.ad.ad_3_sz = buff_sz;
}
uint8_t
edhoc_server_get_ad_2(char *buff)
{
  memcpy(buff, (void *)edhoc_state.ad.ad_2, edhoc_state.ad.ad_2_sz);
  return edhoc_state.ad.ad_2_sz;
}
static void
client_timeout_callback(coap_timer_t *timer)
{
  LOG_ERR("Timeout\n");
  coap_timer_stop(timer);
  edhoc_state.val = CL_TIMEOUT;
  pro = process_post(&edhoc_client, edhoc_event, &edhoc_state);
}
MEMB(edhoc_client_storage, edhoc_client_t, 1);

static inline edhoc_client_t *
client_context_new()
{
  return (edhoc_client_t *)memb_alloc(&edhoc_client_storage);
}
static inline void
client_context_free(edhoc_client_t *ctx)
{
  memb_free(&edhoc_client_storage, ctx);
}
static edhoc_client_t *
client_new()
{
  edhoc_client_t *cli;
  cli = client_context_new();
  return cli;
}
static int
client_block2_handler(coap_message_t *response,
                      uint8_t *target, size_t *len, size_t max_len)
{
  const uint8_t *payload = 0;
  int pay_len = coap_get_payload(response, &payload);

  if(response->block2_offset + pay_len > max_len) {
    LOG_ERR("message to big\n");
    coap_status_code = REQUEST_ENTITY_TOO_LARGE_4_13;
    coap_error_message = "Message to big";
    return -1;
  }

  if(target && len) {
    memcpy(target + response->block2_offset, payload, pay_len);
    *len = response->block2_offset + pay_len;
    print_buff_8_dbg((uint8_t *)payload, (unsigned long)pay_len);
    target = target + pay_len;
  }
  return 0;
}
static void
client_response_handler(coap_callback_request_state_t *callback_state)
{
  if(callback_state->state.response == NULL) {
    LOG_WARN("Request timed out response\n");
    return;
  }
  if(memcmp(callback_state->state.request->token, callback_state->state.response->token, callback_state->state.request->token_len)) {
    LOG_ERR("rx response not correlated\n");
    edhoc_state.val = CL_RESTART;
    coap_timer_stop(&timer);
    pro = process_post(&edhoc_client, edhoc_event, &edhoc_state);
    return;
  }
  /*Check that the response are comming from the correct server */
  if(memcmp(&cli->server_ep.ipaddr, &callback_state->state.remote_endpoint->ipaddr, sizeof(uip_ipaddr_t)) != 0) {
    LOG_ERR("rx response from an error server\n");
    edhoc_state.val = CL_RESTART;
    coap_timer_stop(&timer);
    pro = process_post(&edhoc_client, edhoc_event, &edhoc_state);
    return;
  }

  if((callback_state->state.response->code != CHANGED_2_04)) {
    LOG_WARN("The code responds received is not CHANGED_2_04\n");
  }

  coap_set_option(callback_state->state.response, COAP_OPTION_BLOCK2);

  LOG_DBG("Blockwise: block 2 response: Num: %" PRIu32
          ", More: %u, Size: %u, Offset: %" PRIu32 "\n",
          callback_state->state.response->block2_num,
          callback_state->state.response->block2_more,
          callback_state->state.response->block2_size,
          callback_state->state.response->block2_offset);
  LOG_DBG("Blockwise: block 1 response: Num: %" PRIu32
          ", More: %u, Size: %u, Offset: %" PRIu32 "\n",
          callback_state->state.response->block1_num,
          callback_state->state.response->block1_more,
          callback_state->state.response->block1_size,
          callback_state->state.response->block1_offset);
  if(callback_state->state.more) {
    client_block2_handler(callback_state->state.response, rx_ptr, &rx_sz, MAX_DATA_LEN);
  } else {
    client_block2_handler(callback_state->state.response, rx_ptr, &rx_sz, MAX_DATA_LEN);
    ctx->rx_sz = (uint8_t)rx_sz;
    edhoc_state.val = CL_BLOCKING;
    pro = process_post(PROCESS_BROADCAST, edhoc_event, &edhoc_state);
  }
}
static void
client_chunk_handler(coap_callback_request_state_t *callback_state)
{

  if(callback_state->state.response == NULL) {
    LOG_WARN("Request timed out chunk\n");
    return;
  }
  /*Check the 5-tuple information before retrive the state protocol*/

  LOG_DBG("Blockwise: block 2 response: Num: %" PRIu32
          ", More: %u, Size: %u, Offset: %" PRIu32 "\n",
          callback_state->state.response->block2_num,
          callback_state->state.response->block2_more,
          callback_state->state.response->block2_size,
          callback_state->state.response->block2_offset);
  LOG_DBG("Blockwise: block 1 response: Num: %" PRIu32
          ", More: %u, Size: %u, Offset: %" PRIu32 "\n",
          callback_state->state.response->block1_num,
          callback_state->state.response->block1_more,
          callback_state->state.response->block1_size,
          callback_state->state.response->block1_offset);
  edhoc_state.val = CL_BLOCK1;
  pro = process_post(&edhoc_client, edhoc_event, &edhoc_state);
}
static void
edhoc_client_post()
{
  coap_init_message(state.state.request, COAP_TYPE_CON, COAP_POST, 0);
  coap_set_header_uri_path(state.state.request, WELL_KNOWN);
  send_sz = 0;
  msg_num = 0;
  state.state.block_num = 0;
}
static int
edhoc_client_post_blocks()
{
  if((ctx->tx_sz - send_sz) > COAP_MAX_CHUNK_SIZE) {
    coap_set_payload(state.state.request, (uint8_t *)ctx->msg_tx + send_sz, COAP_MAX_CHUNK_SIZE);
    coap_set_header_block1(state.state.request, msg_num, 1, COAP_MAX_CHUNK_SIZE);
    msg_num++;
    send_sz += COAP_MAX_CHUNK_SIZE;
    coap_send_request(&state, state.state.remote_endpoint, state.state.request, client_chunk_handler);
    return 0;
  } else if(ctx->tx_sz < COAP_MAX_CHUNK_SIZE) {
    coap_set_payload(state.state.request, (uint8_t *)ctx->msg_tx, ctx->tx_sz);
    rx_ptr = ctx->msg_rx;
    rx_sz = 0;
    state.state.block_num = 0;
    coap_send_request(&state, state.state.remote_endpoint, state.state.request, client_response_handler);
    return 1;
  } else {
    coap_set_payload(state.state.request, (uint8_t *)ctx->msg_tx + send_sz, ctx->tx_sz - send_sz);
    coap_set_header_block1(state.state.request, msg_num, 0, COAP_MAX_CHUNK_SIZE);
    send_sz += (ctx->tx_sz - send_sz);
    rx_ptr = ctx->msg_rx;
    rx_sz = 0;
    coap_send_request(&state, state.state.remote_endpoint, state.state.request, client_response_handler);
    return 1;
  }
}
static int 
edhoc_send_msg1(uint8_t *ad, uint8_t ad_sz, bool suit_array){
  LOG_DBG("--------------Generate message_1------------------\n");
  time = RTIMER_NOW();
  edhoc_gen_msg_1(ctx, ad, ad_sz, suit_array);
  time = RTIMER_NOW() - time;
  LOG_INFO("Client time to gen MSG1: %" PRIu32 " ms (%" PRIu32 " CPU cycles ).\n", (uint32_t)((uint64_t)time * 1000 / RTIMER_SECOND), (uint32_t)time);
  time = RTIMER_NOW();
  edhoc_client_post(&cli->server_ep, state.state.request, ctx->msg_tx, ctx->tx_sz);
  cli->state = RX_MSG2;
  return edhoc_client_post_blocks();  
}
PROCESS_THREAD(edhoc_client_protocol, ev, data)
{
  PROCESS_BEGIN();
  switch(cli->state) {
  case RX_MSG2:
    LOG_DBG("--------------Handler message_2------------------\n");
    LOG_DBG("RX message_2 (%d bytes):", ctx->rx_sz);
    print_buff_8_dbg(ctx->msg_rx, ctx->rx_sz);

    time = RTIMER_NOW();
    er = edhoc_handler_msg_2(&msg2, ctx, ctx->msg_rx, ctx->rx_sz);
    time = RTIMER_NOW() - time;
    LOG_INFO("Client time to handler MSG2: %" PRIu32 " ms (%" PRIu32 " CPU cycles ).\n", (uint32_t)((uint64_t)time * 1000 / RTIMER_SECOND), (uint32_t)time);
    time = RTIMER_NOW();
    if(er == ERR_RESEND_MSG_1){
       edhoc_send_msg1((uint8_t*) edhoc_state.ad.ad_1, edhoc_state.ad.ad_1_sz, true);
       break;
    }
    else if(er > 0) {
      er = edhoc_get_auth_key(ctx, &pt, &key);
    }

    if(er > 0) {
      er = edhoc_authenticate_msg(ctx, &pt, msg2.cipher.len, (uint8_t *)edhoc_state.ad.ad_2, &key);
    }
    time = RTIMER_NOW() - time;
    LOG_DBG("Client time to authneticate MSG2: %" PRIu32 " ms (%" PRIu32 " CPU cycles ).\n", (uint32_t)((uint64_t)time * 1000 / RTIMER_SECOND), (uint32_t)time);

    if(er == RX_ERR_MSG) {
      LOG_ERR("error code (%d)\n", er);
    } else if(er < RX_ERR_MSG) {
      LOG_ERR("Client: Send MSG error with code (%d)\n", er);
      ctx->tx_sz = edhoc_gen_msg_error(ctx->msg_tx, ctx, er);
      cli->state = NON_MSG;
      edhoc_client_post();
      edhoc_client_post_blocks();
    } else {
      /*TODO: Include a way to pass aplictaion msgs. */
      edhoc_state.ad.ad_2_sz = er;
      if(edhoc_state.ad.ad_2_sz > 0 && edhoc_state.ad.ad_2) {
        LOG_DBG("Ap_2 (%d bytes):", edhoc_state.ad.ad_2_sz);
        print_char_8_dbg((char *)edhoc_state.ad.ad_2, edhoc_state.ad.ad_2_sz);
      }
      LOG_DBG("-----------------gen MSG3---------------------\n");
      /*Generate MSG3 */
      time = RTIMER_NOW();
      edhoc_gen_msg_3(ctx, (uint8_t *)edhoc_state.ad.ad_3, edhoc_state.ad.ad_3_sz);
      time = RTIMER_NOW() - time;
      LOG_INFO("Client time to gen MSG3: %" PRIu32 " ms (%" PRIu32 " CPU cycles ).\n", (uint32_t)((uint64_t)time * 1000 / RTIMER_SECOND), (uint32_t)time);
      LOG_DBG("message_3 (%d bytes):", ctx->tx_sz);
      print_buff_8_dbg(ctx->msg_tx, ctx->tx_sz);
      cli->rx_msg2 = true;
      cli->state = RX_RESPONSE_MSG3;
      cli->tx_msg3 = true;
      edhoc_client_post();
      edhoc_client_post_blocks();
    }
    break;
  case RX_RESPONSE_MSG3:
    if(ctx->rx_sz > 0) {
      uint8_t *msg_err = ctx->msg_rx;
      edhoc_msg_error err;
      er = edhoc_deserialize_err(&err, msg_err, ctx->rx_sz);
      if(er > 0) {
        LOG_ERR("RX MSG_ERR:");
        print_char_8_err(err.err.buf, err.err.len);
        edhoc_state.val = CL_RESTART;
        cli->state = NON_MSG;
        coap_timer_stop(&timer);
        pro = process_post(&edhoc_client, edhoc_event, &edhoc_state);
        break;
      }
    }
    /*Check every protocol step successfully */
    cli->state = EXP_READY;
    if(cli->tx_msg1 && cli->rx_msg2) {
      cli->rx_msg3_response = true;
    } else {
      LOG_ERR("The edhoc process scape steps\n");
      edhoc_state.val = CL_RESTART;
      coap_timer_stop(&timer);
      pro = process_post(&edhoc_client, edhoc_event, &edhoc_state);
      break;
    }
  case EXP_READY:
    LOG_DBG("-------------------EXPORTER-----------------------\n");
    edhoc_state.val = CL_FINISHED;
    coap_timer_stop(&timer);
    pro = process_post(PROCESS_BROADCAST, edhoc_event, &edhoc_state);
    break;
  }
  PROCESS_END();
}

static void
edhoc_client_init()
{
  cli = client_new();
  edhoc_storage_init();
  ctx = edhoc_new();
  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &cli->server_ep);
  state.state.request = cli->request;
  state.state.response = cli->response;
  state.state.remote_endpoint = &cli->server_ep;
}

static int
edhoc_client_start(uint8_t *ad, uint8_t ad_sz)
{
  cli->tx_msg3 = false;
  cli->rx_msg3_response = false;
  cli->tx_msg1 = true;

  coap_timer_set_callback(&timer, client_timeout_callback);
  coap_timer_set(&timer, CL_TIMEOUT_VAL);
  
  return edhoc_send_msg1(ad,ad_sz,false);
}
void
edhoc_client_close()
{
  coap_timer_stop(&timer);
  client_context_free(cli);
  edhoc_finalize(ctx);
}
PROCESS_THREAD(edhoc_client, ev, data)
{
  PROCESS_BEGIN();
  static struct etimer wait_timer;
  edhoc_client_init();
  time = RTIMER_NOW();
#if TEST == TEST_VECTOR
  LOG_DBG("Using test vector\n");
  memcpy(ctx->ephimeral_key.public.x, eph_pub_x_i, ECC_KEY_BYTE_LENGHT);
  memcpy(ctx->ephimeral_key.public.y, eph_pub_y_i, ECC_KEY_BYTE_LENGHT);
  memcpy(ctx->ephimeral_key.private_key, eph_private_i, ECC_KEY_BYTE_LENGHT);
#if ECC == UECC_ECC
  LOG_DBG("setr curve of uecc\n");
  ctx->curve.curve = uECC_secp256r1();
#endif
#elif ECC == UECC_ECC
  LOG_DBG("generate key with uecc\n");
  ctx->curve.curve = uECC_secp256r1();
  uecc_generate_key(&ctx->ephimeral_key, ctx->curve);
#elif ECC == CC2538_ECC
  LOG_DBG("generate key with CC2538\n");
  static key_gen_t key = {
    .process = &edhoc_client,
    .curve_info = &nist_p_256,
  };
  PT_SPAWN(&edhoc_client.pt, &key.pt, generate_key_hw(&key));

  memcpy(ctx->ephimeral_key.public.x, key.x, ECC_KEY_BYTE_LENGHT);
  memcpy(ctx->ephimeral_key.public.y, key.y, ECC_KEY_BYTE_LENGHT);
  memcpy(ctx->ephimeral_key.private_key, key.private, ECC_KEY_BYTE_LENGHT);
#endif

  LOG_DBG("X (%d bytes):", ECC_KEY_BYTE_LENGHT);
  print_buff_8_dbg(ctx->ephimeral_key.private_key, ECC_KEY_BYTE_LENGHT);
  LOG_DBG("G_X x (%d bytes):", ECC_KEY_BYTE_LENGHT);
  print_buff_8_dbg(ctx->ephimeral_key.public.x, ECC_KEY_BYTE_LENGHT);
  LOG_DBG("y:");
  print_buff_8_dbg(ctx->ephimeral_key.public.y, ECC_KEY_BYTE_LENGHT);

  time_total = RTIMER_NOW();
  time = RTIMER_NOW() - time;
  LOG_INFO("Client time to gen eph key: %" PRIu32 " ms (%" PRIu32 " CPU cycles ).\n", (uint32_t)((uint64_t)time * 1000 / RTIMER_SECOND), (uint32_t)time);
  if(!edhoc_get_authentication_key(ctx)) {
    PROCESS_EXIT();
  }
  time = RTIMER_NOW();
  edhoc_client_start((uint8_t *)edhoc_state.ad.ad_1, edhoc_state.ad.ad_1_sz);

  while(1) {
    PROCESS_WAIT_EVENT();
    if((ev == edhoc_event) && (data == &edhoc_state)) {
      if(edhoc_state.val == CL_RESTART) {
        LOG_ERR("Error\n");
        /*When timeout will restart */
        if(attempt < EDHOC_CONF_ATTEMPTS) {
          LOG_INFO("Attempt %d\n", attempt);
          etimer_set(&wait_timer, CLOCK_SECOND * (CL_TIMEOUT_VAL / 1000));
          PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
          etimer_stop(&wait_timer);
          time = RTIMER_NOW();
          edhoc_client_start((uint8_t *)edhoc_state.ad.ad_1, edhoc_state.ad.ad_1_sz);
          attempt++;
        } else {
          LOG_ERR("Expire edhoc client attempts\n");
          edhoc_state.val = CL_TRIES_EXPIRE;
          pro = process_post(PROCESS_BROADCAST, edhoc_event, &edhoc_state);
          break;
        }
      }
      if(edhoc_state.val == CL_FINISHED) {
        time_total = RTIMER_NOW() - time_total;
        LOG_INFO("Client time to finish: %" PRIu32 " ms (%" PRIu32 " CPU cycles).\n", (uint32_t)((uint64_t)time_total * 1000 / RTIMER_SECOND), (uint32_t)time_total);
        break;
      }
      if(edhoc_state.val == CL_TIMEOUT) {
        LOG_ERR("Expire timeout\n");
        if(attempt < EDHOC_CONF_ATTEMPTS) {
          LOG_INFO("Attempt %d\n", attempt);
          etimer_set(&wait_timer, CLOCK_SECOND * (CL_TIMEOUT_VAL / 1000));
          PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
          etimer_stop(&wait_timer);
          edhoc_client_start((uint8_t *)edhoc_state.ad.ad_1, edhoc_state.ad.ad_1_sz);
          attempt++;
        } else {
          LOG_ERR("Expire edhoc client attempts\n");
          edhoc_state.val = CL_TRIES_EXPIRE;
          pro = process_post(PROCESS_BROADCAST, edhoc_event, &edhoc_state);
          break;
        }
      }
      if(edhoc_state.val == CL_BLOCK1) {
        edhoc_client_post_blocks();
      }
      if(edhoc_state.val == CL_POST) {
        edhoc_client_post();
        edhoc_client_post_blocks();
      }
      if(edhoc_state.val == CL_BLOCKING) {
        process_start(&edhoc_client_protocol, NULL);
        time = RTIMER_NOW() - time;
        LOG_INFO("Client time to rx MSG: %" PRIu32 " ms (%" PRIu32 " CPU cycles ).\n", (uint32_t)((uint64_t)time * 1000 / RTIMER_SECOND), (uint32_t)time);
        time = RTIMER_NOW();
        while(process_is_running(&edhoc_client_protocol)) {	
          process_run();
        }
      }
    }
  }
  PROCESS_EXIT();
  PROCESS_END();
}
