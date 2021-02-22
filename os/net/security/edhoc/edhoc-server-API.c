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
 *      EDHOC server API [draft-ietf-lake-edhoc-01] with CoAP Block-Wise Transfer [RFC7959]
 * \author
 *      Lidia Pocero <pocero@isi.gr>
 */

#include "edhoc-server-API.h"
#include "sys/pt.h"
#include "sys/rtimer.h"

/* EDHOC Client protocol states */
#define NON_MSG 0
#define RX_MSG1 1
#define RX_MSG3 2
#define TX_MSG_ERR 3
#define EXP_READY 4
#define RESTART 5

static rtimer_clock_t time;
static rtimer_clock_t time_total;

#define RTIME_MS 32768

static coap_timer_t timer;
static uint8_t msg_rx[MAX_DATA_LEN];
static size_t msg_rx_len;
static edhoc_server_t server;
static edhoc_server_t *serv;
static process_event_t new_ecc_event;
static ecc_data_even_t new_ecc;

static coap_message_t *request;
static coap_message_t *response;
static int er = 0;
uint8_t more = 0;
static cose_key_t key;
static uint8_t *pt = NULL;
static edhoc_msg_3 msg3;
PROCESS(edhoc_server, "Edhoc Server");

int8_t
edhoc_server_callback(process_event_t ev, void *data)
{
  if((ev == new_ecc_event) && (new_ecc.val == SERV_FINISHED)) {
    return SERV_FINISHED;
  }
  if((ev == new_ecc_event) && (new_ecc.val == SERV_RESTART)) {
    LOG_DBG("server callback: SERV_RESTART\n");
    return SERV_RESTART;
  }
  return 0;
}
void
edhoc_server_set_ad_2(const void *buff, uint8_t buff_sz)
{
  memcpy(new_ecc.ad.ad_2, (void *)buff, buff_sz);
  new_ecc.ad.ad_2_sz = buff_sz;
}
uint8_t
edhoc_server_get_ad_1(char *buff)
{
  memcpy(buff, (void *)new_ecc.ad.ad_1, new_ecc.ad.ad_1_sz);
  return new_ecc.ad.ad_1_sz;
}
uint8_t
edhoc_server_get_ad_3(char *buff)
{
  memcpy(buff, (void *)new_ecc.ad.ad_3, new_ecc.ad.ad_3_sz);
  return new_ecc.ad.ad_3_sz;
}
static void
server_timeout_callback(coap_timer_t *timer)
{
  LOG_ERR("Timeout\n");
  coap_timer_stop(timer);
  edhoc_server_close();
  new_ecc.val = SERV_RESTART;
  process_post(PROCESS_BROADCAST, new_ecc_event, &new_ecc);
}
uint8_t
edhoc_server_restart()
{
  serv->con_num = 0;
  serv->state = 0;
  serv->rx_msg1 = false;
  serv->rx_msg3 = false;
  serv->state = NON_MSG;
  memset(&server, 0, sizeof(edhoc_server_t));
  //memset(&ctx,0,sizeof(edhoc_context_t));
  edhoc_init(ctx);
  return edhoc_get_authentication_key(ctx);
}
uint8_t
edhoc_server_start()
{
  LOG_INFO("SERVER: Edhoc new\n");
  ctx = edhoc_new();
  memset(&server, 0, sizeof(edhoc_server_t));
  serv = &server;
  return edhoc_server_restart();
}
void
edhoc_server_init()
{
  LOG_INFO("SERVER: Coap active resource\n");
  coap_activate_resource(&res_edhoc, WELL_KNOWN);
  new_ecc_event = process_alloc_event();
}
void
edhoc_server_close()
{
  edhoc_finalize(ctx);
}
void
edhoc_server_process(coap_message_t *req, coap_message_t *res, edhoc_server_t *ser, uint8_t *msg, uint8_t len)
{
  serv_data_t serv_data = { req, res, ser };
  dat_ptr = &serv_data;
  process_data_t dat = dat_ptr;
  memcpy(msg_rx, msg, len);
  msg_rx_len = len;
  process_start(&edhoc_server, dat);
  while(process_is_running(&edhoc_server)) {
    process_run();
  }
}
PROCESS_THREAD(edhoc_server, ev, data){
  PROCESS_BEGIN();
  request = ((struct serv_data_t *)data)->request;
  response = ((struct serv_data_t *)data)->response;
  serv = ((struct serv_data_t *)data)->serv;
  LOG_DBG("/edhoc POST (%s %u)\n", request->type == COAP_TYPE_CON ? "CON" : "NON", request->mid);
  LOG_DBG("PAYLOAD:");
  print_buff_8_dbg((uint8_t *)request->payload, request->payload_len);
  LOG_DBG("con_num:%u\n", serv->con_num);
  if(serv->state == EXP_READY) {
    LOG_DBG("process exit\n");
  }
  /*Check the 5-tuple information before retrive the state protocol */
  if((serv->state != NON_MSG) && (memcmp(&serv->con_ipaddr, &request->src_ep->ipaddr, sizeof(uip_ipaddr_t)) != 0)) {
    LOG_ERR("rx request from an error ipaddr\n");
    coap_set_payload(response, NULL, 0);
    coap_set_status_code(response, BAD_REQUEST_4_00);
  } else {

    switch(serv->state) {
    case NON_MSG:
      coap_timer_set_callback(&timer, server_timeout_callback);
      coap_timer_set(&timer, SERV_TIMEOUT_VAL);

    case RX_MSG1:
      LOG_INFO("----------------------------------Handler message_1-----------------------------\n");
      LOG_INFO("RX message_1 (CBOR Sequence) (%d bytes):\n", (int)msg_rx_len);
      print_buff_8_info(msg_rx, msg_rx_len);

      time_total = RTIMER_NOW();
      time = RTIMER_NOW();
      er = edhoc_handler_msg_1(ctx, msg_rx, msg_rx_len, (uint8_t *)new_ecc.ad.ad_1);
      time = RTIMER_NOW() - time;
      LOG_PRINT("Server time to handler MSG1: %" PRIu32 " ms (%" PRIu32 " CPU cycles ).\n", (uint32_t)((uint64_t)time * 1000 / RTIMER_SECOND), (uint32_t)time);

      if(er == RX_ERR_MSG) {
        LOG_WARN("error code (%d)\n", er);
        serv->state = NON_MSG;
        coap_timer_stop(&timer);
        edhoc_server_close();
        new_ecc.val = SERV_RESTART;
        process_post(PROCESS_BROADCAST, new_ecc_event, &new_ecc);
      } else if(er < RX_ERR_MSG) {
        LOG_WARN("Send MSG error with code (%d)\n", er);
        ctx->tx_sz = edhoc_gen_msg_error(ctx->msg_tx, ctx, er);
        serv->state = TX_MSG_ERR;
        if(er != ERR_NEW_SUIT_PROPOSE){
          coap_timer_stop(&timer);
          edhoc_server_close();
          new_ecc.val = SERV_RESTART;
          process_post(PROCESS_BROADCAST, new_ecc_event, &new_ecc);
        }
      } else {
        /* Set the 5-tuple ipaddres to identify the connection */
        memcpy(&serv->con_ipaddr, &request->src_ep->ipaddr, sizeof(uip_ipaddr_t));
        new_ecc.ad.ad_1_sz = er;
        if(new_ecc.ad.ad_1_sz > 0 && new_ecc.ad.ad_1) {
          LOG_INFO("AD_1 (%d bytes):", new_ecc.ad.ad_1_sz);
          print_char_8_info((char *)new_ecc.ad.ad_1, new_ecc.ad.ad_1_sz);
        }
        serv->rx_msg1 = true;
        /*Generate MSG2 */
        time = RTIMER_NOW();
        LOG_INFO("---------------------------------generate message_2-----------------------------\n");
        edhoc_gen_msg_2(ctx, (uint8_t *)new_ecc.ad.ad_2, new_ecc.ad.ad_2_sz);
        time = RTIMER_NOW() - time;
        LOG_PRINT("Server time to gen MSG2: %" PRIu32 " ms (%" PRIu32 " CPU cycles ).\n", (uint32_t)((uint64_t)time * 1000 / RTIMER_SECOND), (uint32_t)time);
        LOG_INFO("message_2 (CBOR Sequence) (%d bytes):", ctx->tx_sz);
        print_buff_8_info(ctx->msg_tx, ctx->tx_sz);
        serv->state = RX_MSG3;
      }
      break;
    case RX_MSG3:
      LOG_INFO("----------------------------------Handler message_3-----------------------------\n");
      LOG_INFO("RX message_3 (%d bytes):", (int)msg_rx_len);
      print_buff_8_info(msg_rx, msg_rx_len);
      time = RTIMER_NOW();
      er = edhoc_handler_msg_3(&msg3, ctx, msg_rx, msg_rx_len);
      time = RTIMER_NOW() - time;
      LOG_PRINT("Server time to handler MSG3: %" PRIu32 " ms (%" PRIu32 " CPU cycles ).\n", (uint32_t)((uint64_t)time * 1000 / RTIMER_SECOND), (uint32_t)time);
      time = RTIMER_NOW();
      if(er > 0) {
        er = edhoc_get_auth_key(ctx, &pt, &key);
      }
      if(er > 0) {
        er = edhoc_authenticate_msg(ctx, &pt, msg3.cipher.len, (uint8_t *)new_ecc.ad.ad_3, &key);
        time = RTIMER_NOW() - time;
        LOG_PRINT("Server time to authenticate: %" PRIu32 " ms (%" PRIu32 " CPU cycles ).\n", (uint32_t)((uint64_t)time * 1000 / RTIMER_SECOND), (uint32_t)time);
      }

      if(er == RX_ERR_MSG) {
        serv->state = NON_MSG;
        coap_timer_stop(&timer);
        edhoc_server_close();
        new_ecc.val = SERV_RESTART;
        process_post(PROCESS_BROADCAST, new_ecc_event, &new_ecc);
        break;
      } else if(er < RX_ERR_MSG) {
        ctx->tx_sz = edhoc_gen_msg_error(ctx->msg_tx, ctx, er);
        coap_timer_stop(&timer);
        edhoc_server_close();
        new_ecc.val = SERV_RESTART;
        process_post(PROCESS_BROADCAST, new_ecc_event, &new_ecc);
        serv->state = TX_MSG_ERR;
        break;
      } else {
        /*TODO: Include a way to pass aplictaion msgs. */
        new_ecc.ad.ad_3_sz = er;
        if(new_ecc.ad.ad_3_sz > 0 && new_ecc.ad.ad_3) {
          LOG_INFO("AP_3 (%d bytes):", new_ecc.ad.ad_3_sz);
          print_char_8_info((char *)new_ecc.ad.ad_3, new_ecc.ad.ad_3_sz);
        }

        serv->state = EXP_READY;
        serv->rx_msg3 = true;
      }
    case EXP_READY:
      if(serv->rx_msg1 && serv->rx_msg3) {
        LOG_INFO("--------------EXPORTER------------------------\n");
        ctx->tx_sz = 0;
        new_ecc.val = SERV_FINISHED;
        time_total = RTIMER_NOW() - time_total;
        LOG_PRINT("Server time to finish: %" PRIu32 " ms (%" PRIu32 " CPU cycles ).\n", (uint32_t)time_total * 1000 / RTIMER_SECOND, (uint32_t)time_total);
        time_total = RTIMER_NOW();
        coap_timer_stop(&timer);
        process_post(PROCESS_BROADCAST, new_ecc_event, &new_ecc);
        coap_timer_stop(&timer);
        break;
      } else {
        LOG_ERR("Protocol steap missed\n");
        serv->state = NON_MSG;
      }
    }
  }

  if(serv->state == NON_MSG) {
    LOG_ERR("RX MSG ERROR response\n");
    coap_set_payload(response, NULL, 0);
    coap_set_status_code(response, DELETED_2_02);
  } else {
    response->payload = (uint8_t *)ctx->msg_tx;
    response->payload_len = ctx->tx_sz;
    coap_set_status_code(response, CHANGED_2_04);
    if(response->payload_len == 0) {
      memset(&(response->options), 0, 8);
      memset(&(request->options), 0, 8);
    } else {
      memset(&(response->options), 0, 8);
      coap_set_header_block2(response, 0, ctx->tx_sz > COAP_MAX_CHUNK_SIZE ? 1 : 0, COAP_MAX_CHUNK_SIZE);
    }
    if(serv->state == TX_MSG_ERR) {
      serv->state = NON_MSG;
      coap_set_status_code(response, CHANGED_2_04);
    } else {
      coap_set_status_code(response, CHANGED_2_04);
    }
    LOG_DBG("Blockwise: block 1 response: Num: %" PRIu32
            ", More: %u, Size: %u, Offset: %" PRIu32 "\n",
            response->block1_num,
            response->block1_more,
            response->block1_size,
            response->block1_offset);
    LOG_DBG("Blockwise: block 2 response: Num: %" PRIu32
            ", More: %u, Size: %u, Offset: %" PRIu32 "\n",
            response->block2_num,
            response->block2_more,
            response->block2_size,
            response->block2_offset);
  }
  PROCESS_END();
}
