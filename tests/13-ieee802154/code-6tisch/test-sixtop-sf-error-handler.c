/*
 * Copyright (c) 2019, Yasuyuki Tanaka
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
 * ARISING IN ANY STEP OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"
#include "lib/assert.h"

#include "net/packetbuf.h"
#include "net/mac/tsch/sixtop/sixtop.h"
#include "net/mac/tsch/sixtop/sixp.h"
#include "net/mac/tsch/sixtop/sixp-nbr.h"
#include "net/mac/tsch/sixtop/sixp-trans.h"

#include "unit-test/unit-test.h"
#include "common.h"

#define UNKNOWN_SF_SFID 0
#define TEST_SF_SFID 0xf1

static uint8_t input_handler_is_called;
static uint8_t error_handler_is_called;
static sixp_error_t returned_err;
static sixp_pkt_type_t returned_type;
static sixp_pkt_code_t returned_code;
static uint8_t returned_seqno;
static linkaddr_t returned_peer_addr;

static const linkaddr_t test_peer_addr = {
  {0x02, 0x00, 0xca, 0xfe, 0xc0, 0xca, 0xbe, 0xef}
};

static void
input(sixp_pkt_type_t type, sixp_pkt_code_t code, const uint8_t *body,
      uint16_t body_len, const linkaddr_t *peer_addr)
{
  input_handler_is_called = 1;
  returned_type = type;
  returned_code = code;
  memcpy(&returned_peer_addr, peer_addr, sizeof(linkaddr_t));
}

static void
error(sixp_error_t err,
      sixp_pkt_cmd_t cmd,
      uint8_t seqno,
      const linkaddr_t *peer_addr)
{
  error_handler_is_called = 1;
  returned_err = err;
  returned_code = (sixp_pkt_code_t)(uint8_t)cmd;
  returned_seqno = seqno;
  assert(peer_addr != NULL);
  memcpy(&returned_peer_addr, peer_addr, sizeof(returned_peer_addr));
}

static const sixtop_sf_t test_sf = {
  TEST_SF_SFID,
  0,      /* timeout_value */
  NULL,   /* init */
  input,  /* input */
  NULL,   /* timeout */
  error   /* error */
};

PROCESS(test_process, "6top SF error handler test");
AUTOSTART_PROCESSES(&test_process);

static void
send_request(void)
{
  uint32_t body = 0;
  int ret;
  ret = sixp_output(SIXP_PKT_TYPE_REQUEST,
                    (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                    TEST_SF_SFID,
                    (const uint8_t *)&body, sizeof(body),
                    &test_peer_addr,
                    NULL, NULL, 0);
  assert(ret == 0);
}

static void
test_setup(void)
{
  test_mac_driver.init();
  sixtop_init();
  packetbuf_clear();
  sixtop_add_sf(&test_sf);

  input_handler_is_called = 0;
  error_handler_is_called = 0;
  returned_err = SIXP_ERROR_UNDEFINED;
  returned_type = SIXP_PKT_TYPE_RESERVED;
  returned_code = (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_UNAVAILABLE;
  returned_seqno = 255;
  memset(&returned_peer_addr, 0, sizeof(returned_peer_addr));
}

UNIT_TEST_REGISTER(test_schedule_inconsistency_on_initiator,
                   "test schedule inconsistency notification on initiator");
UNIT_TEST(test_schedule_inconsistency_on_initiator)
{
  sixp_nbr_t *nbr;
  sixp_trans_t *trans;
  const uint8_t initial_seqno = 100;
  sixp_pkt_code_t return_code;

  UNIT_TEST_BEGIN();
  test_setup();

  /* add a sixp_nbr_t for the peer and set the initial seqno */
  nbr = sixp_nbr_alloc(&test_peer_addr);
  UNIT_TEST_ASSERT(nbr != NULL);
  UNIT_TEST_ASSERT(sixp_nbr_set_next_seqno(nbr, initial_seqno) == 0);
  UNIT_TEST_ASSERT(sixp_nbr_get_next_seqno(nbr) == initial_seqno);

  /* send a request; change the transaction state */
  UNIT_TEST_ASSERT(sixp_trans_find(&test_peer_addr) == NULL);
  send_request();
  UNIT_TEST_ASSERT((trans = sixp_trans_find(&test_peer_addr)) != NULL);
  UNIT_TEST_ASSERT(sixp_trans_transit_state(trans,
                                            SIXP_TRANS_STATE_REQUEST_SENT) == 0);
  UNIT_TEST_ASSERT(sixp_trans_get_state(trans) == SIXP_TRANS_STATE_REQUEST_SENT);

  /* give a response with RC_ERR_SEQNUM, having SeqNum 100 */
  packetbuf_clear();
  return_code = (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_ERR_SEQNUM;
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_RESPONSE, return_code,
                                   TEST_SF_SFID, initial_seqno,
                                   NULL, 0, NULL) == 0);
  UNIT_TEST_ASSERT(input_handler_is_called == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &test_peer_addr);

  /*
   * the error handler should not be called; instead, the input
   * handler should be called with right argument values
   */
  UNIT_TEST_ASSERT(error_handler_is_called == 0);
  UNIT_TEST_ASSERT(input_handler_is_called == 1);
  UNIT_TEST_ASSERT(returned_type == SIXP_PKT_TYPE_RESPONSE);
  UNIT_TEST_ASSERT(returned_code.value == SIXP_PKT_RC_ERR_SEQNUM);
  UNIT_TEST_ASSERT(memcmp(&returned_peer_addr,
                          &test_peer_addr,
                          sizeof(linkaddr_t)) == 0);
  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_schedule_inconsistency_on_responder,
                   "test schedule inconsistency notification on responder");
UNIT_TEST(test_schedule_inconsistency_on_responder)
{
  sixp_pkt_code_t cmd;
  sixp_pkt_metadata_t metadata;
  const uint8_t test_seqno = 100;

  UNIT_TEST_BEGIN();
  test_setup();

  /* make sure we don't have a nbr for test_peer_addr */
  UNIT_TEST_ASSERT(sixp_nbr_find(&test_peer_addr) == NULL);

  /* give a request having SeqNum 100 */
  cmd = (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_SIGNAL;
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST, cmd,
                                   TEST_SF_SFID, test_seqno,
                                   (const uint8_t *)&metadata, sizeof(metadata),
                                   NULL) == 0);
  /* schedule inconsistency should be caught by the error handler  */
  UNIT_TEST_ASSERT(error_handler_is_called == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &test_peer_addr);
  UNIT_TEST_ASSERT(error_handler_is_called == 1);
  UNIT_TEST_ASSERT(returned_err == SIXP_ERROR_SCHEDULE_INCONSISTENCY);
  UNIT_TEST_ASSERT(returned_code.value == SIXP_PKT_CMD_SIGNAL);
  UNIT_TEST_ASSERT(returned_seqno == test_seqno);
  UNIT_TEST_ASSERT(memcmp(&returned_peer_addr,
                          &test_peer_addr,
                          sizeof(linkaddr_t)) == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_mac_callback_on_terminated_trans_on_initiator_req,
                   "test mac_callback on a terminated trans (initiator/req)");
UNIT_TEST(test_mac_callback_on_terminated_trans_on_initiator_req)
{
  sixp_trans_t *trans;

  UNIT_TEST_BEGIN();
  test_setup();

  /* create a transaction */
  UNIT_TEST_ASSERT(sixp_trans_find(&test_peer_addr) == NULL);
  send_request();
  UNIT_TEST_ASSERT((trans = sixp_trans_find(&test_peer_addr)) != NULL);

  /* free the transaction; "free" operation should be deferred */
  UNIT_TEST_ASSERT(sixp_trans_get_state(trans) ==
                   SIXP_TRANS_STATE_REQUEST_SENDING);
  sixp_trans_free(trans);
  UNIT_TEST_ASSERT(sixp_trans_find(&test_peer_addr) == NULL);
  UNIT_TEST_ASSERT(sixp_trans_get_state(trans) == SIXP_TRANS_STATE_WAIT_FREE);

  /* call mac_callback, which is called by TSCH layer in a normal operation */
  UNIT_TEST_ASSERT(error_handler_is_called == 0);
  test_mac_invoke_sent_callback(MAC_TX_OK, 1);

  /*
   * error handler should be called with
   * SIXP_ERROR_TX_AFTER_TRANSACTION_TERMINATION
   */
  UNIT_TEST_ASSERT(error_handler_is_called == 1);
  UNIT_TEST_ASSERT(returned_err == SIXP_ERROR_TX_AFTER_TRANSACTION_TERMINATION);
  UNIT_TEST_ASSERT(returned_code.value == SIXP_PKT_CMD_ADD);
  UNIT_TEST_ASSERT(returned_seqno == 0);
  UNIT_TEST_ASSERT(memcmp(&returned_peer_addr,
                          &test_peer_addr,
                          sizeof(linkaddr_t)) == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_mac_callback_on_terminated_trans_on_responder_res,
                   "test mac_callback on a terminated trans (responder/res)");
UNIT_TEST(test_mac_callback_on_terminated_trans_on_responder_res)
{
  sixp_pkt_cmd_t test_cmd;
  const uint8_t test_seqno = 0;
  sixp_pkt_metadata_t metadata;
  sixp_trans_t *trans;

  UNIT_TEST_BEGIN();
  test_setup();

  /* give a request having SeqNum 0 */
  UNIT_TEST_ASSERT(sixp_trans_find(&test_peer_addr) == NULL);
  test_cmd = SIXP_PKT_CMD_SIGNAL;
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)test_cmd,
                                   TEST_SF_SFID, test_seqno,
                                   (const uint8_t *)&metadata, sizeof(metadata),
                                   NULL) == 0);
  UNIT_TEST_ASSERT(input_handler_is_called == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &test_peer_addr);
  UNIT_TEST_ASSERT(input_handler_is_called == 1);
  UNIT_TEST_ASSERT((trans = sixp_trans_find(&test_peer_addr)) != NULL);

  /* send back a response */
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               TEST_SF_SFID, NULL, 0,
                               &test_peer_addr, NULL, NULL, 0) == 0);
  /* free the transaction */
  sixp_trans_free(trans);
  UNIT_TEST_ASSERT(sixp_trans_find(&test_peer_addr) == NULL);
  UNIT_TEST_ASSERT(sixp_trans_get_state(trans) == SIXP_TRANS_STATE_WAIT_FREE);

  /* call mac_callback */
  UNIT_TEST_ASSERT(error_handler_is_called == 0);
  test_mac_invoke_sent_callback(MAC_TX_OK, 1);

  /*
   * error handler should be called with
   * SIXP_ERROR_TX_AFTER_TRANSACTION_TERMINATION
   */
  UNIT_TEST_ASSERT(error_handler_is_called == 1);
  UNIT_TEST_ASSERT(returned_err == SIXP_ERROR_TX_AFTER_TRANSACTION_TERMINATION);
  UNIT_TEST_ASSERT(returned_code.value == test_cmd);
  UNIT_TEST_ASSERT(returned_seqno == test_seqno);
  UNIT_TEST_ASSERT(memcmp(&returned_peer_addr,
                          &test_peer_addr,
                          sizeof(linkaddr_t)) == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_mac_callback_on_terminated_trans_on_initiator_conf,
                   "test mac_callback on a terminated trans (initiator/conf)");
UNIT_TEST(test_mac_callback_on_terminated_trans_on_initiator_conf)
{
  sixp_trans_t *trans;

  UNIT_TEST_BEGIN();
  test_setup();

  /* create a transaction */
  UNIT_TEST_ASSERT(sixp_trans_find(&test_peer_addr) == NULL);
  send_request();
  UNIT_TEST_ASSERT((trans = sixp_trans_find(&test_peer_addr)) != NULL);

  /* receive a response */
  test_mac_invoke_sent_callback(MAC_TX_OK, 1);
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_RESPONSE,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                                   TEST_SF_SFID, 0, NULL, 0, NULL) == 0);
  UNIT_TEST_ASSERT(input_handler_is_called == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &test_peer_addr);

  /* send a confirmation */
  UNIT_TEST_ASSERT(sixp_output(SIXP_PKT_TYPE_CONFIRMATION,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               TEST_SF_SFID, NULL, 0,
                               &test_peer_addr, NULL, NULL, 0) == 0);

  /* free the transaction; "free" operation should be deferred */
  UNIT_TEST_ASSERT(sixp_trans_get_state(trans) ==
                   SIXP_TRANS_STATE_CONFIRMATION_SENDING);
  sixp_trans_free(trans);
  UNIT_TEST_ASSERT(sixp_trans_find(&test_peer_addr) == NULL);
  UNIT_TEST_ASSERT(sixp_trans_get_state(trans) == SIXP_TRANS_STATE_WAIT_FREE);

  /* call mac_callback, which is called by TSCH layer in a normal operation */
  UNIT_TEST_ASSERT(error_handler_is_called == 0);
  test_mac_invoke_sent_callback(MAC_TX_OK, 1);

  /*
   * error handler should be called with
   * SIXP_ERROR_TX_AFTER_TRANSACTION_TERMINATION
   */
  UNIT_TEST_ASSERT(error_handler_is_called == 1);
  UNIT_TEST_ASSERT(returned_err == SIXP_ERROR_TX_AFTER_TRANSACTION_TERMINATION);
  UNIT_TEST_ASSERT(returned_code.value == SIXP_PKT_CMD_ADD);
  UNIT_TEST_ASSERT(returned_seqno == 0);
  UNIT_TEST_ASSERT(memcmp(&returned_peer_addr,
                          &test_peer_addr,
                          sizeof(linkaddr_t)) == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_invalid_trans_state_transition,
                   "test invalid transaction state transition");
UNIT_TEST(test_invalid_trans_state_transition)
{
  sixp_trans_t *trans;

  UNIT_TEST_BEGIN();
  test_setup();

  /* allocate a trans by sending a request */
  UNIT_TEST_ASSERT(sixp_trans_find(&test_peer_addr) == NULL);
  send_request();
  UNIT_TEST_ASSERT((trans = sixp_trans_find(&test_peer_addr)) != NULL);

  /* the state of the trans should be REQUEST_SENDING */
  UNIT_TEST_ASSERT(sixp_trans_get_state(trans) ==
                   SIXP_TRANS_STATE_REQUEST_SENDING);

  /* change the state to CONFIRMATION SENDING */
  UNIT_TEST_ASSERT(error_handler_is_called == 0);
  UNIT_TEST_ASSERT(
    sixp_trans_transit_state(trans,
                             SIXP_TRANS_STATE_CONFIRMATION_SENDING) < 0);
  UNIT_TEST_ASSERT(error_handler_is_called == 1);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_invalid_6p_version,
                   "test invalid 6P version");
UNIT_TEST(test_invalid_6p_version)
{
  sixp_pkt_metadata_t metadata;
  uint8_t *p;

  UNIT_TEST_BEGIN();
  test_setup();

  /* create a test message */
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_CLEAR,
                                   TEST_SF_SFID, 0,
                                   (const uint8_t *)&metadata, sizeof(metadata),
                                   NULL) == 0);
  p = packetbuf_hdrptr();
  p[0] |= 10; /* set version 10 which we doesn't support */

  /* give this message to the node */
  UNIT_TEST_ASSERT(input_handler_is_called == 0);
  UNIT_TEST_ASSERT(error_handler_is_called == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &test_peer_addr);

  /* neither input_handler nor error_handler shouldn't be called */
  UNIT_TEST_ASSERT(input_handler_is_called == 0);
  UNIT_TEST_ASSERT(error_handler_is_called == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_invalid_6p_type,
                   "test invalid 6P type");
UNIT_TEST(test_invalid_6p_type)
{
  sixp_pkt_metadata_t metadata;
  uint8_t *p;

  UNIT_TEST_BEGIN();
  test_setup();

  /* create a test message */
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_CLEAR,
                                   TEST_SF_SFID, 0,
                                   (const uint8_t *)&metadata, sizeof(metadata),
                                   NULL) == 0);
  p = packetbuf_hdrptr();
  p[0] |= SIXP_PKT_TYPE_RESERVED << 4; /* set invalid type */

  /* give this message to the node */
  UNIT_TEST_ASSERT(input_handler_is_called == 0);
  UNIT_TEST_ASSERT(error_handler_is_called == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &test_peer_addr);

  /* neither input_handler nor error_handler shouldn't be called */
  UNIT_TEST_ASSERT(input_handler_is_called == 0);
  UNIT_TEST_ASSERT(error_handler_is_called == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_invalid_6p_cmd,
                   "test invalid 6P cmd");
UNIT_TEST(test_invalid_6p_cmd)
{
  sixp_pkt_metadata_t metadata;
  uint8_t *p;

  UNIT_TEST_BEGIN();
  test_setup();

  /* create a test message */
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_CLEAR,
                                   TEST_SF_SFID, 0,
                                   (const uint8_t *)&metadata, sizeof(metadata),
                                   NULL) == 0);
  p = packetbuf_hdrptr();
  p[1] = SIXP_PKT_CMD_UNAVAILABLE; /* set invalid type */

  /* give this message to the node */
  UNIT_TEST_ASSERT(input_handler_is_called == 0);
  UNIT_TEST_ASSERT(error_handler_is_called == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &test_peer_addr);

  /* neither input_handler nor error_handler shouldn't be called */
  UNIT_TEST_ASSERT(input_handler_is_called == 0);
  UNIT_TEST_ASSERT(error_handler_is_called == 0);

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(test_invalid_6p_rc,
                   "test invalid 6P return code");
UNIT_TEST(test_invalid_6p_rc)
{
  uint8_t *p;

  UNIT_TEST_BEGIN();
  test_setup();

  /* create a test message */
  packetbuf_clear();
  UNIT_TEST_ASSERT(sixp_pkt_create(SIXP_PKT_TYPE_RESPONSE,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                                   TEST_SF_SFID, 0,
                                   NULL, 0, NULL) == 0);
  p = packetbuf_hdrptr();
  p[1] = SIXP_PKT_RC_RESERVED; /* set invalid rc */

  /* give this message to the node */
  UNIT_TEST_ASSERT(input_handler_is_called == 0);
  UNIT_TEST_ASSERT(error_handler_is_called == 0);
  sixp_input(packetbuf_hdrptr(), packetbuf_totlen(), &test_peer_addr);

  /* neither input_handler nor error_handler shouldn't be called */
  UNIT_TEST_ASSERT(input_handler_is_called == 0);
  UNIT_TEST_ASSERT(error_handler_is_called == 0);

  UNIT_TEST_END();
}

PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();
  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(test_schedule_inconsistency_on_initiator);
  UNIT_TEST_RUN(test_schedule_inconsistency_on_responder);
  UNIT_TEST_RUN(test_mac_callback_on_terminated_trans_on_initiator_req);
  UNIT_TEST_RUN(test_mac_callback_on_terminated_trans_on_responder_res);
  UNIT_TEST_RUN(test_mac_callback_on_terminated_trans_on_initiator_conf);
  UNIT_TEST_RUN(test_invalid_trans_state_transition);

  /* error handler shouldn't be called in the following tests */
  UNIT_TEST_RUN(test_invalid_6p_version);
  UNIT_TEST_RUN(test_invalid_6p_type);
  UNIT_TEST_RUN(test_invalid_6p_cmd);
  UNIT_TEST_RUN(test_invalid_6p_rc);

  printf("=check-me= DONE\n");
  PROCESS_END();
}
