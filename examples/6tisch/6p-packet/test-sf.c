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

#include <contiki.h>
#include <lib/assert.h>
#include <net/mac/tsch/tsch.h>
#include <net/mac/tsch/sixtop/sixtop.h>
#include <net/mac/tsch/sixtop/sixp.h>
#include <net/mac/tsch/sixtop/sixp-nbr.h>
#include <net/mac/tsch/sixtop/sixp-trans.h>

#define DEBUG DEBUG_PRINT
#include <net/net-debug.h>

#define SIXP_PKT_BUFLEN     128
static uint8_t sixp_pkt_buf[SIXP_PKT_BUFLEN];

#define TEST_SF_SFID        0x80
#define TEST_SF_TIMEOUT     CLOCK_SECOND

PROCESS(test_sf_process, "test-sf initiator process");
static linkaddr_t peer_addr;
static process_event_t test_sf_trans_done;
static enum { TEST_SF_MODE_INITIATOR, TEST_SF_MODE_RESPONDER } test_sf_mode;
static int test_index = 0;

static const sixp_pkt_metadata_t sample_metadata = 0xcafe;
static const uint32_t sample_cell_list1[] = { 0xdeadbeef,
                                              0xcafebabe,
                                              0xbaadf00d };
static const uint32_t sample_cell_list2[] = { 0xbaadcafe,
                                              0xfacefeed,
                                              0xbadcab1e };

static void test_add_2_step(const linkaddr_t *peer_addr);
static void test_add_3_step(const linkaddr_t *peer_addr);
static void test_delete_2_step(const linkaddr_t *peer_addr);
static void test_delete_3_step(const linkaddr_t *peer_addr);
static void test_relocate_2_step(const linkaddr_t *peer_addr);
static void test_count_2_step(const linkaddr_t *peer_addr);
static void test_list_2_step(const linkaddr_t *peer_addr);
static void test_list_2_step_eol(const linkaddr_t *peer_addr);
static void test_signal_2_step(const linkaddr_t *peer_addr);
static void test_clear_2_step_success(const linkaddr_t *peer_addr);
static void test_clear_2_step_eol(const linkaddr_t *peer_addr);
static void test_clear_2_step_err(const linkaddr_t *peer_addr);
static void test_clear_2_step_reset(const linkaddr_t *peer_addr);
static void test_clear_2_step_version(const linkaddr_t *peer_addr);
static void test_clear_2_step_sfid(const linkaddr_t *peer_addr);
static void test_clear_2_step_seqnum(const linkaddr_t *peer_addr);
static void test_clear_2_step_celllist(const linkaddr_t *peer_addr);
static void test_clear_2_step_busy(const linkaddr_t *peer_addr);
static void test_clear_2_step_locked(const linkaddr_t *peer_addr);

typedef void (*test_func)(const linkaddr_t *peer_addr);
static const test_func test_case[] = {
  test_add_2_step,
  test_add_3_step,
  test_delete_2_step,
  test_delete_3_step,
  test_relocate_2_step,
  test_count_2_step,
  test_list_2_step,
  test_list_2_step_eol,
  test_signal_2_step,
  test_clear_2_step_success,
  test_clear_2_step_eol,
  test_clear_2_step_err,
  test_clear_2_step_reset,
  test_clear_2_step_version,
  test_clear_2_step_sfid,
  test_clear_2_step_seqnum,
  test_clear_2_step_celllist,
  test_clear_2_step_busy,
  test_clear_2_step_locked,
};

static void
set_peer_addr(const linkaddr_t *addr)
{
  peer_addr = *addr;
  PRINTF("test-sf: set peer addr: ");
  PRINTLLADDR((const uip_lladdr_t *)&peer_addr);
  PRINTF("\n");
}

int
test_sf_start(const linkaddr_t *addr)
{
  if(addr == NULL) {
    test_sf_mode = TEST_SF_MODE_RESPONDER;
  } else {
    test_sf_mode = TEST_SF_MODE_INITIATOR;
    set_peer_addr(addr);
    test_sf_trans_done = process_alloc_event();
    process_start(&test_sf_process, NULL);
  }
  return 0;
}

static void
init(void)
{
}

static void
input(sixp_pkt_type_t type, sixp_pkt_code_t code,
      const uint8_t *body, uint16_t body_len,
      const linkaddr_t *src_addr)
{
  if(test_index < (sizeof(test_case) / sizeof(test_func))) {
    test_case[test_index](src_addr);
  }
}

static void
timeout(sixp_pkt_cmd_t cmd, const linkaddr_t *peer_addr)
{
}

static void
test_add_2_step(const linkaddr_t *peer_addr)
{
  sixp_trans_t *trans = sixp_trans_find(peer_addr);

  if(trans == NULL) {
    memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
    assert(sixp_pkt_set_metadata(SIXP_PKT_TYPE_REQUEST,
                                 (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                 sample_metadata,
                                 sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_pkt_set_cell_options(SIXP_PKT_TYPE_REQUEST,
                                     (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                     SIXP_PKT_CELL_OPTION_TX |
                                     SIXP_PKT_CELL_OPTION_SHARED,
                                     sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_pkt_set_num_cells(SIXP_PKT_TYPE_REQUEST,
                                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                  1,
                                  sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_pkt_set_cell_list(SIXP_PKT_TYPE_REQUEST,
                                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                  (const uint8_t *)sample_cell_list1,
                                  sizeof(sample_cell_list1), 0,
                                  sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                       TEST_SF_SFID, sixp_pkt_buf,
                       sizeof(sixp_pkt_cell_options_t) +
                       sizeof(sixp_pkt_num_cells_t) +
                       sizeof(sixp_pkt_metadata_t) +
                       sizeof(sample_cell_list1),
                       peer_addr,
                       NULL, NULL, 0) == 0);
  } else {
    sixp_trans_state_t state;
    state = sixp_trans_get_state(trans);
    if(state == SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      assert(
        sixp_pkt_set_cell_list(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               (const uint8_t *)sample_cell_list1,
                               sizeof(uint32_t), 0,
                               sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      sixp_output(SIXP_PKT_TYPE_RESPONSE,
                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                  TEST_SF_SFID, sixp_pkt_buf, sizeof(uint32_t),
                  peer_addr,
                  NULL, NULL, 0);
      test_index++;
    } else if(state == SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      process_post(&test_sf_process, test_sf_trans_done, NULL);
    }
  }
}

static void
test_add_3_step(const linkaddr_t *peer_addr)
{
  sixp_trans_t *trans = sixp_trans_find(peer_addr);

  if(trans == NULL) {
    memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
    assert(sixp_pkt_set_metadata(SIXP_PKT_TYPE_REQUEST,
                                 (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                 sample_metadata,
                                 sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_pkt_set_cell_options(SIXP_PKT_TYPE_REQUEST,
                                     (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                     SIXP_PKT_CELL_OPTION_TX |
                                     SIXP_PKT_CELL_OPTION_SHARED,
                                     sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_pkt_set_num_cells(SIXP_PKT_TYPE_REQUEST,
                                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                                  1,
                                  sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                       TEST_SF_SFID, sixp_pkt_buf,
                       sizeof(sixp_pkt_cell_options_t) +
                       sizeof(sixp_pkt_num_cells_t) +
                       sizeof(sixp_pkt_metadata_t),
                       peer_addr,
                       NULL, NULL, 0) == 0);
  } else {
    sixp_trans_state_t state;
    state = sixp_trans_get_state(trans);
    if(state == SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      assert(
        sixp_pkt_set_cell_list(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               (const uint8_t *)sample_cell_list1,
                               sizeof(sample_cell_list1), 0,
                               sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      sixp_output(SIXP_PKT_TYPE_RESPONSE,
                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                  TEST_SF_SFID, sixp_pkt_buf, sizeof(sample_cell_list1),
                  peer_addr,
                  NULL, NULL, 0);
    } else if (state == SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      assert(
        sixp_pkt_set_cell_list(SIXP_PKT_TYPE_CONFIRMATION,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               (const uint8_t *)sample_cell_list1,
                               sizeof(uint32_t), 0,
                               sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      sixp_output(SIXP_PKT_TYPE_CONFIRMATION,
                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                  TEST_SF_SFID, sixp_pkt_buf, sizeof(uint32_t),
                  peer_addr,
                  NULL, NULL, 0);
      process_post(&test_sf_process, test_sf_trans_done, NULL);
    } else if (state == SIXP_TRANS_STATE_CONFIRMATION_RECEIVED){
      test_index++;
    }
  }
}

static void
test_delete_2_step(const linkaddr_t *peer_addr)
{
  sixp_trans_t *trans = sixp_trans_find(peer_addr);

  if(trans == NULL) {
    memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
    assert(sixp_pkt_set_metadata(SIXP_PKT_TYPE_REQUEST,
                                 (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE,
                                 sample_metadata,
                                 sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(
      sixp_pkt_set_cell_options(SIXP_PKT_TYPE_REQUEST,
                                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE,
                                SIXP_PKT_CELL_OPTION_TX |
                                SIXP_PKT_CELL_OPTION_SHARED,
                                sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_pkt_set_num_cells(SIXP_PKT_TYPE_REQUEST,
                                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE,
                                  1,
                                  sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_pkt_set_cell_list(SIXP_PKT_TYPE_REQUEST,
                                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE,
                                  (const uint8_t *)sample_cell_list1,
                                  sizeof(sample_cell_list1), 0,
                                  sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE,
                       TEST_SF_SFID, sixp_pkt_buf,
                       sizeof(sixp_pkt_cell_options_t) +
                       sizeof(sixp_pkt_num_cells_t) +
                       sizeof(sixp_pkt_metadata_t) +
                       sizeof(sample_cell_list1),
                       peer_addr,
                       NULL, NULL, 0) == 0);
  } else {
    sixp_trans_state_t state;
    state = sixp_trans_get_state(trans);
    if(state == SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      assert(
        sixp_pkt_set_cell_list(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               (const uint8_t *)sample_cell_list1,
                               sizeof(uint32_t), 0,
                               sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      sixp_output(SIXP_PKT_TYPE_RESPONSE,
                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                  TEST_SF_SFID, sixp_pkt_buf, sizeof(uint32_t),
                  peer_addr,
                  NULL, NULL, 0);
      test_index++;
    } else if(state == SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      process_post(&test_sf_process, test_sf_trans_done, NULL);
    }
  }
}

static void
test_delete_3_step(const linkaddr_t *peer_addr)
{
  sixp_trans_t *trans = sixp_trans_find(peer_addr);

  if(trans == NULL) {
    memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
    assert(sixp_pkt_set_metadata(SIXP_PKT_TYPE_REQUEST,
                                 (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE,
                                 sample_metadata,
                                 sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(
      sixp_pkt_set_cell_options(SIXP_PKT_TYPE_REQUEST,
                                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE,
                                SIXP_PKT_CELL_OPTION_TX |
                                SIXP_PKT_CELL_OPTION_SHARED,
                                sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_pkt_set_num_cells(SIXP_PKT_TYPE_REQUEST,
                                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE,
                                  1,
                                  sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE,
                       TEST_SF_SFID, sixp_pkt_buf,
                       sizeof(sixp_pkt_cell_options_t) +
                       sizeof(sixp_pkt_num_cells_t) +
                       sizeof(sixp_pkt_metadata_t),
                       peer_addr,
                       NULL, NULL, 0) == 0);
  } else {
    sixp_trans_state_t state;
    state = sixp_trans_get_state(trans);
    if(state == SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      assert(
        sixp_pkt_set_cell_list(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               (const uint8_t *)sample_cell_list1,
                               sizeof(sample_cell_list1), 0,
                               sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      sixp_output(SIXP_PKT_TYPE_RESPONSE,
                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                  TEST_SF_SFID, sixp_pkt_buf, sizeof(sample_cell_list1),
                  peer_addr,
                  NULL, NULL, 0);
    } else if (state == SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      assert(
        sixp_pkt_set_cell_list(SIXP_PKT_TYPE_CONFIRMATION,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               (const uint8_t *)sample_cell_list1,
                               sizeof(uint32_t), 0,
                               sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      sixp_output(SIXP_PKT_TYPE_CONFIRMATION,
                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                  TEST_SF_SFID, sixp_pkt_buf, sizeof(uint32_t),
                  peer_addr,
                  NULL, NULL, 0);
      process_post(&test_sf_process, test_sf_trans_done, NULL);
    } else if (state == SIXP_TRANS_STATE_CONFIRMATION_RECEIVED) {
      test_index++;
    }
  }
}

static void
test_relocate_2_step(const linkaddr_t *peer_addr)
{
  sixp_trans_t *trans = sixp_trans_find(peer_addr);

  if(trans == NULL) {
    memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
    assert(
      sixp_pkt_set_metadata(SIXP_PKT_TYPE_REQUEST,
                            (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_RELOCATE,
                            sample_metadata,
                            sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(
      sixp_pkt_set_cell_options(SIXP_PKT_TYPE_REQUEST,
                                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_RELOCATE,
                                SIXP_PKT_CELL_OPTION_TX |
                                SIXP_PKT_CELL_OPTION_SHARED,
                                sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(
      sixp_pkt_set_num_cells(SIXP_PKT_TYPE_REQUEST,
                             (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_RELOCATE,
                             3,
                             sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(
      sixp_pkt_set_rel_cell_list(SIXP_PKT_TYPE_REQUEST,
                                 (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_RELOCATE,
                                 (const uint8_t *)sample_cell_list1,
                                 sizeof(sample_cell_list1), 0,
                                 sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(
      sixp_pkt_set_cand_cell_list(SIXP_PKT_TYPE_REQUEST,
                                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_RELOCATE,
                                  (const uint8_t *)sample_cell_list2,
                                  sizeof(sample_cell_list2), 0,
                                  sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_RELOCATE,
                       TEST_SF_SFID, sixp_pkt_buf,
                       sizeof(sixp_pkt_metadata_t) +
                       sizeof(sixp_pkt_cell_options_t) +
                       sizeof(sixp_pkt_num_cells_t) +
                       sizeof(sample_cell_list1) +
                       sizeof(sample_cell_list2),
                       peer_addr,
                       NULL, NULL, 0) == 0);
  } else {
    sixp_trans_state_t state;
    state = sixp_trans_get_state(trans);
    if(state == SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      assert(
        sixp_pkt_set_cell_list(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               (const uint8_t *)sample_cell_list2,
                               sizeof(uint32_t), 0,
                               sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      sixp_output(SIXP_PKT_TYPE_RESPONSE,
                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                  TEST_SF_SFID, sixp_pkt_buf, sizeof(uint32_t),
                  peer_addr,
                  NULL, NULL, 0);
      test_index++;
    } else if(state == SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      process_post(&test_sf_process, test_sf_trans_done, NULL);
    }
  }
}

static void
test_count_2_step(const linkaddr_t *peer_addr)
{
  sixp_trans_t *trans = sixp_trans_find(peer_addr);

  if(trans == NULL) {
    memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
    assert(
      sixp_pkt_set_metadata(SIXP_PKT_TYPE_REQUEST,
                            (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_COUNT,
                            sample_metadata,
                            sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(
      sixp_pkt_set_cell_options(SIXP_PKT_TYPE_REQUEST,
                                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_COUNT,
                                SIXP_PKT_CELL_OPTION_TX |
                                SIXP_PKT_CELL_OPTION_RX |
                                SIXP_PKT_CELL_OPTION_SHARED,
                                sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

    assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_COUNT,
                       TEST_SF_SFID, sixp_pkt_buf,
                       sizeof(sixp_pkt_metadata_t) +
                       sizeof(sixp_pkt_cell_options_t),
                       peer_addr,
                       NULL, NULL, 0) == 0);
  } else {
    sixp_trans_state_t state;
    state = sixp_trans_get_state(trans);
    if(state == SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      assert(
        sixp_pkt_set_total_num_cells(SIXP_PKT_TYPE_RESPONSE,
                                     (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                                     0xf0,
                                     sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      sixp_output(SIXP_PKT_TYPE_RESPONSE,
                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                  TEST_SF_SFID, sixp_pkt_buf,
                  sizeof(sixp_pkt_total_num_cells_t),
                  peer_addr,
                  NULL, NULL, 0);
      test_index++;
    } else if(state == SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      process_post(&test_sf_process, test_sf_trans_done, NULL);
    }
  }
}

static void
test_list_2_step(const linkaddr_t *peer_addr)
{
  sixp_trans_t *trans = sixp_trans_find(peer_addr);

  if(trans == NULL) {
    memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
    assert(
      sixp_pkt_set_metadata(SIXP_PKT_TYPE_REQUEST,
                            (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                            sample_metadata,
                            sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(
      sixp_pkt_set_cell_options(SIXP_PKT_TYPE_REQUEST,
                                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                                SIXP_PKT_CELL_OPTION_TX |
                                SIXP_PKT_CELL_OPTION_RX |
                                SIXP_PKT_CELL_OPTION_SHARED,
                                sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

    assert(
      sixp_pkt_set_offset(SIXP_PKT_TYPE_REQUEST,
                          (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                          0x0f,
                          sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(
      sixp_pkt_set_max_num_cells(SIXP_PKT_TYPE_REQUEST,
                                 (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                                 0xf0,
                                 sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

    assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                       TEST_SF_SFID, sixp_pkt_buf,
                       sizeof(sixp_pkt_metadata_t) +
                       sizeof(sixp_pkt_cell_options_t) +
                       sizeof(sixp_pkt_reserved_t) +
                       sizeof(sixp_pkt_offset_t) +
                       sizeof(sixp_pkt_max_num_cells_t),
                       peer_addr,
                       NULL, NULL, 0) == 0);
  } else {
    sixp_trans_state_t state;
    state = sixp_trans_get_state(trans);
    if(state == SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      assert(
        sixp_pkt_set_cell_list(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               (const uint8_t *)sample_cell_list1,
                               sizeof(sample_cell_list1), 0,
                               sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      assert(
        sixp_pkt_set_cell_list(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                               (const uint8_t *)sample_cell_list2,
                               sizeof(sample_cell_list2),
                               sizeof(sample_cell_list1),
                               sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      sixp_output(SIXP_PKT_TYPE_RESPONSE,
                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                  TEST_SF_SFID, sixp_pkt_buf,
                  sizeof(sample_cell_list1) + sizeof(sample_cell_list2),
                  peer_addr,
                  NULL, NULL, 0);
      test_index++;
    } else if(state == SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      process_post(&test_sf_process, test_sf_trans_done, NULL);
    }
  }
}

static void
test_list_2_step_eol(const linkaddr_t *peer_addr)
{
  sixp_trans_t *trans = sixp_trans_find(peer_addr);

  if(trans == NULL) {
    memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
    assert(
      sixp_pkt_set_metadata(SIXP_PKT_TYPE_REQUEST,
                            (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                            sample_metadata,
                            sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(
      sixp_pkt_set_cell_options(SIXP_PKT_TYPE_REQUEST,
                                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                                SIXP_PKT_CELL_OPTION_TX |
                                SIXP_PKT_CELL_OPTION_RX |
                                SIXP_PKT_CELL_OPTION_SHARED,
                                sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

    assert(
      sixp_pkt_set_offset(SIXP_PKT_TYPE_REQUEST,
                          (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                          0x0f,
                          sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
    assert(
      sixp_pkt_set_max_num_cells(SIXP_PKT_TYPE_REQUEST,
                                 (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                                 0xf0,
                                 sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

    assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                       TEST_SF_SFID, sixp_pkt_buf,
                       sizeof(sixp_pkt_metadata_t) +
                       sizeof(sixp_pkt_cell_options_t) +
                       sizeof(sixp_pkt_reserved_t) +
                       sizeof(sixp_pkt_offset_t) +
                       sizeof(sixp_pkt_max_num_cells_t),
                       peer_addr,
                       NULL, NULL, 0) == 0);
  } else {
    sixp_trans_state_t state;
    state = sixp_trans_get_state(trans);
    if(state == SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      assert(
        sixp_pkt_set_cell_list(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_EOL,
                               (const uint8_t *)sample_cell_list1,
                               sizeof(sample_cell_list1), 0,
                               sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      assert(
        sixp_pkt_set_cell_list(SIXP_PKT_TYPE_RESPONSE,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_EOL,
                               (const uint8_t *)sample_cell_list2,
                               sizeof(sample_cell_list2),
                               sizeof(sample_cell_list1),
                               sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
      sixp_output(SIXP_PKT_TYPE_RESPONSE,
                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_EOL,
                  TEST_SF_SFID, sixp_pkt_buf,
                  sizeof(sample_cell_list1) + sizeof(sample_cell_list2),
                  peer_addr,
                  NULL, NULL, 0);
      test_index++;
    } else if(state == SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      process_post(&test_sf_process, test_sf_trans_done, NULL);
    }
  }
}

static void
test_signal_2_step(const linkaddr_t *peer_addr)
{
  sixp_trans_t *trans = sixp_trans_find(peer_addr);
  uint8_t payload[10];
  size_t payload_len;

  if(trans == NULL) {
    memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
    assert(
      sixp_pkt_set_metadata(SIXP_PKT_TYPE_REQUEST,
                            (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                            sample_metadata,
                            sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

    payload[0] =  0xbe;
    payload[1] =  0xef;
    payload[2] =  0xca;
    payload[3] =  0xfe;
    payload_len = 4;

    assert(
      sixp_pkt_set_payload(SIXP_PKT_TYPE_REQUEST,
                           (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_SIGNAL,
                           (const uint8_t *)payload, payload_len,
                           sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

    assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_SIGNAL,
                       TEST_SF_SFID, sixp_pkt_buf,
                       sizeof(sixp_pkt_metadata_t) + payload_len,
                       peer_addr,
                       NULL, NULL, 0) == 0);
  } else {
    sixp_trans_state_t state;
    printf("hoge\n");
    state = sixp_trans_get_state(trans);
    if(state == SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      payload[0] =  0x01;
      payload[1] =  0x02;
      payload[2] =  0x03;
      payload[3] =  0x04;
      payload[4] =  0x05;
      payload_len = 5;

      assert(
        sixp_pkt_set_payload(SIXP_PKT_TYPE_RESPONSE,
                             (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                             (const uint8_t *)payload, payload_len,
                             sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

      sixp_output(SIXP_PKT_TYPE_RESPONSE,
                  (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                  TEST_SF_SFID, sixp_pkt_buf,
                  payload_len,
                  peer_addr,
                  NULL, NULL, 0);
      test_index++;
    } else if(state == SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      process_post(&test_sf_process, test_sf_trans_done, NULL);
    }
  }
}

static void
test_clear_2_step(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc)
{
  sixp_trans_t *trans = sixp_trans_find(peer_addr);

  if(trans == NULL) {
    memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
    assert(
      sixp_pkt_set_metadata(SIXP_PKT_TYPE_REQUEST,
                            (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_CLEAR,
                            sample_metadata,
                            sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

    assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_CLEAR,
                       TEST_SF_SFID, sixp_pkt_buf,
                       sizeof(sixp_pkt_metadata_t),
                       peer_addr,
                       NULL, NULL, 0) == 0);
  } else {
    sixp_trans_state_t state;
    state = sixp_trans_get_state(trans);
    if(state == SIXP_TRANS_STATE_REQUEST_RECEIVED) {
      sixp_output(SIXP_PKT_TYPE_RESPONSE,
                  (sixp_pkt_code_t)rc,
                  TEST_SF_SFID, NULL, 0, peer_addr,
                  NULL, NULL, 0);
      test_index++;
    } else if(state == SIXP_TRANS_STATE_RESPONSE_RECEIVED) {
      process_post(&test_sf_process, test_sf_trans_done, NULL);
    }
  }
}

static void
test_clear_2_step_success(const linkaddr_t *peer_addr)
{
  test_clear_2_step(peer_addr, SIXP_PKT_RC_SUCCESS);
}

static void
test_clear_2_step_err(const linkaddr_t *peer_addr)
{
  test_clear_2_step(peer_addr, SIXP_PKT_RC_ERR);
}

static void
test_clear_2_step_eol(const linkaddr_t *peer_addr)
{
  test_clear_2_step(peer_addr, SIXP_PKT_RC_EOL);
}

static void
test_clear_2_step_reset(const linkaddr_t *peer_addr)
{
  test_clear_2_step(peer_addr, SIXP_PKT_RC_RESET);
}

static void
test_clear_2_step_version(const linkaddr_t *peer_addr)
{
  test_clear_2_step(peer_addr, SIXP_PKT_RC_ERR_VERSION);
}

static void
test_clear_2_step_sfid(const linkaddr_t *peer_addr)
{
  test_clear_2_step(peer_addr, SIXP_PKT_RC_ERR_SFID);
}

static void
test_clear_2_step_seqnum(const linkaddr_t *peer_addr)
{
  test_clear_2_step(peer_addr, SIXP_PKT_RC_ERR_SEQNUM);
}

static void
test_clear_2_step_busy(const linkaddr_t *peer_addr)
{
  test_clear_2_step(peer_addr, SIXP_PKT_RC_ERR_BUSY);
}

static void
test_clear_2_step_locked(const linkaddr_t *peer_addr)
{
  test_clear_2_step(peer_addr, SIXP_PKT_RC_ERR_LOCKED);
}

static void
test_clear_2_step_celllist(const linkaddr_t *peer_addr)
{
  test_clear_2_step(peer_addr, SIXP_PKT_RC_ERR_CELLLIST);
}

PROCESS_THREAD(test_sf_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();
  etimer_set(&et, CLOCK_SECOND);

  while(1) {
    if(test_index == (sizeof(test_case) / sizeof(test_func))) {
      break;
    } else {
      test_case[test_index]((const linkaddr_t *)&peer_addr);
    }
    PROCESS_WAIT_EVENT_UNTIL(ev == test_sf_trans_done);
    PROCESS_YIELD_UNTIL(etimer_expired(&et));
    etimer_reset(&et);
    test_index++;
  }
  PRINTF("done\n");

  PROCESS_END();
}

const sixtop_sf_t test_sf = {
  TEST_SF_SFID,
  TEST_SF_TIMEOUT,
  init,
  input,
  timeout
};
