/*
 * Copyright (c) 2017, Yasuyuki Tanaka
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"
#include "lib/assert.h"

#include "net/packetbuf.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/sixtop/sixtop.h"
#include "net/mac/tsch/sixtop/sixp.h"
#include "net/mac/tsch/sixtop/sixp-nbr.h"
#include "net/mac/tsch/sixtop/sixp-trans.h"

#include "unit-test/unit-test.h"
#include "common.h"

#define UNKNOWN_SF_SFID 0
#define TEST_SF_SFID   0xf1

static linkaddr_t peer_addr;
static uint8_t test_sf_input_is_called = 0;

#define YIELD_CPU_FOR_TIMER_TASKS(et) do {              \
    etimer_set(&et, CLOCK_SECOND);                      \
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));      \
} while(0)

static void
input(sixp_pkt_type_t type,sixp_pkt_code_t code, const uint8_t *body,
      uint16_t body_len, const linkaddr_t *peer_addr)
{
  test_sf_input_is_called = 1;
}

static const sixtop_sf_t test_sf = {
  TEST_SF_SFID,
  0,
  NULL,
  input,
  NULL,
  NULL
};

PROCESS(test_process, "6top protocol APIs test");
AUTOSTART_PROCESSES(&test_process);

static void
test_setup(void)
{
  test_mac_driver.init();
  sixtop_init();
  packetbuf_clear();
  memset(&peer_addr, 0, sizeof(peer_addr));
  sixtop_add_sf(&test_sf);
  test_sf_input_is_called = 0;
}

UNIT_TEST_REGISTER(test_input_no_sf,
                   "sixp_input(no_sf)");
UNIT_TEST(test_input_no_sf)
{
  uint8_t seqno = 10;
  uint32_t body;
  uint8_t *p;

  UNIT_TEST_BEGIN();
  test_setup();

  memset(&body, 0, sizeof(body));
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   UNKNOWN_SF_SFID, seqno,
                                   (const uint8_t *)&body, sizeof(body),
                                   NULL) == 0);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);

  p = packetbuf_hdrptr();

  /* length */
  UNIT_TEST_ASSERT(packetbuf_totlen() == 11);

  /* Termination 1 IE */
  UNIT_TEST_ASSERT(p[0] == 0x00);
  UNIT_TEST_ASSERT(p[1] == 0x3f);

  /* IETF IE */
  UNIT_TEST_ASSERT(p[2] == 0x05);
  UNIT_TEST_ASSERT(p[3] == 0xa8);

  /* 6top IE */
  UNIT_TEST_ASSERT(p[4] == 0xc9);
  UNIT_TEST_ASSERT(p[5] == 0x10);
  UNIT_TEST_ASSERT(p[6] == SIXP_PKT_RC_ERR_SFID);
  UNIT_TEST_ASSERT(p[7] == UNKNOWN_SF_SFID);
  UNIT_TEST_ASSERT(p[8] == seqno);

  /* Payload Termination IE */
  UNIT_TEST_ASSERT(p[9] == 0x00);
  UNIT_TEST_ASSERT(p[10] == 0xf8);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 1);
  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_input_busy,
                   "sixp_input(busy)");
UNIT_TEST(test_input_busy)
{
  sixp_nbr_t *nbr;
  uint8_t *p;
  uint8_t seqno = 10;
  uint16_t metadata = 0;
  uint32_t body;

  UNIT_TEST_BEGIN();
  test_setup();

  /* send a request to the peer first */
  /* initial seqnum is set to 10 */
  UNIT_TEST_ASSERT((nbr = sixp_nbr_alloc(&peer_addr)) != NULL);
  UNIT_TEST_ASSERT(sixp_nbr_set_next_seqno(nbr, seqno) == 0);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) == NULL);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_REQUEST,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_SIGNAL,
                               TEST_SF_SFID,
                               (const uint8_t*)&metadata, sizeof(metadata),
                               &peer_addr, NULL, NULL, 0) == 0);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 1);
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) != NULL);

  test_mac_driver.init(); /* clear test_mac_send_is_called status */

  /*
   * when received a request having non-zero SeqNum, we will send back
   * ERR_RC_BUSY
   */
  memset(&body, 0, sizeof(body));
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID, seqno,
                                   (const uint8_t *)&body, sizeof(body),
                                   NULL) == 0);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);

  p = packetbuf_hdrptr();

  /* length */
  UNIT_TEST_ASSERT(packetbuf_totlen() == 11);

  /* Termination 1 IE */
  UNIT_TEST_ASSERT(p[0] == 0x00);
  UNIT_TEST_ASSERT(p[1] == 0x3f);

  /* IETF IE */
  UNIT_TEST_ASSERT(p[2] == 0x05);
  UNIT_TEST_ASSERT(p[3] == 0xa8);

  /* 6top IE */
  UNIT_TEST_ASSERT(p[4] == 0xc9);
  UNIT_TEST_ASSERT(p[5] == 0x10);
  UNIT_TEST_ASSERT(p[6] == SIXP_PKT_RC_ERR_BUSY);
  UNIT_TEST_ASSERT(p[7] == TEST_SF_SFID);
  UNIT_TEST_ASSERT(p[8] == seqno);

  /* Payload Termination IE */
  UNIT_TEST_ASSERT(p[9] == 0x00);
  UNIT_TEST_ASSERT(p[10] == 0xf8);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 1);

  /*
   * in case a received request having SeqNum of zero, we think the
   * peer had power-cycle, we will send back ERR_RC_SEQNUM.
   */
  test_mac_driver.init();
  packetbuf_clear();
  memset(&body, 0, sizeof(body));
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID,
                                   0, /* SeqNum = 0*/
                                   (const uint8_t *)&body, sizeof(body),
                                   NULL) == 0);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(test_sf_input_is_called == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);
  UNIT_TEST_ASSERT(test_sf_input_is_called == 0);

  p = packetbuf_hdrptr();

  /* length */
  UNIT_TEST_ASSERT(packetbuf_totlen() == 11);

  /* Termination 1 IE */
  UNIT_TEST_ASSERT(p[0] == 0x00);
  UNIT_TEST_ASSERT(p[1] == 0x3f);

  /* IETF IE */
  UNIT_TEST_ASSERT(p[2] == 0x05);
  UNIT_TEST_ASSERT(p[3] == 0xa8);

  /* 6top IE */
  UNIT_TEST_ASSERT(p[4] == 0xc9);
  UNIT_TEST_ASSERT(p[5] == 0x10);
  UNIT_TEST_ASSERT(p[6] == SIXP_PKT_RC_ERR_SEQNUM);
  UNIT_TEST_ASSERT(p[7] == TEST_SF_SFID);
  UNIT_TEST_ASSERT(p[8] == 0);

  /* Payload Termination IE */
  UNIT_TEST_ASSERT(p[9] == 0x00);
  UNIT_TEST_ASSERT(p[10] == 0xf8);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_input_no_memory,
                   "sixp_input(no_memory)");
UNIT_TEST(test_input_no_memory)
{
  uint8_t seqno = 10;
  uint32_t body;
  sixp_pkt_t pkt;
  uint8_t *p;
  linkaddr_t addr;

  UNIT_TEST_BEGIN();
  test_setup();

  memset(&body, 0, sizeof(body));
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID, seqno,
                                   (const uint8_t *)&body, sizeof(body),
                                   &pkt) == 0);
  memset(&addr, 0, sizeof(addr));
  addr.u8[0] = 1;
  UNIT_TEST_ASSERT(sixp_trans_alloc(&pkt, &addr) != NULL);
  addr.u8[0] = 2;
  UNIT_TEST_ASSERT(sixp_trans_alloc(&pkt, &addr) != NULL);
  addr.u8[0] = 3;
  /* no memory left for another transaction */
  UNIT_TEST_ASSERT(sixp_trans_alloc(&pkt, &addr) == NULL);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);

  p = packetbuf_hdrptr();

  /* length */
  UNIT_TEST_ASSERT(packetbuf_totlen() == 11);

  /* Termination 1 IE */
  UNIT_TEST_ASSERT(p[0] == 0x00);
  UNIT_TEST_ASSERT(p[1] == 0x3f);

  /* IETF IE */
  UNIT_TEST_ASSERT(p[2] == 0x05);
  UNIT_TEST_ASSERT(p[3] == 0xa8);

  /* 6top IE */
  UNIT_TEST_ASSERT(p[4] == 0xc9);
  UNIT_TEST_ASSERT(p[5] == 0x10);
  UNIT_TEST_ASSERT(p[6] == SIXP_PKT_RC_ERR_BUSY);
  UNIT_TEST_ASSERT(p[7] == TEST_SF_SFID);
  UNIT_TEST_ASSERT(p[8] == seqno);

  /* Payload Termination IE */
  UNIT_TEST_ASSERT(p[9] == 0x00);
  UNIT_TEST_ASSERT(p[10] == 0xf8);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 1);
  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_output_request_1,
                   "sixp_output(request_1)");
UNIT_TEST(test_output_request_1)
{
  sixp_pkt_t pkt;
  sixp_trans_t *trans;
  uint32_t body;

  UNIT_TEST_BEGIN();

  test_setup();
  memset(&body, 0, sizeof(body));

  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID, 10,
                                   (const uint8_t *)&body, sizeof(body),
                                   &pkt) == 0);
  UNIT_TEST_ASSERT((trans = sixp_trans_alloc(&pkt, &peer_addr)) != NULL);
  memset(&body, 0, sizeof(body));

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_REQUEST,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                               TEST_SF_SFID, (uint8_t *)&body, sizeof(body),
                               &peer_addr, NULL, NULL, 0) == -1);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_output_request_2,
                   "sixp_output(request_2)");
UNIT_TEST(test_output_request_2)
{
  uint32_t body;
  UNIT_TEST_BEGIN();

  test_setup();
  memset(&body, 0, sizeof(body));

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_REQUEST,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                               TEST_SF_SFID, (uint8_t *)&body, sizeof(body),
                               &peer_addr, NULL, NULL, 0) == 0);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_output_response_1,
                   "sixp_output(response_1)");
UNIT_TEST(test_output_response_1)
{
  UNIT_TEST_BEGIN();
  test_setup();

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               TEST_SF_SFID, NULL, 0,
                               &peer_addr, NULL, NULL, 0) == -1);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_output_response_2,
                   "sixp_output(response_2)");
UNIT_TEST(test_output_response_2)
{
  sixp_pkt_t pkt;
  sixp_trans_t *trans;
  uint32_t body;

  UNIT_TEST_BEGIN();
  test_setup();

  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID, 10,
                                   (const uint8_t *)&body, sizeof(body),
                                   &pkt) == 0);
  UNIT_TEST_ASSERT((trans = sixp_trans_alloc(&pkt, &peer_addr)) != NULL);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               TEST_SF_SFID, NULL, 0,
                               &peer_addr, NULL, NULL, 0) == -1);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_output_response_3,
                   "sixp_output(response_3)");
UNIT_TEST(test_output_response_3)
{
  sixp_pkt_t pkt;
  sixp_trans_t *trans;
  uint32_t body;

  UNIT_TEST_BEGIN();
  test_setup();

  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID, 10,
                                   (const uint8_t *)&body, sizeof(body),
                                   &pkt) == 0);
  UNIT_TEST_ASSERT((trans = sixp_trans_alloc(&pkt, &peer_addr)) != NULL);
  UNIT_TEST_ASSERT(
                   sixp_trans_transit_state(trans, SIXP_TRANS_STATE_REQUEST_RECEIVED) == 0);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               TEST_SF_SFID, NULL, 0,
                               &peer_addr, NULL, NULL, 0) == 0);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_output_response_4,
                   "sixp_output(response_4)");
UNIT_TEST(test_output_response_4)
{
  sixp_pkt_t pkt;
  sixp_trans_t *trans;
  uint32_t body;

  UNIT_TEST_BEGIN();
  test_setup();

  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID, 10,
                                   (const uint8_t *)&body, sizeof(body),
                                   &pkt) == 0);
  UNIT_TEST_ASSERT((trans = sixp_trans_alloc(&pkt, &peer_addr)) != NULL);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans, SIXP_TRANS_STATE_REQUEST_SENDING) == 0);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans, SIXP_TRANS_STATE_REQUEST_SENT) == 0);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans, SIXP_TRANS_STATE_RESPONSE_RECEIVED) == 0);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               TEST_SF_SFID, NULL, 0,
                               &peer_addr, NULL, NULL, 0) == -1);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_output_confirmation_1,
                   "sixp_output(confirmation_1)");
UNIT_TEST(test_output_confirmation_1)
{
  UNIT_TEST_BEGIN();
  test_setup();

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_CONFIRMATION,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               TEST_SF_SFID, NULL, 0,
                               &peer_addr, NULL, NULL, 0) == -1);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_output_confirmation_2,
                   "sixp_output(confirmation_2)");
UNIT_TEST(test_output_confirmation_2)
{
  sixp_pkt_t pkt;
  sixp_trans_t *trans;
  uint32_t body;

  UNIT_TEST_BEGIN();
  test_setup();

  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID, 10,
                                   (const uint8_t *)&body, sizeof(body),
                                   &pkt) == 0);
  UNIT_TEST_ASSERT((trans = sixp_trans_alloc(&pkt, &peer_addr)) != NULL);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_CONFIRMATION,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               TEST_SF_SFID, NULL, 0,
                               &peer_addr, NULL, NULL, 0) == -1);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_output_confirmation_3,
                   "sixp_output(confirmation_3)");
UNIT_TEST(test_output_confirmation_3)
{
  sixp_pkt_t pkt;
  sixp_trans_t *trans;
  uint32_t body;

  UNIT_TEST_BEGIN();
  test_setup();

  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID, 10,
                                   (const uint8_t *)&body, sizeof(body),
                                   &pkt) == 0);
  UNIT_TEST_ASSERT((trans = sixp_trans_alloc(&pkt, &peer_addr)) != NULL);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans, SIXP_TRANS_STATE_REQUEST_SENDING) == 0);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans, SIXP_TRANS_STATE_REQUEST_SENT) == 0);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans, SIXP_TRANS_STATE_RESPONSE_RECEIVED) == 0);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_CONFIRMATION,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               TEST_SF_SFID, NULL, 0,
                               &peer_addr, NULL, NULL, 0) == 0);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_output_confirmation_4,
                   "sixp_output(confirmation_4)");
UNIT_TEST(test_output_confirmation_4)
{
  sixp_pkt_t pkt;
  sixp_trans_t *trans;
  uint32_t body;

  UNIT_TEST_BEGIN();
  test_setup();

  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID, 10,
                                   (const uint8_t *)&body, sizeof(body),
                                   &pkt) == 0);
  UNIT_TEST_ASSERT((trans = sixp_trans_alloc(&pkt, &peer_addr)) != NULL);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans, SIXP_TRANS_STATE_REQUEST_SENDING) == 0);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans, SIXP_TRANS_STATE_REQUEST_SENT) == 0);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans, SIXP_TRANS_STATE_RESPONSE_RECEIVED) == 0);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans,
                             SIXP_TRANS_STATE_CONFIRMATION_SENDING) == 0);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans,
                             SIXP_TRANS_STATE_CONFIRMATION_SENT) == 0);

  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_CONFIRMATION,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               TEST_SF_SFID, NULL, 0,
                               &peer_addr, NULL, NULL, 0) == -1);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_reset_by_clear_request_1,
                   "test if next_seqno is reset by sending CLEAR");
UNIT_TEST(test_next_seqno_reset_by_clear_request_1)
{
  linkaddr_t peer_addr;
  sixp_nbr_t *nbr;
  sixp_trans_t *trans;

  UNIT_TEST_BEGIN();

  test_setup();

  /*
   * When the node is the initiator of CLEAR, nbr->next_seqno must be
   * reset to 0 regardless of the presence or absent of L2 ACK to the
   * CLEAR Request.
   */
  /* set next_seqno to 3 as the initial state for this sub-test  */
  memset(&peer_addr, 0, sizeof(peer_addr));
  peer_addr.u8[0] = 1;
  UNIT_TEST_ASSERT((nbr = sixp_nbr_alloc(&peer_addr)) != NULL);
  UNIT_TEST_ASSERT(sixp_nbr_set_next_seqno(nbr, 3) == 0);
  UNIT_TEST_ASSERT(sixp_nbr_get_next_seqno(nbr) == 3);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_REQUEST,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_CLEAR,
                               TEST_SF_SFID, NULL, 0, &peer_addr,
                               NULL, NULL, 0) == 0);
  UNIT_TEST_ASSERT(sixp_nbr_get_next_seqno(nbr) == 0);
  UNIT_TEST_ASSERT((trans = sixp_trans_find(&peer_addr)) != NULL);
  UNIT_TEST_ASSERT(sixp_trans_transit_state(trans,
                                            SIXP_TRANS_STATE_REQUEST_SENT)
                   == 0);
  UNIT_TEST_ASSERT(sixp_nbr_get_next_seqno(nbr) == 0);
  UNIT_TEST_ASSERT(sixp_trans_transit_state(trans,
                                            SIXP_TRANS_STATE_TERMINATING)
                   == 0);
  UNIT_TEST_ASSERT(sixp_nbr_get_next_seqno(nbr) == 0);
  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_reset_by_clear_request_2,
                   "test if next_seqno is reset by receiving CLEAR");
UNIT_TEST(test_next_seqno_reset_by_clear_request_2)
{
  linkaddr_t peer_addr;
  sixp_nbr_t *nbr;
  sixp_pkt_metadata_t metadata;

  UNIT_TEST_BEGIN();

  test_setup();

  /*
   * When the node is the responder of CLEAR, nbr->next_seqno must be
   * reset to 0 regardless of the presence or absent of L2 ACK to the
   * CLEAR Response.
   */
  /* set next_seqno to 3 as the initial state for this sub-test  */
  memset(&peer_addr, 0, sizeof(peer_addr));
  peer_addr.u8[0] = 1;
  UNIT_TEST_ASSERT((nbr = sixp_nbr_alloc(&peer_addr)) != NULL);
  UNIT_TEST_ASSERT(sixp_nbr_set_next_seqno(nbr, 3) == 0);
  UNIT_TEST_ASSERT(sixp_nbr_get_next_seqno(nbr) == 3);
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_CLEAR,
                                   TEST_SF_SFID, 10,
                                   (const uint8_t *)&metadata,
                                   sizeof(metadata), NULL) == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);
  UNIT_TEST_ASSERT(sixp_nbr_get_next_seqno(nbr) == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_detect_seqno_error_1,
                   "test if seqno error is handled correctly (1)");
UNIT_TEST(test_detect_seqno_error_1)
{
  linkaddr_t peer_addr;
  uint32_t body;
  uint8_t *p;

  UNIT_TEST_BEGIN();

  test_setup();

  memset(&peer_addr, 0, sizeof(peer_addr));
  peer_addr.u8[0] = 1;
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID, 10,
                                   (const uint8_t *)&body,
                                   sizeof(body), NULL) == 0);

  /* return RC_ERR_RSEQNUM on receiving non-zero seqno when nbr doesn't exist */
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);
  /*
   * 2 octets for Termination 1 IE, one octet for 6top Sub-IE ID, and
   * 2 octets for Payload IE Header.
   */
  p = packetbuf_hdrptr() + 5;
  /* now, p pointes to the 6P header */
  UNIT_TEST_ASSERT(packetbuf_totlen() == 11);
  UNIT_TEST_ASSERT(p[0] == ((SIXP_PKT_TYPE_RESPONSE << 4) | SIXP_PKT_VERSION));
  UNIT_TEST_ASSERT(p[1] == SIXP_PKT_RC_ERR_SEQNUM);
  UNIT_TEST_ASSERT(p[2] == TEST_SF_SFID);
  UNIT_TEST_ASSERT(p[3] == 10);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_detect_seqno_error_2,
                   "test if seqno error is handled correctly (2)");
UNIT_TEST(test_detect_seqno_error_2)
{
  sixp_nbr_t *nbr;
  sixp_trans_t *trans;
  linkaddr_t peer_addr;
  uint32_t body;
  uint8_t *p;

  UNIT_TEST_BEGIN();

  test_setup();

  memset(&peer_addr, 0, sizeof(peer_addr));
  peer_addr.u8[0] = 1;
  UNIT_TEST_ASSERT((nbr = sixp_nbr_alloc(&peer_addr)) != NULL);
  UNIT_TEST_ASSERT(sixp_nbr_set_next_seqno(nbr, 3) == 0);
  UNIT_TEST_ASSERT(sixp_nbr_get_next_seqno(nbr) == 3);
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                   TEST_SF_SFID, 0,
                                   (const uint8_t *)&body,
                                   sizeof(body), NULL) == 0);

  /* return RC_ERR_RSEQNUM on receiving zero when nbr->seqno is non-zero */
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);
  /*
   * 2 octets for Termination 1 IE, one octet for 6top Sub-IE ID, and
   * 2 octets for Payload IE Header.
   */
  p = packetbuf_hdrptr() + 5;
  /* now, p pointes to the 6P header */
  UNIT_TEST_ASSERT(packetbuf_totlen() == 11);
  UNIT_TEST_ASSERT(p[0] == ((SIXP_PKT_TYPE_RESPONSE << 4) | SIXP_PKT_VERSION));
  UNIT_TEST_ASSERT(p[1] == SIXP_PKT_RC_ERR_SEQNUM);
  UNIT_TEST_ASSERT(p[2] == TEST_SF_SFID);
  UNIT_TEST_ASSERT(p[3] == 0);

  UNIT_TEST_ASSERT((trans = sixp_trans_find(&peer_addr)) != NULL);
  UNIT_TEST_ASSERT(sixp_trans_transit_state(trans,
                                            SIXP_TRANS_STATE_RESPONSE_SENT)
                   == 0);
  /*
   * next_seqno should be 1, the next value of 0, which is of the
   * received request
   */
  UNIT_TEST_ASSERT(sixp_nbr_get_next_seqno(nbr) == 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_initiator_after_initial_success_trans_1,
                   "test next_seqno on initiator after "
                   "an initial transaction of success [first half]");
UNIT_TEST(test_next_seqno_on_initiator_after_initial_success_trans_1)
{
  const uint8_t seqno_of_initial_trans = 0;
  const uint8_t cell_options = 0;
  const uint16_t num_cells = 0;
  const sixp_pkt_rc_t return_code = SIXP_PKT_RC_SUCCESS;

  UNIT_TEST_BEGIN();

  test_setup();

  /* we don't have a nbr to the peer at the beginning */
  UNIT_TEST_ASSERT(sixp_nbr_find(&peer_addr) == NULL);

  /* send a COUNT request to the peer */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) == NULL);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_REQUEST,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_COUNT,
                               TEST_SF_SFID,
                               &cell_options, sizeof(cell_options),
                               &peer_addr, NULL, NULL, 0) == 0);
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) != NULL);
  test_mac_invoke_sent_callback(MAC_TX_OK, 1);

  /*
   * inject a SUCCESS response to the initiator to complete the
   * transaction
   */
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_RESPONSE,
                                   (sixp_pkt_code_t)(uint8_t)return_code,
                                   TEST_SF_SFID, seqno_of_initial_trans,
                                   (const uint8_t *)&num_cells,
                                   sizeof(num_cells), NULL) == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_initiator_after_initial_success_trans_2,
                   "test next_seqno on initiator after "
                   "an initial transaction of success [second half]");
UNIT_TEST(test_next_seqno_on_initiator_after_initial_success_trans_2)
{
  const uint8_t seqno_of_initial_trans = 0;
  uint8_t next_seqno;
  sixp_nbr_t *nbr;

  UNIT_TEST_BEGIN();
  /*
   * test_next_seqno_on_initiator_after_initial_success_trans_1() is
   * expected to be called just before this function; we are just
   * after having a transaction complete with the peer.
   */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) == NULL);

  /*
   * initiator should keep the nbr for the peer after the transaction
   * completes
   */
  UNIT_TEST_ASSERT((nbr = sixp_nbr_find(&peer_addr)) != NULL);

  /* next_seqno should be one */
  next_seqno = sixp_nbr_get_next_seqno(nbr);
  printf("next_seqno: %u (expected to be 1)\n", next_seqno);
  UNIT_TEST_ASSERT(next_seqno == (seqno_of_initial_trans + 1));

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_initiator_after_initial_err_trans_1,
                   "test next_seqno on initiator after "
                   "an initial transaction of RC_ERR [first half]");
UNIT_TEST(test_next_seqno_on_initiator_after_initial_err_trans_1)
{
  const uint8_t seqno_of_initial_trans = 0;
  const uint8_t cell_options = 0;
  const sixp_pkt_rc_t return_code = SIXP_PKT_RC_ERR;

  UNIT_TEST_BEGIN();

  test_setup();

  /* we don't have a nbr to the peer at the beginning */
  UNIT_TEST_ASSERT(sixp_nbr_find(&peer_addr) == NULL);

  /* send a COUNT request to the peer */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) == NULL);
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_REQUEST,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_COUNT,
                               TEST_SF_SFID,
                               &cell_options, sizeof(cell_options),
                               &peer_addr, NULL, NULL, 0) == 0);
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) != NULL);
  test_mac_invoke_sent_callback(MAC_TX_OK, 1);

  /*
   * inject an ERR response to the initiator to complete the
   * transaction
   */
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_RESPONSE,
                                   (sixp_pkt_code_t)(uint8_t)return_code,
                                   TEST_SF_SFID, seqno_of_initial_trans,
                                   NULL, 0, NULL) == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_initiator_after_initial_err_trans_2,
                   "test next_seqno on initiator after "
                   "an initial transaction of RC_ERR [second half]");
UNIT_TEST(test_next_seqno_on_initiator_after_initial_err_trans_2)
{
  const uint8_t seqno_of_initial_trans = 0;
  uint8_t next_seqno;
  sixp_nbr_t *nbr;

  UNIT_TEST_BEGIN();
  /*
   * test_next_seqno_on_initiator_after_initial_err_trans_1() is
   * expected to be called just before this function; we are just
   * after having a transaction complete with the peer.
   */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) == NULL);

  /*
   * initiator should keep the nbr for the peer after the transaction
   * completes
   */
  UNIT_TEST_ASSERT((nbr = sixp_nbr_find(&peer_addr)) != NULL);

  /* next_seqno should be one */
  next_seqno = sixp_nbr_get_next_seqno(nbr);
  printf("next_seqno: %u (expected to be 1)\n", next_seqno);
  UNIT_TEST_ASSERT(next_seqno == (seqno_of_initial_trans + 1));

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_responder_after_initial_success_trans_1,
                   "test next_seqno on responder after "
                   "an initial transaction of success [first half]");
UNIT_TEST(test_next_seqno_on_responder_after_initial_success_trans_1)
{
  const uint8_t seqno_of_initial_trans = 0;
  const uint8_t request_body[3];
  const uint16_t num_cells = 0;
  const sixp_pkt_rc_t return_code = SIXP_PKT_RC_SUCCESS;

  UNIT_TEST_BEGIN();

  test_setup();

  /* we don't have a nbr to the peer at the beginning */
  UNIT_TEST_ASSERT(sixp_nbr_find(&peer_addr) == NULL);

  /* inject a COUNT request */
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_COUNT,
                                   TEST_SF_SFID, seqno_of_initial_trans,
                                   request_body, sizeof(request_body),
                                   NULL) == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);

  /* return a SUCCESS response to the peer */
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)return_code,
                               TEST_SF_SFID,
                               (const uint8_t *)&num_cells, sizeof(num_cells),
                               &peer_addr, NULL, NULL, 0) == 0);
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) != NULL);
  test_mac_invoke_sent_callback(MAC_TX_OK, 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_responder_after_initial_success_trans_2,
                   "test next_seqno on responder after "
                   "an initial transaction of success [second half]");
UNIT_TEST(test_next_seqno_on_responder_after_initial_success_trans_2)
{
  const uint8_t seqno_of_initial_trans = 0;
  uint8_t next_seqno;
  sixp_nbr_t *nbr;

  UNIT_TEST_BEGIN();
  /*
   * test_next_seqno_on_responder_after_initial_success_trans_1() is
   * expected to be called just before this function; we are just
   * after having a transaction complete with the peer.
   */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) == NULL);

  /*
   * responder should keep the nbr for the peer after the transaction
   * completes
   */
  UNIT_TEST_ASSERT((nbr = sixp_nbr_find(&peer_addr)) != NULL);

  /* next_seqno should be one */
  next_seqno = sixp_nbr_get_next_seqno(nbr);
  printf("next_seqno: %u (expected to be 1)\n", next_seqno);
  UNIT_TEST_ASSERT(next_seqno == (seqno_of_initial_trans + 1));

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_responder_after_initial_err_trans_1,
                   "test next_seqno on responder after "
                   "an initial transaction of RC_ERR [first half]");
UNIT_TEST(test_next_seqno_on_responder_after_initial_err_trans_1)
{
  const uint8_t seqno_of_initial_trans = 0;
  const uint8_t request_body[3];
  const uint16_t num_cells = 0;
  const sixp_pkt_rc_t return_code = SIXP_PKT_RC_ERR;

  UNIT_TEST_BEGIN();

  test_setup();

  /* we don't have a nbr to the peer at the beginning */
  UNIT_TEST_ASSERT(sixp_nbr_find(&peer_addr) == NULL);

  /* inject a COUNT request */
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_COUNT,
                                   TEST_SF_SFID, seqno_of_initial_trans,
                                   request_body, sizeof(request_body),
                                   NULL) == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);

  /* return an ERR response to the peer */
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)return_code,
                               TEST_SF_SFID,
                               (const uint8_t *)&num_cells, sizeof(num_cells),
                               &peer_addr, NULL, NULL, 0) == 0);
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) != NULL);
  test_mac_invoke_sent_callback(MAC_TX_OK, 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_responder_after_initial_err_trans_2,
                   "test next_seqno on responder after "
                   "an initial transaction of RC_ERR [second half]");
UNIT_TEST(test_next_seqno_on_responder_after_initial_err_trans_2)
{
  const uint8_t seqno_of_initial_trans = 0;
  uint8_t next_seqno;
  sixp_nbr_t *nbr;

  UNIT_TEST_BEGIN();
  /*
   * test_next_seqno_on_responder_after_initial_err_trans_1() is
   * expected to be called just before this function; we are just
   * after having a transaction complete with the peer.
   */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) == NULL);

  /*
   * responder should keep the nbr for the peer after the transaction
   * completes
   */
  UNIT_TEST_ASSERT((nbr = sixp_nbr_find(&peer_addr)) != NULL);

  /* next_seqno should be one */
  next_seqno = sixp_nbr_get_next_seqno(nbr);
printf("next_seqno: %u (expected to be 1)\n", next_seqno);
  UNIT_TEST_ASSERT(next_seqno == (seqno_of_initial_trans) + 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_responder_after_initial_err_sfid_trans_1,
                   "test next_seqno on responder after "
                   "an initial transaction of RC_ERR_SFID [first half]");
UNIT_TEST(test_next_seqno_on_responder_after_initial_err_sfid_trans_1)
{
  const uint8_t sfid = TEST_SF_SFID + 1;  /* invalid SFID */
  const uint8_t seqno_of_initial_trans = 0;
  const uint8_t request_body[3];

  UNIT_TEST_BEGIN();

  test_setup();

  /* we don't have a nbr to the peer at the beginning */
  UNIT_TEST_ASSERT(sixp_nbr_find(&peer_addr) == NULL);

  /* inject a COUNT request */
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_COUNT,
                                   sfid, seqno_of_initial_trans,
                                   request_body, sizeof(request_body),
                                   NULL) == 0);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);

  /* RC_ERR_SFID is sent automatically by 6P layer */
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 1);
  /*
   * when 6P performs auto-reply with RC_ERR_SFID, it doesn't generate
   * a trans to manage
   */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) == NULL);
  test_mac_invoke_sent_callback(MAC_TX_OK, 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_responder_after_initial_err_sfid_trans_2,
                   "test next_seqno on responder after "
                   "an initial transaction of RC_ERR_SFID [second half]");
UNIT_TEST(test_next_seqno_on_responder_after_initial_err_sfid_trans_2)
{
  UNIT_TEST_BEGIN();
  /*
   * test_next_seqno_on_responder_after_initial_err_sfid_trans_1() is
   * expected to be called just before this function; we are just
   * after having a transaction complete with the peer.
   */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) == NULL);

  /*
   * responder shouldn't have a nbr for the peer after the transaction
   * completes
   */
  UNIT_TEST_ASSERT(sixp_nbr_find(&peer_addr) == NULL);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_responder_after_initial_seqnum0_trans_1,
                   "test next_seqno on responder after "
                   "an initial transaction of RC_ERR_SEQNUM [first half]");
UNIT_TEST(test_next_seqno_on_responder_after_initial_seqnum0_trans_1)
{
  const uint8_t seqno_in_existing_nbr = 1;
  const uint8_t seqno_of_initial_trans = 0;
  const uint8_t request_body[3];
  sixp_nbr_t *nbr;

  UNIT_TEST_BEGIN();

  test_setup();

  /* we don't have a nbr to the peer at the beginning */
  UNIT_TEST_ASSERT(sixp_nbr_find(&peer_addr) == NULL);
  /* create a nbr and set next_seqno to 1 */
  UNIT_TEST_ASSERT((nbr = sixp_nbr_alloc(&peer_addr)) != NULL);
  UNIT_TEST_ASSERT(sixp_nbr_increment_next_seqno(nbr) == 0);
  UNIT_TEST_ASSERT(sixp_nbr_get_next_seqno(nbr) == seqno_in_existing_nbr);

  /* inject a COUNT request (SeqNum=0) */
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_COUNT,
                                   TEST_SF_SFID, seqno_of_initial_trans,
                                   request_body, sizeof(request_body),
                                   NULL) == 0);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);

  /* RC_ERR_SEQNUM is sent automatically by 6P layer */
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 1);
  /*
   * even when 6P performs auto-reply, if its return code is
   * RC_ERR_SFID, 6P layer creates a trans for the erroneous
   * transaction
   */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) != NULL);
  test_mac_invoke_sent_callback(MAC_TX_OK, 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_responder_after_initial_seqnum0_trans_2,
                   "test next_seqno on responder after "
                   "an initial transaction of RC_ERR_SEQNUM [second half]");
UNIT_TEST(test_next_seqno_on_responder_after_initial_seqnum0_trans_2)
{
  const uint8_t seqno_of_initial_trans = 0;
  uint8_t next_seqno;
  sixp_nbr_t *nbr;

  UNIT_TEST_BEGIN();
  /*
   * test_next_seqno_on_responder_after_initial_seqnum1_trans_1()
   * is expected to be called just before this function; we are just
   * after having a transaction complete with the peer.
   */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) == NULL);

  /*
   * in this case, responding with ERR_RC_SEQNUM, responder should
   * have a nbr for the peer after the transaction completes, which
   * has the right next_seqno
   */
  UNIT_TEST_ASSERT((nbr = sixp_nbr_find(&peer_addr)) != NULL);

  /*
   * since the initial request has SeqNum=0, next_seqno should remain
   * 1
   */
  next_seqno = sixp_nbr_get_next_seqno(nbr);
  printf("next_seqno: %u (expected to be 1)\n", next_seqno);
  UNIT_TEST_ASSERT(next_seqno == (seqno_of_initial_trans + 1));

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_responder_after_initial_seqnum1_trans_1,
                   "test next_seqno on responder after "
                   "an initial transaction of RC_ERR_SEQNUM [first half]");
UNIT_TEST(test_next_seqno_on_responder_after_initial_seqnum1_trans_1)
{
  const uint8_t seqno_of_initial_trans = 1;
  const uint8_t request_body[3];

  UNIT_TEST_BEGIN();

  test_setup();

  /* we don't have a nbr to the peer at the beginning */
  UNIT_TEST_ASSERT(sixp_nbr_find(&peer_addr) == NULL);

  /* inject a COUNT request (SeqNum=1) */
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_COUNT,
                                   TEST_SF_SFID, seqno_of_initial_trans,
                                   request_body, sizeof(request_body),
                                   NULL) == 0);
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);

  /* RC_ERR_SEQNUM is sent automatically by 6P layer */
  UNIT_TEST_ASSERT(test_mac_send_function_is_called() == 1);
  /*
   * even when 6P performs auto-reply, if its return code is
   * RC_ERR_SFID, 6P layer creates a trans for the erroneous
   * transaction
   */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) != NULL);
  test_mac_invoke_sent_callback(MAC_TX_OK, 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_next_seqno_on_responder_after_initial_seqnum1_trans_2,
                   "test next_seqno on responder after "
                   "an initial transaction of RC_ERR_SEQNUM [second half]");
UNIT_TEST(test_next_seqno_on_responder_after_initial_seqnum1_trans_2)
{
  const uint8_t seqno_of_initial_trans = 1;
  uint8_t next_seqno;
  sixp_nbr_t *nbr;

  UNIT_TEST_BEGIN();
  /*
   * test_next_seqno_on_responder_after_initial_seqnum1_trans_1()
   * is expected to be called just before this function; we are just
   * after having a transaction complete with the peer.
   */
  UNIT_TEST_ASSERT(sixp_trans_find(&peer_addr) == NULL);

  /*
   * in this case, responding with ERR_RC_SEQNUM, responder should
   * have a nbr for the peer after the transaction completes, which
   * has the right next_seqno
   */
  UNIT_TEST_ASSERT((nbr = sixp_nbr_find(&peer_addr)) != NULL);

  /* since the initial request has SeqNum=1, next_seqno should be 2 */
  next_seqno = sixp_nbr_get_next_seqno(nbr);
  printf("next_seqno: %u (expected to be 2)\n", sixp_nbr_get_next_seqno(nbr));
  UNIT_TEST_ASSERT(next_seqno == (seqno_of_initial_trans + 1));

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_invalid_version,
                   "test invalid version");
UNIT_TEST(test_invalid_version)
{
  linkaddr_t peer_addr;
  uint8_t *p;
  sixp_pkt_t pkt;

  UNIT_TEST_BEGIN();

  test_setup();

  memset(&peer_addr, 0, sizeof(peer_addr));
  peer_addr.u8[0] = 1;
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_CLEAR,
                                   TEST_SF_SFID, 0,
                                   NULL, 0, NULL) == 0);
  p = packetbuf_hdrptr();
  p[0] |= 10; /* set version 10 which we doesn't support */
  /* this 6P packet shouldn't be parsed */
  UNIT_TEST_ASSERT(sixp_pkt_parse(packetbuf_hdrptr(), packetbuf_totlen(),
                                  &pkt) == -1);

  /* return RC_ERR_VERSION on receiving zero when nbr->seqno is non-zero */
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &peer_addr);
  /*
   * 2 octets for Termination 1 IE, one octet for 6top Sub-IE ID, and
   * 2 octets for Payload IE Header.
   */
  p = packetbuf_hdrptr() + 5;
  /* now, p pointes to the 6P header */
  UNIT_TEST_ASSERT(packetbuf_totlen() == 11);
  UNIT_TEST_ASSERT(p[0] == ((SIXP_PKT_TYPE_RESPONSE << 4) | SIXP_PKT_VERSION));
  UNIT_TEST_ASSERT(p[1] == SIXP_PKT_RC_ERR_VERSION);
  UNIT_TEST_ASSERT(p[2] == TEST_SF_SFID);
  UNIT_TEST_ASSERT(p[3] == 0);

  UNIT_TEST_END();
}

PROCESS_THREAD(test_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  /* wait until the sixtop module gets ready */
  etimer_set(&et, CLOCK_SECOND);
  tschmac_driver.init();
  tschmac_driver.on();
  tsch_set_coordinator(1);
  while(tsch_is_associated == 0) {
    PROCESS_YIELD_UNTIL(etimer_expired(&et));
    etimer_reset(&et);
  }

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(test_input_no_sf);
  UNIT_TEST_RUN(test_input_busy);
  UNIT_TEST_RUN(test_input_no_memory);

  UNIT_TEST_RUN(test_output_request_1);
  UNIT_TEST_RUN(test_output_request_2);
  UNIT_TEST_RUN(test_output_response_1);
  UNIT_TEST_RUN(test_output_response_2);
  UNIT_TEST_RUN(test_output_response_3);
  UNIT_TEST_RUN(test_output_response_4);
  UNIT_TEST_RUN(test_output_confirmation_1);
  UNIT_TEST_RUN(test_output_confirmation_2);
  UNIT_TEST_RUN(test_output_confirmation_3);
  UNIT_TEST_RUN(test_output_confirmation_4);

  /* testing for SeqNum Management */
  UNIT_TEST_RUN(test_next_seqno_reset_by_clear_request_1);
  UNIT_TEST_RUN(test_next_seqno_reset_by_clear_request_2);
  UNIT_TEST_RUN(test_detect_seqno_error_1);
  UNIT_TEST_RUN(test_detect_seqno_error_2);

  UNIT_TEST_RUN(test_next_seqno_on_initiator_after_initial_success_trans_1);
  YIELD_CPU_FOR_TIMER_TASKS(et);
  UNIT_TEST_RUN(test_next_seqno_on_initiator_after_initial_success_trans_2);

  UNIT_TEST_RUN(test_next_seqno_on_initiator_after_initial_err_trans_1);
  YIELD_CPU_FOR_TIMER_TASKS(et);
  UNIT_TEST_RUN(test_next_seqno_on_initiator_after_initial_err_trans_2);

  UNIT_TEST_RUN(test_next_seqno_on_responder_after_initial_success_trans_1);
  YIELD_CPU_FOR_TIMER_TASKS(et);
  UNIT_TEST_RUN(test_next_seqno_on_responder_after_initial_success_trans_2);

  UNIT_TEST_RUN(test_next_seqno_on_responder_after_initial_err_trans_1);
  YIELD_CPU_FOR_TIMER_TASKS(et);
  UNIT_TEST_RUN(test_next_seqno_on_responder_after_initial_err_trans_2);

  UNIT_TEST_RUN(test_next_seqno_on_responder_after_initial_err_sfid_trans_1);
  YIELD_CPU_FOR_TIMER_TASKS(et);
  UNIT_TEST_RUN(test_next_seqno_on_responder_after_initial_err_sfid_trans_2);

  UNIT_TEST_RUN(test_next_seqno_on_responder_after_initial_seqnum0_trans_1);
  YIELD_CPU_FOR_TIMER_TASKS(et);
  UNIT_TEST_RUN(test_next_seqno_on_responder_after_initial_seqnum0_trans_2);

  UNIT_TEST_RUN(test_next_seqno_on_responder_after_initial_seqnum1_trans_1);
  YIELD_CPU_FOR_TIMER_TASKS(et);
  UNIT_TEST_RUN(test_next_seqno_on_responder_after_initial_seqnum1_trans_2);

  UNIT_TEST_RUN(test_invalid_version);

  printf("=check-me= DONE\n");
  PROCESS_END();
}
