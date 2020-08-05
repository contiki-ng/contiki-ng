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

/* EDHOC Client protocol states */
#define NON_MSG 0
#define RX_MSG1 1
#define RX_MSG3 2
#define TX_MSG_ERR 3
#define EXP_READY 4

/* EDHOC process states */
#define SERV_FINISHED 1

static coap_timer_t timer;
static uint8_t msg_rx[MAX_DATA_LEN];
static size_t msg_rx_len;
static edhoc_server_t serv;
static process_event_t new_ecc_event;
static ecc_data_even_t new_ecc;

int8_t 
edhoc_server_callback(process_event_t ev, void *data){
  if((ev == new_ecc_event) && (new_ecc.val == SERV_FINISHED)){
    return 1;
  }
  return 0;
}
void 
edhoc_server_set_ad_2(const void * buff, uint8_t buff_sz){
  memcpy(new_ecc.ad.ad_2, (void*) buff, buff_sz);
  new_ecc.ad.ad_2_sz = buff_sz;
}

uint8_t 
edhoc_server_get_ad_1(char * buff){
  memcpy(buff, (void*) new_ecc.ad.ad_1, new_ecc.ad.ad_1_sz);
  return new_ecc.ad.ad_1_sz;
}

uint8_t 
edhoc_server_get_ad_3(char * buff){
  memcpy(buff, (void*) new_ecc.ad.ad_3, new_ecc.ad.ad_3_sz);
  return new_ecc.ad.ad_3_sz;
}

static void
server_timeout_callback(coap_timer_t *timer)
{
  LOG_ERR("Timeout\n");
  coap_timer_stop(timer);
  edhoc_server_close();
  LOG_DBG("server ctx start\n");
  edhoc_server_start();
}
static void
server_restart()
{
  serv.con_num = 0;
  serv.state = 0;
  serv.rx_msg1 = false;
  serv.rx_msg3 = false;
}
void
edhoc_server_start()
{
  LOG_INFO("SERVER: Edhoc new\n");
  ctx = edhoc_new();
  LOG_INFO("\n");
  LOG_DBG("x:");
  print_buff_8_dbg(ctx->ephimeral_key.public.x, ECC_KEY_BYTE_LENGHT + 1);
  LOG_DBG("y:");
  print_buff_8_dbg(ctx->ephimeral_key.public.y, ECC_KEY_BYTE_LENGHT);
  LOG_DBG("private:");
  print_buff_8_dbg(ctx->ephimeral_key.private_key, ECC_KEY_BYTE_LENGHT);
  serv.state = NON_MSG;
  edhoc_get_authentication_key(ctx);
  server_restart();
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
server_protocol(coap_message_t *request)
{
  int er = 0;
  int pro = 0;
  switch(serv.state) {
  case NON_MSG:
    LOG_DBG("NON_MSG\n");
    coap_timer_set_callback(&timer, server_timeout_callback);
    coap_timer_set(&timer, SERV_TIMEOUT_VAL);

  case RX_MSG1:
    LOG_DBG("RX_MSG1 (%d):\n", (int)msg_rx_len);
    print_buff_8_dbg(msg_rx, msg_rx_len);
    er = edhoc_handler_msg_1(ctx, msg_rx, msg_rx_len, (uint8_t *)new_ecc.ad.ad_1);
    if(er == RX_ERR_MSG) {
      LOG_DBG("error code (%d)\n", er);
      serv.state = NON_MSG;
      coap_timer_stop(&timer);
      edhoc_server_close();
      LOG_DBG("server ctx start\n");
      edhoc_server_start();
    } else if(er < RX_ERR_MSG) {
      LOG_DBG("Send MSG error with code (%d)\n", er);
      ctx->tx_sz = edhoc_gen_msg_error(ctx->msg_tx, ctx, er);
      serv.state = TX_MSG_ERR;
      coap_timer_stop(&timer);
      edhoc_server_close();
      LOG_DBG("server ctx start\n");
      edhoc_server_start();
    } else {

      /* Set the 5-tuple ipaddres to identify the connection */
      memcpy(&serv.con_ipaddr, &request->src_ep->ipaddr, sizeof(uip_ipaddr_t));
      new_ecc.ad.ad_1_sz = er;
      if(new_ecc.ad.ad_1_sz > 0 && new_ecc.ad.ad_1) {
        LOG_INFO("APP DATA MSG 1 (%d):", new_ecc.ad.ad_1_sz);
        print_char_8_info((char *)new_ecc.ad.ad_1, new_ecc.ad.ad_1_sz);
      }
      serv.rx_msg1 = true;
      /*Generate MSG2 */
      edhoc_gen_msg_2(ctx, (uint8_t *)new_ecc.ad.ad_2, new_ecc.ad.ad_2_sz);
      LOG_INFO("MSG2 (%d)\n", ctx->tx_sz);
      print_buff_8_dbg(ctx->msg_tx, ctx->tx_sz);
      serv.state = RX_MSG3;
    }
    break;
  case RX_MSG3:
    LOG_DBG("RX_MSG3 (%d):\n", (int)msg_rx_len);
    print_buff_8_dbg(msg_rx, msg_rx_len);
    er = edhoc_handler_msg_3(ctx, msg_rx, msg_rx_len, (uint8_t *)new_ecc.ad.ad_3);
    if(er == RX_ERR_MSG) {
      LOG_DBG("error code (%d)\n", er);
      serv.state = NON_MSG;
      coap_timer_stop(&timer);
      edhoc_server_close();
      LOG_DBG("server ctx start\n");
      edhoc_server_start();
      break;
    } else if(er < RX_ERR_MSG) {
      LOG_DBG("Send MSG error with code (%d)\n", er);
      ctx->tx_sz = edhoc_gen_msg_error(ctx->msg_tx, ctx, er);
      coap_timer_stop(&timer);
      edhoc_server_close();
      LOG_DBG("server ctx start\n");
      edhoc_server_start();
      serv.state = TX_MSG_ERR;
      break;
    } else {
      /*TODO: Include a way to pass aplictaion msgs. */
      new_ecc.ad.ad_3_sz = er;
      if(new_ecc.ad.ad_3_sz > 0 && new_ecc.ad.ad_3) {
        LOG_INFO("APP DATA MSG 3 (%d):", new_ecc.ad.ad_3_sz);
        print_char_8_info((char *)new_ecc.ad.ad_3, new_ecc.ad.ad_3_sz);
      }

      serv.state = EXP_READY;
      serv.rx_msg3 = true;
    }
  case EXP_READY:
    if(serv.rx_msg1 && serv.rx_msg3) {
      LOG_INFO("EXPORTER\n");
      ctx->tx_sz = 0;
      new_ecc.val = SERV_FINISHED;
      coap_timer_stop(&timer);
      pro = process_post(PROCESS_BROADCAST, new_ecc_event, &new_ecc);
      LOG_DBG("PROCESS POST EVENT: %d\n", pro);
      coap_timer_stop(&timer);
      break;
    } else {
      LOG_ERR("Protocol steap missed\n");
      serv.state = NON_MSG;
    }
  }
}
void
edhoc_server_resource(coap_message_t *request, coap_message_t *response)
{
  LOG_DBG("/edhoc POST (%s %u)\n", request->type == COAP_TYPE_CON ? "CON" : "NON", request->mid);
  LOG_DBG("PAYLOAD:");
  print_buff_8_dbg((uint8_t *)request->payload, request->payload_len);

  LOG_DBG("con_num:%u\n", serv.con_num);

  if(coap_is_option(request, COAP_OPTION_BLOCK1)) {
    LOG_DBG("Blockwise: block 1 request: Num: %" PRIu32
            ", More: %u, Size: %u, Offset: %" PRIu32 "\n",
            request->block1_num,
            request->block1_more,
            request->block1_size,
            request->block1_offset);

    if(coap_block1_handler(request, response, msg_rx, &msg_rx_len, MAX_DATA_LEN)) {
      LOG_DBG("handeler (%d)\n", (int)msg_rx_len);
      print_buff_8_dbg(msg_rx, msg_rx_len);
      /*More blocks will followr*/
      return;
    }
    /*Check the 5-tuple information before retrive the state protocol*/
    if((serv.state != NON_MSG) && (memcmp(&serv.con_ipaddr, &request->src_ep->ipaddr, sizeof(uip_ipaddr_t)) != 0)) {
      LOG_ERR("rx request from an error ipaddr\n");
      coap_set_payload(response, NULL, 0);
      coap_set_status_code(response, BAD_REQUEST_4_00);
    } else {
      LOG_INFO("correct ipdaadr\n");
      server_protocol(request);
      if(serv.state == NON_MSG) {
        LOG_ERR("RX MSG ERROR response\n");
        coap_set_payload(response, NULL, 0);
        coap_set_status_code(response, DELETED_2_02);
      } else {
        LOG_DBG("Set response payload 1\n");
        response->payload = (uint8_t *)ctx->msg_tx;
        response->payload_len = ctx->tx_sz;
        if(response->payload_len == 0) {
          memset(&(response->options), 0, 8);
          memset(&(request->options), 0, 8);
          LOG_DBG("Payload len 0 \n");
          coap_set_header_block1(response, request->block1_num, 0, COAP_MAX_CHUNK_SIZE);
        } else {
          LOG_DBG("Payload len > 0\n");
          coap_set_header_block1(response, request->block1_num, 0, COAP_MAX_CHUNK_SIZE);
          coap_set_header_block2(response, 0, ctx->tx_sz > COAP_MAX_CHUNK_SIZE ? 1 : 0, COAP_MAX_CHUNK_SIZE);
        }
        if(serv.state == TX_MSG_ERR) {
          serv.state = NON_MSG;
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
      LOG_DBG("Set BLOCK 2 (%d): ", response->payload_len);
      print_buff_8_dbg(response->payload, response->payload_len);
    }
  }
  if(coap_is_option(request, COAP_OPTION_BLOCK2) && !coap_is_option(request, COAP_OPTION_BLOCK1)) {
    response->payload = (uint8_t *)ctx->msg_tx;
    response->payload_len = ctx->tx_sz;
    coap_set_option(response, COAP_OPTION_BLOCK2);
    coap_set_status_code(response, CHANGED_2_04);
    LOG_DBG("BLOCK 2 (%d): ", response->payload_len);
    print_buff_8_dbg(response->payload, response->payload_len);
  }
}
