/*
 * Copyright (c) 2017, Toshiba Corporation
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
 *
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
#include <shell.h>
#include <shell-commands.h>
#include <net/linkaddr.h>
#include <net/mac/tsch/tsch-schedule.h>
#include <net/mac/tsch/sixtop/sixtop.h>
#include <net/mac/tsch/sixtop/sixp.h>
#include <net/mac/tsch/sixtop/sixp-nbr.h>
#include <net/mac/tsch/sixtop/sixp-trans.h>
#include <net/mac/tsch/sixtop/sixp-pkt.h>

#include <sys/log.h>
#define LOG_MODULE "6top"
#define LOG_LEVEL LOG_LEVEL_6TOP

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <lib/assert.h>

#define SIXP_PKT_BUFLEN     128
static uint8_t sixp_pkt_buf[SIXP_PKT_BUFLEN];

static shell_output_func *shell_output = NULL;
/* this variable is used for LIST Request */
static uint16_t cell_list_offset = 0; /* XXX: should be had in sixp_nbr */

typedef struct {
  uint8_t slot_offset[2];
  uint8_t channel_offset[2];
} sf_plugtest_cell_t;

typedef struct {
  sixp_pkt_cmd_t cmd;
  linkaddr_t peer_addr;
  uint16_t slot_offset;
  uint16_t channel_offset;
} subcmd_args_t;

typedef void subcmd(shell_output_func output, subcmd_args_t *args);

typedef void (req_handler_t)(const linkaddr_t *peer_addr,
                             const uint8_t *body, size_t body_len);
typedef void (res_handler_t)(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc,
                             const uint8_t *body, size_t body_len);

static void add_res_sent_callback(void *arg, uint16_t arg_len,
                                  const linkaddr_t *dest_addr,
                                  sixp_output_status_t status);
static void delete_res_sent_callback(void *arg, uint16_t arg_len,
                                     const linkaddr_t *dest_addr,
                                     sixp_output_status_t status);

static void send_list_req(const linkaddr_t *peer_addr);

static int add_cell(const linkaddr_t *peer_addr, const sf_plugtest_cell_t *cell,
                    uint8_t link_options);
static int delete_cell(const linkaddr_t *peer_addr,
                       const sf_plugtest_cell_t *cell);
static int reserve_cell(const linkaddr_t *peer_addr,
                        const sf_plugtest_cell_t *cell);
static void clear_cells(const linkaddr_t *peer_addr,
                        struct tsch_slotframe *slotframe);

static void help(shell_output_func output, subcmd_args_t *args);
static void add_delete(shell_output_func output, subcmd_args_t *args);
static void count(shell_output_func output, subcmd_args_t *args);
static void list(shell_output_func output, subcmd_args_t *args);
static void clear(shell_output_func output, subcmd_args_t *args);

static int parse_args(shell_output_func output,
                      char *args, const char **subcmd,
                      subcmd_args_t *subcmd_args);

static const char subcmd_help[] = "help";
static void shell_subcmd(shell_output_func output, char *args);

static req_handler_t add_req_handler;
static res_handler_t add_res_handler;
static req_handler_t delete_req_handler;
static res_handler_t delete_res_handler;
static req_handler_t count_req_handler;
static res_handler_t count_res_handler;
static req_handler_t list_req_handler;
static res_handler_t list_res_handler;
static req_handler_t clear_req_handler;
static res_handler_t clear_res_handler;

static const struct {
  char *name;
  subcmd *func;
  sixp_pkt_cmd_t cmd;
} subcmds[] = {
  { "help", help, SIXP_PKT_CMD_UNAVAILABLE },
  { "add", add_delete, SIXP_PKT_CMD_ADD },
  { "delete", add_delete, SIXP_PKT_CMD_DELETE },
  { "count", count, SIXP_PKT_CMD_COUNT },
  { "list", list, SIXP_PKT_CMD_LIST },
  { "clear", clear, SIXP_PKT_CMD_CLEAR },
  { NULL, NULL, SIXP_PKT_CMD_UNAVAILABLE }
};

static const struct {
  sixp_pkt_cmd_t cmd;
  req_handler_t *req;
  res_handler_t *res;
} handlers[] = {
  { SIXP_PKT_CMD_ADD, add_req_handler, add_res_handler },
  { SIXP_PKT_CMD_DELETE, delete_req_handler, delete_res_handler },
  { SIXP_PKT_CMD_COUNT, count_req_handler, count_res_handler },
  { SIXP_PKT_CMD_LIST, list_req_handler, list_res_handler },
  { SIXP_PKT_CMD_CLEAR, clear_req_handler, clear_res_handler },
  { SIXP_PKT_CMD_UNAVAILABLE, NULL, NULL }
};

static void
add_res_sent_callback(void *arg, uint16_t arg_len,
                      const linkaddr_t *dest_addr,
                      sixp_output_status_t status)
{
  if(arg_len != sizeof(sf_plugtest_cell_t) ||
     status == SIXP_OUTPUT_STATUS_FAILURE ||
     dest_addr == NULL) {
    LOG_ERR("error in sending a response\n");
  } else {
    add_cell(dest_addr, (sf_plugtest_cell_t *)arg, LINK_OPTION_RX);
  }
}

static void
delete_res_sent_callback(void *arg, uint16_t arg_len,
                         const linkaddr_t *dest_addr,
                         sixp_output_status_t status)
{
  if(arg_len != sizeof(sf_plugtest_cell_t) ||
     status == SIXP_OUTPUT_STATUS_FAILURE ||
     dest_addr == NULL) {
    LOG_ERR("error in sending a response\n");
  } else {
    delete_cell(dest_addr, (sf_plugtest_cell_t *)arg);
  }
}

static void
send_list_req(const linkaddr_t *peer_addr)
{
  const sixp_pkt_max_num_cells_t SF_PLUGTEST_MAC_NUM_CELLS = 1;
  memset(&sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
  assert(sixp_pkt_set_cell_options(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                                   SIXP_PKT_CELL_OPTION_TX,
                                   sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

  assert(sixp_pkt_set_offset(SIXP_PKT_TYPE_REQUEST,
                             (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                             (sixp_pkt_offset_t)cell_list_offset,
                             sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

  assert(sixp_pkt_set_max_num_cells(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                                    SF_PLUGTEST_MAC_NUM_CELLS,
                                    sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

  assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                     (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                     SF_PLUGTEST_SFID, sixp_pkt_buf,
                     sizeof(sixp_pkt_metadata_t) +
                     sizeof(sixp_pkt_cell_options_t) +
                     sizeof(sixp_pkt_reserved_t) +
                     sizeof(sixp_pkt_offset_t) +
                     sizeof(sixp_pkt_max_num_cells_t),
                     peer_addr,
                     NULL, NULL, 0) == 0);
}

static int
add_cell(const linkaddr_t *peer_addr, const sf_plugtest_cell_t *cell,
         uint8_t link_options)
{
  struct tsch_slotframe *slotframe;
  uint16_t timeslot;
  uint16_t channel_offset;

  assert(peer_addr != NULL && cell != NULL);
  if(peer_addr == NULL || cell == NULL) {
    return -1;
  }

  timeslot = cell->slot_offset[0] + (cell->slot_offset[1] << 8);
  channel_offset = cell->channel_offset[0] + (cell->channel_offset[1] << 8);

  if((slotframe = tsch_schedule_get_slotframe_by_handle(0)) == NULL ||
     tsch_schedule_add_link(slotframe, link_options, LINK_TYPE_NORMAL,
                            peer_addr, timeslot, channel_offset, 1) == NULL) {
    LOG_ERR("cannot add a cell\n");
    return -1;
  }
  LOG_INFO("Succeeded to add a cell [slot:%u]\n", timeslot);

  return 0;
}

static int
delete_cell(const linkaddr_t *peer_addr, const sf_plugtest_cell_t *cell)
{
  struct tsch_slotframe *slotframe;
  uint16_t timeslot;
  uint16_t channel_offset;

  assert(peer_addr != NULL && cell != NULL);
  if(peer_addr == NULL || cell == NULL) {
    return -1;
  }

  timeslot = cell->slot_offset[0] + (cell->slot_offset[1] << 8);
  channel_offset = cell->channel_offset[0] + (cell->channel_offset[1] << 8);

  if((slotframe = tsch_schedule_get_slotframe_by_handle(0)) == NULL ||
     tsch_schedule_remove_link_by_timeslot(slotframe, timeslot, channel_offset) == 0) {
    LOG_ERR("cannot delete a cell\n");
    return -1;
  }
  LOG_INFO("Succeeded to delete a cell [slot:%u]\n", timeslot);

  return 0;
}

static int
reserve_cell(const linkaddr_t *peer_addr, const sf_plugtest_cell_t *cell)
{
  if(add_cell(peer_addr, cell, 0) < 0 ||
     delete_cell(peer_addr, cell) < 0) {
    /* fail to reserve the cell */
    return -1;
  }
  return 0;
}

static void
clear_cells(const linkaddr_t *peer_addr, struct tsch_slotframe *slotframe)
{
  struct tsch_link *cell, *next_cell;

  assert(peer_addr != NULL);
  if(peer_addr == NULL) {
    return;
  }
  for(cell = (struct tsch_link *)list_head(slotframe->links_list);
      cell != NULL; cell = next_cell) {
    next_cell = (struct tsch_link *)list_item_next(cell);
    if(memcmp(&cell->addr, peer_addr, sizeof(linkaddr_t)) == 0) {
      assert(tsch_schedule_remove_link(slotframe, cell) == 1);
    }
  }
}

static void
add_req_handler(const linkaddr_t *peer_addr,
                const uint8_t *body, size_t body_len)
{
  sixp_pkt_cell_options_t cell_options;
  const uint8_t *cell;
  sixp_pkt_offset_t cell_list_len;
  static sf_plugtest_cell_t pending_cell;
  uint16_t timeslot;
  uint16_t channel_offset;
  struct tsch_slotframe *slotframe;


  assert(peer_addr != NULL && body != NULL);
  if(body_len != (sizeof(sixp_pkt_metadata_t) +
                  sizeof(sixp_pkt_cell_options_t) +
                  sizeof(sixp_pkt_num_cells_t) +
                  sizeof(sf_plugtest_cell_t))) {
    LOG_ERR("invalid Add Request length: %lu\n", (unsigned long)body_len);
  }
  assert(
    sixp_pkt_get_cell_options(SIXP_PKT_TYPE_REQUEST,
                              (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                              &cell_options,
                              body, body_len) == 0);
  if(cell_options != SIXP_PKT_CELL_OPTION_TX &&
     cell_options != (SIXP_PKT_CELL_OPTION_TX | SIXP_PKT_CELL_OPTION_SHARED)) {
    LOG_ERR("invalid Cell Options: %u\n", cell_options);
  }
  assert(
    sixp_pkt_get_cell_list(SIXP_PKT_TYPE_REQUEST,
                           (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                           &cell, &cell_list_len, body, body_len) == 0);
  assert(cell_list_len == sizeof(sf_plugtest_cell_t));
  memcpy(&pending_cell, cell, sizeof(pending_cell));
  timeslot = pending_cell.slot_offset[0] + (pending_cell.slot_offset[1] << 8);
  channel_offset = pending_cell.channel_offset[0] + (pending_cell.channel_offset[1] << 8);

  if((slotframe = tsch_schedule_get_slotframe_by_handle(0)) == NULL ||
     tsch_schedule_get_link_by_timeslot(slotframe, timeslot, channel_offset) != NULL ||
     reserve_cell(peer_addr, &pending_cell) < 0) {
    LOG_ERR("Failed to add a cell [slot:%u]\n", timeslot);
    sixp_output(SIXP_PKT_TYPE_RESPONSE,
                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_ERR_BUSY,
                SF_PLUGTEST_SFID, NULL, 0, peer_addr,
                NULL, NULL, 0);
  } else {
    sixp_output(SIXP_PKT_TYPE_RESPONSE,
                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                SF_PLUGTEST_SFID, cell, cell_list_len, peer_addr,
                add_res_sent_callback, &pending_cell, sizeof(pending_cell));
  }
}

static void
add_res_handler(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc,
                const uint8_t *body, size_t body_len)
{
  const uint8_t *cell;
  sixp_pkt_offset_t cell_list_len;
  uint16_t timeslot;
  uint16_t channel_offset;
  struct tsch_slotframe *slotframe;

  if(body_len != 4) {
    LOG_ERR("invalid Add Response length: %lu\n", (unsigned long)body_len);
    return;
  }

  assert(sixp_pkt_get_cell_list(SIXP_PKT_TYPE_RESPONSE,
                                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                                &cell, &cell_list_len, body, body_len) == 0);
  timeslot = cell[0] + (cell[1] << 8);
  channel_offset = cell[2] + (cell[3] << 8);

  if(rc != SIXP_PKT_RC_SUCCESS) {
    LOG_ERR("received return code of %u\n", rc);
    return;
  }

  if((slotframe = tsch_schedule_get_slotframe_by_handle(0)) == NULL ||
     tsch_schedule_get_link_by_timeslot(slotframe, timeslot, channel_offset) != NULL ||
     add_cell(peer_addr, (sf_plugtest_cell_t *)cell, LINK_OPTION_TX) < 0) {
    LOG_ERR("Failed to add a cell [slot:%u]\n", timeslot);
  }
}

static void
delete_req_handler(const linkaddr_t *peer_addr,
                const uint8_t *body, size_t body_len)
{
  sixp_pkt_cell_options_t cell_options;
  const uint8_t *cell;
  sixp_pkt_offset_t cell_list_len;
  static sf_plugtest_cell_t pending_cell;
  uint16_t timeslot;
  uint16_t channel_offset;
  struct tsch_slotframe *slotframe;
  struct tsch_link *link;


  assert(peer_addr != NULL && body != NULL);
  if(body_len != (sizeof(sixp_pkt_metadata_t) +
                  sizeof(sixp_pkt_cell_options_t) +
                  sizeof(sixp_pkt_num_cells_t) +
                  sizeof(sf_plugtest_cell_t))) {
    LOG_ERR("invalid Delete Request length: %lu\n", (unsigned long)body_len);
  }
  assert(
    sixp_pkt_get_cell_options(SIXP_PKT_TYPE_REQUEST,
                              (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE,
                              &cell_options,
                              body, body_len) == 0);
  if(cell_options != SIXP_PKT_CELL_OPTION_TX &&
     cell_options != (SIXP_PKT_CELL_OPTION_TX | SIXP_PKT_CELL_OPTION_SHARED)) {
    LOG_ERR("invalid Cell Options: %u\n", cell_options);
  }
  assert(
    sixp_pkt_get_cell_list(SIXP_PKT_TYPE_REQUEST,
                           (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE,
                           &cell, &cell_list_len, body, body_len) == 0);
  assert(cell_list_len == sizeof(sf_plugtest_cell_t));
  memcpy(&pending_cell, cell, sizeof(pending_cell));
  timeslot = pending_cell.slot_offset[0] + (pending_cell.slot_offset[1] << 8);
  channel_offset = pending_cell.channel_offset[0] + (pending_cell.channel_offset[1] << 8);

  if((slotframe = tsch_schedule_get_slotframe_by_handle(0)) == NULL ||
     (link = tsch_schedule_get_link_by_timeslot(slotframe, timeslot, channel_offset)) == NULL ||
     memcmp(peer_addr, &link->addr, sizeof(linkaddr_t)) != 0) {
    LOG_ERR("Failed to delete a cell [slot:%u]\n", timeslot);
    sixp_output(SIXP_PKT_TYPE_RESPONSE,
                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_ERR_BUSY,
                SF_PLUGTEST_SFID, NULL, 0, peer_addr,
                NULL, NULL, 0);
  } else {
    sixp_output(SIXP_PKT_TYPE_RESPONSE,
                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                SF_PLUGTEST_SFID, cell, cell_list_len, peer_addr,
                delete_res_sent_callback, &pending_cell, sizeof(pending_cell));
  }
}

static void
delete_res_handler(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc,
                const uint8_t *body, size_t body_len)
{
  struct tsch_slotframe *slotframe;
  struct tsch_link *link;
  const sf_plugtest_cell_t *cell;
  sixp_pkt_offset_t cell_list_len;
  sixp_nbr_t *nbr;
  uint16_t timeslot;
  uint16_t channel_offset;

  if(body_len != 4) {
    LOG_ERR("invalid Delete Response length: %lu\n", (unsigned long)body_len);
    return;
  }

  assert(
    sixp_pkt_get_cell_list(SIXP_PKT_TYPE_RESPONSE,
                           (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                           (const uint8_t **)&cell, &cell_list_len,
                           body, body_len) == 0);
  timeslot = cell->slot_offset[0] + (cell->slot_offset[1] << 8);
  channel_offset = cell->channel_offset[0] + (cell->channel_offset[1] << 8);

  if((nbr = sixp_nbr_find(peer_addr)) == NULL) {
    LOG_ERR("unexpected error; cannot find nbr\n");
    return;
  }

  if(rc != SIXP_PKT_RC_SUCCESS) {
    LOG_ERR("received return code of %u\n", rc);
    return;
  }

  if((slotframe = tsch_schedule_get_slotframe_by_handle(0)) == NULL ||
     (link = tsch_schedule_get_link_by_timeslot(slotframe, timeslot, channel_offset)) == NULL ||
     memcmp(peer_addr, &link->addr, sizeof(linkaddr_t)) != 0 ||
     delete_cell(peer_addr, cell) < 0) {
    LOG_ERR("Failed to delete a cell [slot:%u]\n", timeslot);
  }
}

static void
count_req_handler(const linkaddr_t *peer_addr,
                  const uint8_t *body, size_t body_len)
{
  sixp_pkt_cell_options_t cell_options;
  struct tsch_slotframe *slotframe;
  struct tsch_link *cell;
  sixp_pkt_total_num_cells_t total_num_cells = 0;
  uint8_t buf[2];

  assert(peer_addr != NULL && body != NULL);
  if(body_len != (sizeof(sixp_pkt_metadata_t) +
                  sizeof(sixp_pkt_cell_options_t))) {
    LOG_ERR("invalid Count Request length: %lu\n", (unsigned long)body_len);
  }
  assert(
    sixp_pkt_get_cell_options(SIXP_PKT_TYPE_REQUEST,
                              (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_COUNT,
                              &cell_options,
                              body, body_len) == 0);
  if(cell_options != SIXP_PKT_CELL_OPTION_TX &&
     cell_options != (SIXP_PKT_CELL_OPTION_TX | SIXP_PKT_CELL_OPTION_SHARED)) {
    LOG_ERR("invalid Cell Options: %u\n", cell_options);
  }

  if((slotframe = tsch_schedule_get_slotframe_by_handle(0)) == NULL) {
    sixp_output(SIXP_PKT_TYPE_RESPONSE,
                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_ERR,
                SF_PLUGTEST_SFID, NULL, 0, peer_addr,
                NULL, NULL, 0);
    return;
  }

  for(cell = (struct tsch_link *)list_head(slotframe->links_list);
      cell != NULL; cell = (struct tsch_link *)list_item_next(cell)) {
    if(memcmp(&cell->addr, peer_addr, sizeof(linkaddr_t)) == 0 &&
       cell->link_options == LINK_OPTION_RX) {
      total_num_cells++;
    }
  }

  /* make sure total_num_cells are set in little-endian */
  buf[0] = (total_num_cells & 0xff);
  buf[1] = (total_num_cells >> 8);

  sixp_output(SIXP_PKT_TYPE_RESPONSE,
              (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS, SF_PLUGTEST_SFID,
              (const uint8_t *)buf, sizeof(buf),
              peer_addr, NULL, NULL, 0);
}

static void
count_res_handler(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc,
                const uint8_t *body, size_t body_len)
{
  sixp_pkt_total_num_cells_t total_num_cells;

  if(body_len != 2) {
    LOG_ERR("invalid Count Response length: %lu\n", (unsigned long)body_len);
    return;
  }

  if(rc != SIXP_PKT_RC_SUCCESS) {
    LOG_ERR("received return code of %u\n", rc);
    return;
  }

  assert(
    sixp_pkt_get_total_num_cells(SIXP_PKT_TYPE_RESPONSE,
                                 (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                                 &total_num_cells, body, body_len) == 0);

  LOG_INFO("Succeeded to get COUNT: %u\n", total_num_cells);
}

static void
list_req_handler(const linkaddr_t *peer_addr,
                  const uint8_t *body, size_t body_len)
{
  sixp_pkt_cell_options_t cell_options;
  sixp_pkt_offset_t cell_list_offset;
  sixp_pkt_max_num_cells_t max_num_cells;

  struct tsch_slotframe *slotframe;
  struct tsch_link *link;
  sixp_pkt_offset_t cell_nums;
  sf_plugtest_cell_t cell;

  assert(peer_addr != NULL && body != NULL);
  if(body_len != (sizeof(sixp_pkt_metadata_t) +
                  sizeof(sixp_pkt_cell_options_t) +
                  sizeof(sixp_pkt_reserved_t) +
                  sizeof(sixp_pkt_offset_t) +
                  sizeof(sixp_pkt_max_num_cells_t))) {
    LOG_ERR("invalid List Request length: %lu\n", (unsigned long)body_len);
  }

  assert(
    sixp_pkt_get_cell_options(SIXP_PKT_TYPE_REQUEST,
                              (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                              &cell_options,
                              body, body_len) == 0);
  assert(
    sixp_pkt_get_offset(SIXP_PKT_TYPE_REQUEST,
                        (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                        &cell_list_offset,
                        body, body_len) == 0);
  assert(
    sixp_pkt_get_max_num_cells(SIXP_PKT_TYPE_REQUEST,
                               (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_LIST,
                               &max_num_cells,
                               body, body_len) == 0);

  if(cell_options != SIXP_PKT_CELL_OPTION_TX &&
     cell_options != (SIXP_PKT_CELL_OPTION_TX | SIXP_PKT_CELL_OPTION_SHARED)) {
    LOG_ERR("invalid Cell Options: %u\n", cell_options);
    return;
  }

  if(max_num_cells < 1) {
    LOG_ERR("invalid MaxNumCells: %u\n", max_num_cells);
    return;
  }

  if((slotframe = tsch_schedule_get_slotframe_by_handle(0)) == NULL) {
    sixp_output(SIXP_PKT_TYPE_RESPONSE,
                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_ERR,
                SF_PLUGTEST_SFID, NULL, 0, peer_addr,
                NULL, NULL, 0);
    return;
  }

  cell_nums = 0;
  for(link = (struct tsch_link *)list_head(slotframe->links_list);
      link != NULL; link = (struct tsch_link *)list_item_next(link)) {
    if(memcmp(&link->addr, peer_addr, sizeof(linkaddr_t)) == 0 &&
       link->link_options == LINK_OPTION_RX) {
      if(cell_list_offset == cell_nums) {
        cell.slot_offset[0] = link->timeslot & 0xff;
        cell.slot_offset[1] = link->timeslot >> 8;
        cell.channel_offset[0] = link->channel_offset & 0xff;
        cell.channel_offset[1] = link->channel_offset >> 8;
      }
      cell_nums++;
    }
  }

  if(cell_nums == 0) {
    assert(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_EOL,
                       SF_PLUGTEST_SFID, NULL, 0,
                       peer_addr, NULL, NULL, 0) == 0);
  } else {
    assert(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                       (cell_list_offset + 1) == cell_nums ?
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_EOL :
                       (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                       SF_PLUGTEST_SFID,
                       (const uint8_t *)&cell, sizeof(cell),
                       peer_addr, NULL, NULL, 0) == 0);
  }
}

static void
list_res_handler(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc,
                const uint8_t *body, size_t body_len)
{
  sf_plugtest_cell_t *cell;
  uint16_t slot_offset, channel_offset;
  sixp_pkt_offset_t cell_list_len;
  sixp_pkt_offset_t i;

  assert(sixp_pkt_get_cell_list(SIXP_PKT_TYPE_RESPONSE,
                                (sixp_pkt_code_t)(uint8_t)rc,
                                (const uint8_t **)&cell, &cell_list_len,
                                body, body_len) == 0);

  LOG_INFO("Succeeded to get LIST: ");
  for(i = 0; i < cell_list_len; i += sizeof(sf_plugtest_cell_t)) {
    slot_offset = cell->slot_offset[0] + (cell->slot_offset[1] << 8);
    channel_offset = cell->channel_offset[0] + (cell->channel_offset[1] << 8);
    LOG_INFO_("[slot:%u, channel:%u]", slot_offset, channel_offset);
  }

  if(rc == SIXP_PKT_RC_SUCCESS) {
    LOG_INFO_(" continued\n");
    cell_list_offset += (i / sizeof(sf_plugtest_cell_t));
  } else {
    /* EOL */
    LOG_INFO_(" EOL\n");
    cell_list_offset = 0;
  }
}

static void
clear_req_handler(const linkaddr_t *peer_addr,
                  const uint8_t *body, size_t body_len)
{
  struct tsch_slotframe *slotframe;
  sixp_nbr_t *nbr;

  assert(peer_addr != NULL && body != NULL);
  if(body_len != sizeof(sixp_pkt_metadata_t)) {
    LOG_ERR("invalid Clear Request length: %lu\n", (unsigned long)body_len);
  }

  if((slotframe = tsch_schedule_get_slotframe_by_handle(0)) == NULL) {
    sixp_output(SIXP_PKT_TYPE_RESPONSE,
                (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_ERR,
                SF_PLUGTEST_SFID, NULL, 0, peer_addr,
                NULL, NULL, 0);
    return;
  }

  clear_cells(peer_addr, slotframe);

  sixp_output(SIXP_PKT_TYPE_RESPONSE,
              (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
              SF_PLUGTEST_SFID, NULL, 0, peer_addr,
              NULL, NULL, 0);

  if((nbr = sixp_nbr_find(peer_addr)) != NULL) {
    /* clear GEN */
    sixp_nbr_free(nbr);
  }
}

static void
clear_res_handler(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc,
                const uint8_t *body, size_t body_len)
{
  struct tsch_slotframe *slotframe;
  sixp_nbr_t *nbr;

  assert(peer_addr != NULL && body != NULL);

  if(peer_addr == NULL) {
    return;
  }

  if(body_len != 0) {
    LOG_ERR("invalid Clear Response length: %lu\n", (unsigned long)body_len);
    return;
  }

  if((slotframe = tsch_schedule_get_slotframe_by_handle(0)) != NULL) {
    clear_cells(peer_addr, slotframe);
  }
  if((nbr = sixp_nbr_find(peer_addr)) != NULL) {
    /* clear GEN */
    sixp_nbr_free(nbr);
  }

  LOG_INFO("Succeeded to CLEAR\n");
}

static void
help(shell_output_func output, subcmd_args_t *args)
{
  /* help doesn't use peer_addr */
  SHELL_OUTPUT(output, "cmd [slot_offset] [channel_offset] [peer_addr]\n");
  SHELL_OUTPUT(output, "available commands\n");
  SHELL_OUTPUT(output, "add    - add a TX cell\n");
  SHELL_OUTPUT(output, "delete - delete a TX cell\n");
  SHELL_OUTPUT(output, "count  - get cell count\n");
  SHELL_OUTPUT(output, "list   - get a cell list\n");
  SHELL_OUTPUT(output, "clear  - clear TX cells\n");
  SHELL_OUTPUT(output, "help   - show this usage\n");
  SHELL_OUTPUT(output, "example> 6p add 4 5 01:01:01:01:01:01:01:01\n");
}

static void
add_delete(shell_output_func output, subcmd_args_t *args)
{
  assert(args != NULL);
  sf_plugtest_cell_t cell;

  /* set cell attributes in little-endian */
  cell.slot_offset[0] = args->slot_offset & 0xff;
  cell.slot_offset[1] = args->slot_offset >> 8;
  cell.channel_offset[0] = args->channel_offset & 0xff;
  cell.channel_offset[1] = args->channel_offset >> 8;

  if(args->cmd == SIXP_PKT_CMD_ADD &&
     reserve_cell(&(args->peer_addr), &cell) < 0) {
    SHELL_OUTPUT(output, "invalid arguments for Add Request\n");
    return;
  }

  memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
  assert(sixp_pkt_set_cell_options(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)args->cmd,
                                   SIXP_PKT_CELL_OPTION_TX,
                                   sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
  assert(sixp_pkt_set_num_cells(SIXP_PKT_TYPE_REQUEST,
                                (sixp_pkt_code_t)(uint8_t)args->cmd,
                                1,
                                sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
  assert(sixp_pkt_set_cell_list(SIXP_PKT_TYPE_REQUEST,
                                (sixp_pkt_code_t)(uint8_t)args->cmd,
                                (const uint8_t *)&cell,
                                sizeof(cell), 0,
                                sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);
  assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                     (sixp_pkt_code_t)(uint8_t)args->cmd,
                     SF_PLUGTEST_SFID, sixp_pkt_buf,
                     sizeof(sixp_pkt_metadata_t) +
                     sizeof(sixp_pkt_cell_options_t) +
                     sizeof(sixp_pkt_num_cells_t) +
                     sizeof(cell),
                     &(args->peer_addr),
                     NULL, NULL, 0) == 0);

  SHELL_OUTPUT(output, "sent %s request [slot:%u, channel:%u]\n",
               args->cmd == SIXP_PKT_CMD_ADD ? "an Add" : "a Delete",
               args->slot_offset, args->channel_offset);
}

static void
count(shell_output_func output, subcmd_args_t *args)
{
  memset(&sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
  assert(sixp_pkt_set_cell_options(SIXP_PKT_TYPE_REQUEST,
                                   (sixp_pkt_code_t)(uint8_t)args->cmd,
                                   SIXP_PKT_CELL_OPTION_TX,
                                   sixp_pkt_buf, sizeof(sixp_pkt_buf)) == 0);

  assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                     (sixp_pkt_code_t)(uint8_t)args->cmd,
                     SF_PLUGTEST_SFID, sixp_pkt_buf,
                     sizeof(sixp_pkt_metadata_t) +
                     sizeof(sixp_pkt_cell_options_t),
                     &(args->peer_addr),
                     NULL, NULL, 0) == 0);

  SHELL_OUTPUT(output, "sent a Count request\n");
}

static void
list(shell_output_func output, subcmd_args_t *args)
{
  send_list_req(&(args->peer_addr));
  SHELL_OUTPUT(output, "sent a List request\n");
}

static void
clear(shell_output_func output, subcmd_args_t *args)
{
  cell_list_offset = 0;
  memset(sixp_pkt_buf, 0, sizeof(sixp_pkt_buf));
  assert(sixp_output(SIXP_PKT_TYPE_REQUEST,
                     (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_CLEAR,
                     SF_PLUGTEST_SFID, sixp_pkt_buf,
                     sizeof(sixp_pkt_metadata_t),
                     &(args->peer_addr),
                     NULL, NULL, 0) == 0);
  SHELL_OUTPUT(output, "sent a Clear request\n");
}

static int
parse_args(shell_output_func output,
           char *args, const char **subcmd, subcmd_args_t *subcmd_args)
{
  struct tsch_neighbor *time_source = NULL;
  char *next_args;
#if CONTIKI_TARGET_COOJA
  char *saveptr;
  char *octet;
  int i;
#endif /* CONTIKI_TARGET_COOJA */

  SHELL_ARGS_INIT(args, next_args);
  SHELL_ARGS_NEXT(args, next_args);

  if(args == NULL) {
    *subcmd = subcmd_help;
  } else {
    *subcmd = args;
  }

  if(strncmp("help", *subcmd, sizeof("help")) == 0) {
    /* we don't need peer_addr for the help sub-command. */
    return 0;
  }

  /* add and delete need slot_offset and channel_offset */
  if(strncmp("add", *subcmd, sizeof("add")) == 0 ||
     strncmp("delete", *subcmd, sizeof("delete")) == 0) {
    SHELL_ARGS_NEXT(args, next_args);
    if(args != NULL) {
      subcmd_args->slot_offset = strtol(args, NULL, 10);
    } else {
      return -1;
    }
    SHELL_ARGS_NEXT(args, next_args);
    if(args != NULL) {
      subcmd_args->channel_offset = strtol(args, NULL, 10);
    } else {
      return -1;
    }
  }

  SHELL_ARGS_NEXT(args, next_args);
  if(args != NULL) {
#if CONTIKI_TARGET_COOJA
    for(octet = strtok_r(args, ":", &saveptr), i = 0;
        octet != NULL && i < LINKADDR_SIZE;
        octet = strtok_r(NULL, ":", &saveptr), i++) {
      subcmd_args->peer_addr.u8[i] = strtol(octet, NULL, 16);
    }
    if(i > 1 && i != LINKADDR_SIZE) {
      /* invalid MAC address */
      memset(&(subcmd_args->peer_addr), 0, sizeof(linkaddr_t));
      return -1;
    }
#else
    SHELL_OUTPUT(output, "MAC address cannot be specified on this platform\n");
    return -1;
#endif /* CONTIKI_TARGET_COOJA */
  } else {
    if((time_source = tsch_queue_get_time_source()) == NULL) {
      SHELL_OUTPUT(output, "time source is not available\n");
      return -1;
    } else {
      memcpy(&(subcmd_args->peer_addr), tsch_queue_get_nbr_address(time_source),
             sizeof(linkaddr_t));
    }
  }

  return 0;
}

static void
shell_subcmd(shell_output_func output, char *args)
{
  const char *subcmd;
  subcmd_args_t subcmd_args;

  int i;

  if(shell_output == NULL) {
    shell_output = output;
  }

  if(parse_args(output, args, &subcmd, &subcmd_args) < 0) {
    SHELL_OUTPUT(output,
                 "invalid argument; command argument parse error\n");
    return;
  }

  for(i = 0; subcmds[i].name != NULL; i++) {
    if(strcmp(subcmds[i].name, subcmd) == 0) {
      subcmd_args.cmd = subcmds[i].cmd;
      subcmds[i].func(output, &subcmd_args);
    }
  }
}

static void
init(void)
{
  shell_commands_set_6top_sub_cmd(shell_subcmd);
}

static void
input(sixp_pkt_type_t type, sixp_pkt_code_t code,
      const uint8_t *body, uint16_t body_len,
      const linkaddr_t *src_addr)
{
  sixp_trans_t *trans;
  sixp_pkt_cmd_t cmd;
  int i;

  switch(type) {
    case SIXP_PKT_TYPE_REQUEST:
      cmd = code.cmd;
      break;
    case SIXP_PKT_TYPE_RESPONSE:
      if((trans = sixp_trans_find(src_addr)) == NULL) {
        LOG_ERR("internal error; cannot find a trans\n");
        return;
      }
      cmd = sixp_trans_get_cmd(trans);
      break;
    default:
      LOG_ERR("unsupported type %u by sf-plugtest\n", type);
      return;
  }

  if(type == SIXP_PKT_TYPE_RESPONSE &&
     code.rc != SIXP_PKT_RC_SUCCESS &&
     cmd != SIXP_PKT_CMD_LIST &&
     code.rc != SIXP_PKT_RC_EOL) {
    LOG_ERR("received return code of %u\n", code.value);
    return;
  }

  for(i = 0; i < sizeof(handlers); i++) {
    if(handlers[i].cmd == cmd) {
      if(type == SIXP_PKT_TYPE_REQUEST && handlers[i].req != NULL) {
        handlers[i].req(src_addr, body, body_len);
        return;
      }
      if(type == SIXP_PKT_TYPE_RESPONSE && handlers[i].res != NULL) {
        handlers[i].res(src_addr, code.rc, body, body_len);
        return;
      }
      i = sizeof(handlers);
      break;
    }
  }

  if(i == sizeof(handlers)) {
    LOG_ERR("unsupported command %u by sf-plugtest\n",
                       code.cmd);
  }
}

static void
timeout(sixp_pkt_cmd_t cmd, const linkaddr_t *peer_addr)
{
  LOG_ERR("transaction timeout\n");
}

const sixtop_sf_t sf_plugtest = {
  SF_PLUGTEST_SFID,
  SF_PLUGTEST_TIMEOUT,
  init,
  input,
  timeout,
  NULL
};
