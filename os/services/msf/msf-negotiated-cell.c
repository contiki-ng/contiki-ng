/*
 * Copyright (c) 2019, Inria.
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

/**
 * \file
 *         MSF Negotiated Cell APIs
 * \author
 *         Yasuyuki Tanaka <yasuyuki.tanaka@inria.fr>
 */

#include <contiki.h>
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/memb.h>
#include <lib/random.h>
#include <sys/log.h>

#include <net/mac/tsch/tsch.h>
#include <net/mac/tsch/sixtop/sixp.h>
#include <net/mac/tsch/sixtop/sixp-pkt.h>
#include <net/mac/tsch/sixtop/sixp-trans.h>

#include "msf.h"
#include "msf-autonomous-cell.h"
#include "msf-negotiated-cell.h"

#define LOG_MODULE "MSF"
#define LOG_LEVEL LOG_LEVEL_6TOP

/* definitions */
typedef enum {
  MSF_NEGOTIATED_TX_CELL,
  MSF_NEGOTIATED_RX_CELL
} msf_negotiated_cell_type_t;

typedef struct tsch_link tsch_link_t;

typedef struct {
  tsch_link_t *next;
  uint16_t num_tx;
  uint16_t num_tx_ack;
} msf_negotiated_tx_cell_data_t;

/* variables */
extern struct tsch_asn_divisor_t tsch_hopping_sequence_length;
extern tsch_link_t *msf_autonomous_rx_cell;
extern bool msf_is_activated;

static const linkaddr_t *parent_addr;
static int num_required_upward_cells;
static uint16_t num_cells_elapse;
static uint16_t num_cells_used;
static tsch_link_t *cell_to_relocate;
static struct timer request_wait_timer;

MEMB(msf_negotiated_tx_cell_data_memb,
     msf_negotiated_tx_cell_data_t,
     MSF_MAX_NUM_NEGOTIATED_TX_CELLS);

PROCESS(msf_negotiated_cell_management_process, "MSF negotiated cell magement");

/* static functions */
static void update_num_required_upward_cells(void);
static struct tsch_slotframe *get_slotframe(void);
static struct tsch_slotframe *add_slotframe(void);
static int add_negotiated_cell(msf_negotiated_cell_type_t type,
                               const linkaddr_t *peer_addr,
                               uint16_t timeslot, uint16_t channel_offset);
static int remove_negotiated_cell(tsch_link_t *cell_to_remove);
static unsigned int get_num_negotiated_tx_cells(const linkaddr_t *peer_addr);
static void set_cell_params(sixp_pkt_cell_t *cell,
                            uint16_t timeslot, uint16_t channel_offset);
static void get_cell_params(const sixp_pkt_cell_t *cell,
                            uint16_t *timeslot, uint16_t *channel_offset);
static tsch_link_t *find_reserved_cell_head(const linkaddr_t *peer_addr);
static tsch_link_t *reserve_cell(const linkaddr_t *peer_addr,
                                 uint16_t timeslot,
                                 uint16_t channel_offset);
static void release_reserved_cells(tsch_link_t *reserved_cell_head);
static tsch_link_t *prepare_cell_list_to_return(
  const linkaddr_t *peer_addr,
  const sixp_pkt_cell_t *cell_list, uint8_t cell_list_len,
  const sixp_pkt_cell_t **cell_list_to_return);
static tsch_link_t *prepare_candidate_cell_list(const linkaddr_t *peer_addr,
                                                sixp_pkt_cell_t *cell_list,
                                                uint8_t cell_list_len);
static const sixp_pkt_cell_t *pick_available_cell(
  const sixp_pkt_cell_t *cell_list, uint16_t cell_list_len);
static tsch_link_t *get_cell_to_relocate(void);
static void set_random_wait_to_next_request(void);
static void add_send_request(msf_negotiated_cell_type_t cell_type);
static void add_recv_request(const linkaddr_t *peer_addr,
                             const uint8_t *body, uint16_t body_len);
static void add_send_response(const linkaddr_t *peer_addr,
                              const sixp_pkt_cell_t *cell_list,
                              uint16_t cell_list_len);
static void add_recv_response(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc,
                              const uint8_t *body, uint16_t body_len);
static void add_callback_sent(void *arg, uint16_t arg_len,
                              const linkaddr_t *dest_addr,
                              sixp_output_status_t status);
static void delete_send_request(msf_negotiated_cell_type_t type);
static void delete_recv_request(const linkaddr_t *peer_addr,
                                const uint8_t *body, uint16_t body_len);
static void delete_send_response(const linkaddr_t *peer_addr,
                                 const sixp_pkt_cell_t *cell_list,
                                 uint16_t cell_list_len);
static void delete_recv_response(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc,
                                 const uint8_t *body, uint16_t body_len);
static void delete_callback_sent(void *arg, uint16_t arg_len,
                                 const linkaddr_t *dest_addr,
                                 sixp_output_status_t status);
static void relocate_send_request(const tsch_link_t *cell);
static void relocate_recv_request(const linkaddr_t *peer_addr,
                                  const uint8_t *body, uint16_t body_len);
static void relocate_send_response(const linkaddr_t *peer_addr,
                                   const sixp_pkt_cell_t *relocation_cell_list,
                                   uint16_t relocation_cell_list_len,
                                   const sixp_pkt_cell_t *candidate_cell_list,
                                   uint16_t candidate_cell_list_len);
static void relocate_recv_response(const linkaddr_t *peer_addr,
                                   sixp_pkt_rc_t rc,
                                   const uint8_t *body, uint16_t body_len);
static void relocate_callback_sent(void *arg, uint16_t arg_len,
                                   const linkaddr_t *dest_addr,
                                   sixp_output_status_t status);
static void clear_send_request(const linkaddr_t *peer_addr);
static void clear_recv_request(const linkaddr_t *peer_addr);
static void clear_send_response(const linkaddr_t *peer_addr);
static void clear_recv_response(const linkaddr_t *peer_addr);
static void clear_callback_sent(void *arg, uint16_t arg_len,
                                const linkaddr_t *dest_addr,
                                sixp_output_status_t status);
static void count_send_request(const linkaddr_t *peer_addr);
static void init(void);
static void input_handler(sixp_pkt_type_t type, sixp_pkt_code_t code,
                          const uint8_t *body, uint16_t body_len,
                          const linkaddr_t *src_addr);
static void timeout_handler(sixp_pkt_cmd_t cmd,
                            const linkaddr_t *peer_addr);
static void error_handler(sixp_error_t err, sixp_pkt_cmd_t cmd,
                          uint8_t seqno, const linkaddr_t *peer_addr);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(msf_negotiated_cell_management_process, ev, data)
{
  static struct etimer et;
  static struct timer t;
  const clock_time_t slotframe_interval = (CLOCK_SECOND /
                                           TSCH_SLOTS_PER_SECOND *
                                           MSF_SLOTFRAME_LENGTH);
  unsigned int num_negotiated_tx_cells;

  PROCESS_BEGIN();
  etimer_set(&et, slotframe_interval);
  timer_set(&t, MSF_HOUSEKEEPING_COLLISION_PERIOD);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    etimer_reset(&et);

    if(msf_is_activated == false ||
       parent_addr == NULL) {
      continue;
    }

    /* update the counters */
    num_negotiated_tx_cells = get_num_negotiated_tx_cells(parent_addr);
    num_cells_elapse += num_negotiated_tx_cells;

    if(num_cells_elapse >= MSF_MAX_NUM_CELLS) {
      update_num_required_upward_cells();
      /* reset the counters */
      num_cells_elapse = 0;
      num_cells_used = 0;
    }

    /* decide to relocate a cell or not */
    if(timer_expired(&t) && cell_to_relocate == NULL) {
      cell_to_relocate = get_cell_to_relocate();
      timer_restart(&t);
    }

    /* start an ADD or a DELETE transaction if necessary and possible */
    if(sixp_trans_find(parent_addr) == NULL &&
       timer_expired(&request_wait_timer)) {
      if(num_negotiated_tx_cells < num_required_upward_cells) {
        add_send_request(MSF_NEGOTIATED_TX_CELL);
      } else if(num_negotiated_tx_cells > num_required_upward_cells) {
        delete_send_request(MSF_NEGOTIATED_TX_CELL);
      } else if(cell_to_relocate != NULL) {
        relocate_send_request(cell_to_relocate);
      } else {
        /* nothing to do*/
      }
    } else {
      /*
       * we cannot send a request since 're busy on an on-going
       * transaction with the parent. try it later.
       */
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
update_num_required_upward_cells(void)
{
  unsigned int num_negotiated_tx_cells;
  num_negotiated_tx_cells = get_num_negotiated_tx_cells(parent_addr);

  /*
   * we're evaluating NumCellsUsed/NumCellsElapse, although we
   * cannot have a precise usage value as described in the MSF
   * draft. NumCellsUsed could be larger than NumCellsElapse. we
   * may overcount or undercount the number of elapsed negotiated
   * cells. and, num_cells_used may have a count of the autonomous
   * TX cell.
   */
  LOG_DBG("NumCellsElapsed: %u, "
          "NumCellsUsed: %u, NumRequiredUpwardCells: %u",
          num_cells_elapse, num_cells_used, num_required_upward_cells);
  if(num_cells_used > MSF_LIM_NUM_CELLS_USED_HIGH &&
     num_negotiated_tx_cells < MSF_MAX_NUM_NEGOTIATED_TX_CELLS &&
     num_required_upward_cells != (num_negotiated_tx_cells + 1)) {
    num_required_upward_cells = num_negotiated_tx_cells + 1;
    LOG_DBG_(" -> %u; ", num_required_upward_cells);
    LOG_DBG_("going to add another negotiated TX cell\n");
  } else if(num_cells_used < MSF_LIM_NUM_CELLS_USED_LOW &&
            num_negotiated_tx_cells > 1 &&
            num_required_upward_cells != (num_negotiated_tx_cells - 1)) {
    num_required_upward_cells = num_negotiated_tx_cells - 1;
    LOG_DBG_(" -> %u; ", num_required_upward_cells);
    LOG_DBG_("going to delete a negotiated TX cell\n");
  } else {
    /*
     *  we have a right amount of negotiated cells for the latest
     *  traffic
     */
    LOG_DBG_("\n");
  }
}
/*---------------------------------------------------------------------------*/
static struct tsch_slotframe *
get_slotframe(void)
{
  const uint16_t slotframe_handle = MSF_SLOTFRAME_HANDLE_NEGOTIATED_CELLS;
  return tsch_schedule_get_slotframe_by_handle(slotframe_handle);
}
/*---------------------------------------------------------------------------*/
static struct tsch_slotframe *
add_slotframe(void)
{
  return tsch_schedule_add_slotframe(MSF_SLOTFRAME_HANDLE_NEGOTIATED_CELLS,
                                     MSF_SLOTFRAME_LENGTH);
}
/*---------------------------------------------------------------------------*/
static int
add_negotiated_cell(msf_negotiated_cell_type_t type,
                    const linkaddr_t *peer_addr,
                    uint16_t timeslot, uint16_t channel_offset)
{
  struct tsch_slotframe *slotframe = get_slotframe();
  struct tsch_neighbor *nbr = tsch_queue_add_nbr(peer_addr);
  uint8_t cell_options;
  const char* cell_type_str;
  tsch_link_t *cell_to_add;

  assert(peer_addr != NULL);
  assert(slotframe != NULL);

  /* release all the reserved cells if any */
  release_reserved_cells(find_reserved_cell_head((peer_addr)));

  if(type == MSF_NEGOTIATED_TX_CELL) {
    cell_options = LINK_OPTION_TX;
    cell_type_str = "TX";
  } else {
    assert(type == MSF_NEGOTIATED_RX_CELL);
    cell_options = LINK_OPTION_RX;
    cell_type_str = "RX";
  }

  if(nbr == NULL) {
    LOG_ERR("failed to add a negotiated %s cell because nbr is not available\n",
            cell_type_str);
    return -1;
  }

  cell_to_add = tsch_schedule_add_link(slotframe, cell_options,
                                       LINK_TYPE_NORMAL, peer_addr,
                                       timeslot, channel_offset);
  if(cell_to_add != NULL && type == MSF_NEGOTIATED_TX_CELL) {
    cell_to_add->data = memb_alloc(&msf_negotiated_tx_cell_data_memb);
    memset(cell_to_add->data, 0, sizeof(msf_negotiated_tx_cell_data_t));
  }

  if(cell_to_add == NULL ||
     (type == MSF_NEGOTIATED_TX_CELL && cell_to_add->data == NULL)){
    LOG_ERR("failed to add a negotiated %s cell for ", cell_type_str);
    LOG_ERR_LLADDR(peer_addr);
    LOG_ERR_(" at slot_offset:%u, channel_offset:%u\n",
             timeslot, channel_offset);
  } else {
    LOG_INFO("added a negotiated %s cell for ", cell_type_str);
    LOG_INFO_LLADDR(peer_addr);
    LOG_INFO_(" at slot_offset:%u, channel_offset:%u\n",
              timeslot, channel_offset);

    if(type == MSF_NEGOTIATED_TX_CELL) {
      /* sort the list in reverse order of timeslot */
      tsch_link_t *curr = nbr->negotiated_tx_cell;
      tsch_link_t *prev = NULL;
      while(curr != NULL) {
        if(cell_to_add->timeslot > curr->timeslot) {
          ((msf_negotiated_tx_cell_data_t *)cell_to_add->data)->next = curr;
          if(prev == NULL) {
            /* cell_to_add has the largest timeslot in the list */
            nbr->negotiated_tx_cell = cell_to_add;
          } else {
            ((msf_negotiated_tx_cell_data_t *)prev->data)->next = cell_to_add;
          }
          break;
        } else {
          prev = curr;
          curr = ((msf_negotiated_tx_cell_data_t *)curr->data)->next;
        }
      }
      if(curr == NULL) {
        if(nbr->negotiated_tx_cell == NULL) {
          nbr->negotiated_tx_cell = cell_to_add;
        } else {
          /* put the cell to the end */
          ((msf_negotiated_tx_cell_data_t *)prev->data)->next = cell_to_add;
        }
      }

      if(nbr->autonomous_tx_cell != NULL) {
        /* we don't need the autonomous TX cell any more */
        msf_autonomous_cell_delete(nbr->autonomous_tx_cell);
        nbr->autonomous_tx_cell = NULL;
      }
    }
  }

  return cell_to_add == NULL ? -1 : 0;
}
/*---------------------------------------------------------------------------*/
static int
remove_negotiated_cell(tsch_link_t *cell_to_remove)
{
  struct tsch_slotframe *slotframe = get_slotframe();
  linkaddr_t peer_addr;
  const char *cell_type_str;

  assert(slotframe != NULL);
  assert(cell_to_remove != NULL);
  linkaddr_copy(&peer_addr, &cell_to_remove->addr);

  if(cell_to_remove->link_options == LINK_OPTION_TX) {
    struct tsch_neighbor *nbr;
    tsch_link_t *cell;
    tsch_link_t *prev_cell = NULL;

    nbr = tsch_queue_get_nbr(&peer_addr);
    assert(nbr != NULL);
    assert(nbr->negotiated_tx_cell != NULL);
    /* update the chain of the negotiated TX cells */
    for(cell = nbr->negotiated_tx_cell;
        cell != NULL;
        cell = ((msf_negotiated_tx_cell_data_t *)cell->data)->next) {
      if(cell == cell_to_remove) {
        if(prev_cell == NULL) {
          nbr->negotiated_tx_cell =
            ((msf_negotiated_tx_cell_data_t *)cell_to_remove->data)->next;
        } else {
          ((msf_negotiated_tx_cell_data_t *)prev_cell->data)->next =
            ((msf_negotiated_tx_cell_data_t *)cell_to_remove->data)->next;
        }
        break;
      } else {
        prev_cell = cell;
      }
    }
    assert(cell != NULL);
    assert(cell_to_remove->data != NULL);
    memb_free(&msf_negotiated_tx_cell_data_memb, cell_to_remove->data);
    cell_type_str = "TX";
  } else {
    assert(cell_to_remove->link_options == LINK_OPTION_RX);
    cell_type_str = "RX";
  }

  if(tsch_schedule_remove_link(slotframe, cell_to_remove) == 0) {
    LOG_ERR("failed to remove a negotiated %s cell for ", cell_type_str);
    LOG_ERR_LLADDR(&peer_addr);
    LOG_ERR_(" at slot_offset:%u, channel_offset:%u\n",
             cell_to_remove->timeslot, cell_to_remove->channel_offset);
    return -1;
  } else {
    LOG_INFO("removed a negotiated %s cell for ", cell_type_str);
    LOG_INFO_LLADDR(&peer_addr);
    LOG_INFO_(" at slot_offset:%u, channel_offset:%u\n",
              cell_to_remove->timeslot, cell_to_remove->channel_offset);
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
static unsigned int
get_num_negotiated_tx_cells(const linkaddr_t *peer_addr)
{
  struct tsch_neighbor *nbr;
  int ret;

  if(peer_addr == NULL) {
    nbr = NULL;
  } else {
    nbr = tsch_queue_get_nbr(peer_addr);
  }

  if(nbr == NULL) {
    ret = 0;
  } else {
    ret = 0;
    for(tsch_link_t *cell = nbr->negotiated_tx_cell;
        cell != NULL;
        cell = ((msf_negotiated_tx_cell_data_t *)cell->data)->next) {
      /* all the negotiated TX cells should have msf_negotiated_tx_cell_data */
      assert(cell->data != NULL);
      ret++;
    }
  }

  return ret;
}
/*---------------------------------------------------------------------------*/
static void
set_cell_params(sixp_pkt_cell_t *cell,
                uint16_t timeslot, uint16_t channel_offset)
{
  uint8_t *p = (uint8_t *)cell;
  p[0] = timeslot & 0xff;
  p[1] = timeslot >> 8;
  p[2] = channel_offset & 0xff;
  p[3] = channel_offset >> 8;
}
/*---------------------------------------------------------------------------*/
static void
get_cell_params(const sixp_pkt_cell_t *cell,
                uint16_t *timeslot, uint16_t *channel_offset)
{
  uint8_t *p = (uint8_t *)cell;
  *timeslot = p[0] + (p[1] << 8);
  *channel_offset = p[2] + (p[3] << 8);
}
/*---------------------------------------------------------------------------*/
static tsch_link_t *
find_reserved_cell_head(const linkaddr_t *peer_addr)
{
  struct tsch_slotframe *slotframe = get_slotframe();
  tsch_link_t *cell;

  assert(peer_addr != NULL);
  assert(slotframe != NULL);

  for(cell = list_head(slotframe->links_list);
      cell != NULL;
      cell = list_item_next(cell)) {
    if(linkaddr_cmp(&cell->addr, peer_addr) &&
       cell->link_options == 0) {
      /*
       * we expect the head to appear first in links_list since it
       * was added earlier than others.
       */
      break;
    }
  }

  return cell;
}
/*---------------------------------------------------------------------------*/
static tsch_link_t *
reserve_cell(const linkaddr_t *peer_addr,
             uint16_t timeslot, uint16_t channel_offset)
{
  struct tsch_slotframe *slotframe = get_slotframe();
  tsch_link_t *tsch_link;
  const uint8_t no_link_options = 0;

  assert(slotframe != NULL);

  tsch_link = tsch_schedule_add_link(slotframe,
                                     no_link_options, LINK_TYPE_NORMAL,
                                     peer_addr, timeslot, channel_offset);
  if(tsch_link == NULL) {
    LOG_ERR("failed to reserve a cell at slot_offset:%u, channel_offset:%u\n",
            timeslot, channel_offset);
  } else {
    LOG_INFO("reserved a cell at slot_offset:%u, channel_offset:%u\n",
             timeslot, channel_offset);
  }

  return tsch_link;
}
/*---------------------------------------------------------------------------*/
static void
release_reserved_cells(tsch_link_t *reserved_cell_head)
{
  struct tsch_slotframe *slotframe = get_slotframe();
  tsch_link_t *cell;
  uint16_t timeslot, channel_offset;

  assert(slotframe != NULL);
  assert(reserved_cell_head != NULL);

  while(reserved_cell_head != NULL) {
    cell = reserved_cell_head;
    reserved_cell_head = (tsch_link_t*)cell->data;
    timeslot = cell->timeslot;
    channel_offset = cell->channel_offset;
    if(tsch_schedule_remove_link(slotframe, cell) == 0) {
      LOG_ERR("failed to release a reserved cell at "
              "slot_offset:%u, channel_offset:%u\n", timeslot, channel_offset);
    } else {
      LOG_INFO("released a reserved cell at "
               "slot_offset:%u, channel_offset:%u\n", timeslot, channel_offset);
    }
  }
}
/*---------------------------------------------------------------------------*/
static tsch_link_t *
prepare_cell_list_to_return(const linkaddr_t *peer_addr,
                            const sixp_pkt_cell_t *cell_list,
                            uint8_t cell_list_len,
                            const sixp_pkt_cell_t **cell_list_to_return)
{
  tsch_link_t *reserved_cell;
  const sixp_pkt_cell_t *cell;

  if((cell = pick_available_cell(cell_list, cell_list_len)) == NULL) {
    /* no cell is available on our side */
    LOG_INFO("none of cells in received CellList is available\n");
    reserved_cell = NULL;
  } else {
    uint16_t timeslot;
    uint16_t channel_offset;
    get_cell_params(cell, (uint16_t *)&timeslot, &channel_offset);
    if((reserved_cell = reserve_cell(peer_addr,
                                     timeslot, channel_offset)) == NULL) {
      /* we're going to send an empty CellList */
      reserved_cell = NULL;
    } else {
      /* make sure reserved_cell doesn't have a "next" cell */
      reserved_cell->data = NULL;
    }
  }
  if(reserved_cell == NULL) {
    *cell_list_to_return = NULL;
  } else {
    *cell_list_to_return = cell;
  }
  return reserved_cell;
}
/*---------------------------------------------------------------------------*/
static tsch_link_t *
prepare_candidate_cell_list(const linkaddr_t *peer_addr,
                            sixp_pkt_cell_t *cell_list, uint8_t cell_list_len)
{
  struct tsch_slotframe *slotframe = get_slotframe();
  sixp_pkt_cell_t *cell;
  uint16_t slot_offset_base;
  int32_t slot_offset;
  uint16_t channel_offset;
  uint8_t num_reserved_cells = 0;
  tsch_link_t *link = NULL;
  tsch_link_t *reserved_cell_head = NULL;
  tsch_link_t *reserved_cell_tail = NULL;

  assert(slotframe != NULL);
  assert(msf_autonomous_rx_cell != NULL);

  for(int i = 0; i < cell_list_len; i++) {
    cell = &cell_list[i];

    /* look for an used slot */
    slot_offset_base = random_rand() % slotframe->size.val;
    slot_offset = -1;
    for(int j = 0; j < slotframe->size.val; j++) {
      slot_offset = (slot_offset_base + j) % slotframe->size.val;
      if(tsch_schedule_get_link_by_timeslot(slotframe, slot_offset) == NULL &&
         slot_offset != 0 &&
         slot_offset != msf_autonomous_rx_cell->timeslot) {
        /* found one, which we're going to put into the cell list */
        break;
      } else {
        /* try the next one */
        slot_offset = -1;
        continue;
      }
    }

    if(slot_offset < 0) {
      /* we don't have enough slots available; get out of this loop */
      break;
    } else {
      /* slot_offset must not be zero */
      assert(slot_offset > 0);
      channel_offset = random_rand() % tsch_hopping_sequence_length.val;

      if((link = reserve_cell(peer_addr, slot_offset, channel_offset)) ==
         NULL) {
        /* we failed to reserve the cell for some reason; abort */
        break;
      } else {
        /* we use "data" as "next" cell in a chain of reserved cells */
        if(reserved_cell_head == NULL) {
          assert(reserved_cell_tail == NULL);
          reserved_cell_head = link;
        } else {
          reserved_cell_tail->data = (void *)link;
          reserved_cell_tail = link;
        }
        reserved_cell_tail = link;
        link->data = NULL;
        set_cell_params(cell, slot_offset, channel_offset);
        num_reserved_cells++;
      }
    }
  }

  if(num_reserved_cells != cell_list_len) {
    /* if something went wrong, we release all the reserved cells */
    while(reserved_cell_head != NULL && link != NULL) {
      link = (tsch_link_t *)link->data;
      if(link->data == link) {
        /* link is the tail; we don't have any link left */
        reserved_cell_head = NULL;
      } else {
        reserved_cell_head = link->data;
      }
      if(tsch_schedule_remove_link(slotframe, link) == 0) {
        LOG_ERR("failed to remove a reserved cell at "
                "slot_offset:%u, channel_offset:%u\n",
                link->timeslot, link->channel_offset);
      } else {
        LOG_INFO("removed a reserved cell at "
                 "timeslot:%u, channel_offset:%u\n",
                 link->timeslot, link->channel_offset);
      }
    }
    reserved_cell_head = NULL;
  }

  return reserved_cell_head;
}

/*---------------------------------------------------------------------------*/
static const sixp_pkt_cell_t *
pick_available_cell(const sixp_pkt_cell_t *cell_list, uint16_t cell_list_len)
{
  struct tsch_slotframe *slotframe = get_slotframe();
  const sixp_pkt_cell_t *cell;
  uint16_t timeslot;
  uint16_t channel_offset;

  assert(slotframe != NULL);
  assert(cell_list != NULL);
  assert(cell_list_len > 0);

  for(int i = 0; i < cell_list_len; i++) {
    cell = &cell_list[i];
    get_cell_params(cell, &timeslot, &channel_offset);
    if(tsch_schedule_get_link_by_timeslot(slotframe, timeslot) == NULL) {
      /* this slot offset is not used */
      break;
    } else {
      /* try next */
      cell = NULL;
    }
  }

  return cell;
}
/*---------------------------------------------------------------------------*/
static tsch_link_t *
get_cell_to_relocate(void)
{
  struct tsch_neighbor *nbr = tsch_queue_get_nbr(parent_addr);
  tsch_link_t *cell;
  tsch_link_t *cell_to_relocate = NULL;
  msf_negotiated_tx_cell_data_t *cell_data;
  tsch_link_t *worst_pdr_cell = NULL;
  int16_t best_pdr = -1; /* initialized with an invalid value for PDR */
  uint16_t worst_pdr;
  int16_t pdr;

  assert(parent_addr != NULL);
  assert(nbr != NULL);

  for(cell = nbr->negotiated_tx_cell; cell != NULL; cell = cell_data->next) {
    cell_data = (msf_negotiated_tx_cell_data_t *)cell->data;
    if(cell_data->num_tx < MSF_MIN_NUM_TX_FOR_RELOCATION) {
      /* we don't evaluate this cell since it's not used much enough yet */
      pdr = -1;
    } else {
      assert(cell_data->num_tx > 0);
      pdr = cell_data->num_tx_ack * 100 / cell_data->num_tx;

      if(best_pdr < 0 || pdr > best_pdr) {
        best_pdr = pdr;
      }

      if(worst_pdr_cell == NULL || pdr < worst_pdr) {
        worst_pdr_cell = cell;
        worst_pdr = pdr;
        assert(best_pdr >= worst_pdr);
      }
    }
    LOG_DBG("cell[slot_offset: %3u, channel_offset: %3u] -- ",
            cell->timeslot, cell->channel_offset);
    LOG_DBG_("NumTx: %u, NumTxAck: %u ",
             cell_data->num_tx, cell_data->num_tx_ack);
    if(pdr < 0) {
      LOG_DBG_("PDR: N/A\n");
    } else {
      LOG_DBG_("PDR: %d\%\n", pdr);
    }
  }

  if(best_pdr < 0) {
    cell_to_relocate = NULL;
  } else {
    assert(worst_pdr >= 0);
    LOG_DBG("best PDR is %d\%, worst PDR is %u\%", best_pdr, worst_pdr);
    if((best_pdr - worst_pdr) <= MSF_RELOCATE_PDR_THRESHOLD) {
      /* worst_pdr_cell is not so bad to relocate */
      LOG_DBG_("\n");
      cell_to_relocate = NULL;
    } else {
      cell_to_relocate = worst_pdr_cell;
      LOG_DBG_("; going to relocate a TX cell"
               " [slot_offset: %u, channel_offset: %u]\n",
               cell_to_relocate->timeslot, cell_to_relocate->channel_offset);
    }
  }

  return cell_to_relocate;
}
/*---------------------------------------------------------------------------*/
static void
set_random_wait_to_next_request(void)
{
  clock_time_t wait_duration;
  unsigned short random_value = random_rand();

  assert(MSF_WAIT_DURATION_MIN < MSF_WAIT_DURATION_MAX);
  wait_duration = (MSF_WAIT_DURATION_MIN +
                   ((MSF_WAIT_DURATION_MAX - MSF_WAIT_DURATION_MIN) *
                    random_value /
                    RANDOM_RAND_MAX));

  assert(timer_expired(&request_wait_timer) != 0);
  timer_set(&request_wait_timer, wait_duration);
  LOG_DBG("delay the next request for %u seconds\n", wait_duration);
}
/*---------------------------------------------------------------------------*/
static void
add_send_request(msf_negotiated_cell_type_t cell_type)
{
  const sixp_pkt_type_t type = SIXP_PKT_TYPE_REQUEST;
  const sixp_pkt_code_t code = (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD;
  const size_t cell_list_len = sizeof(sixp_pkt_cell_t) * MSF_6P_CELL_LIST_LEN;
  const size_t body_len = (sizeof(sixp_pkt_metadata_t) +
                           sizeof(sixp_pkt_cell_options_t) +
                           sizeof(sixp_pkt_num_cells_t) +
                           cell_list_len);
  sixp_pkt_cell_t cell_list[MSF_6P_CELL_LIST_LEN];
  uint8_t body[body_len];
  sixp_pkt_cell_options_t cell_options = SIXP_PKT_CELL_OPTION_TX;
  const sixp_pkt_num_cells_t num_cells = 1;
  tsch_link_t *reserved_cell_head;

  assert(parent_addr != NULL);
  assert(cell_type == MSF_NEGOTIATED_TX_CELL);

  memset(body, 0, body_len);
  reserved_cell_head = prepare_candidate_cell_list(parent_addr,
                                                   cell_list,
                                                   MSF_6P_CELL_LIST_LEN);
  if(reserved_cell_head == NULL) {
    LOG_ERR("failed to send an ADD request; cannot make a CellList\n");
    set_random_wait_to_next_request();
    return;
  } else if(
    sixp_pkt_set_cell_options(type, code, cell_options, body, body_len) < 0 ||
    sixp_pkt_set_num_cells(type, code, num_cells, body, body_len) < 0 ||
    sixp_pkt_set_cell_list(type, code,
                           (const uint8_t *)cell_list, cell_list_len, 0,
                           body, body_len) < 0) {
    LOG_ERR("failed to send an ADD request; cannot build a body\n");
    release_reserved_cells(reserved_cell_head);
    set_random_wait_to_next_request();
  } else if(sixp_output(type, code, MSF_SFID,
                        body, body_len, parent_addr, add_callback_sent,
                        reserved_cell_head, sizeof(tsch_link_t)) < 0) {
    LOG_ERR("failed to send an ADD request\n");
    release_reserved_cells(reserved_cell_head);
    set_random_wait_to_next_request();
  } else {
    LOG_DBG("sent an ADD request to the parent: ");
    LOG_DBG_LLADDR(parent_addr);
    LOG_DBG_("\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
add_recv_request(const linkaddr_t *peer_addr,
                 const uint8_t *body, uint16_t body_len)
{
  const sixp_pkt_type_t type = SIXP_PKT_TYPE_REQUEST;
  const sixp_pkt_code_t code = (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD;
  sixp_pkt_cell_options_t cell_options;
  sixp_pkt_num_cells_t num_cells;
  const sixp_pkt_cell_t *cell_list;
  uint16_t cell_list_len;

  assert(peer_addr != NULL);

  if(sixp_pkt_get_cell_options(type, code, &cell_options, body, body_len) < 0 ||
     sixp_pkt_get_num_cells(type, code, &num_cells, body, body_len) < 0 ||
     sixp_pkt_get_cell_list(type, code,
                            (const uint8_t **)&cell_list, &cell_list_len,
                            body, body_len) < 0) {
    LOG_ERR("failed to process an ADD request; parse error\n");
  } else {
    assert(cell_options == SIXP_PKT_CELL_OPTION_TX);
    assert(num_cells == 1);
    assert(cell_list_len >= MSF_6P_CELL_LIST_MIN_LEN);
    LOG_DBG("received an ADD request from ");
    LOG_DBG_LLADDR(peer_addr);
    LOG_DBG_("\n");
    add_send_response(peer_addr, cell_list, cell_list_len);
  }
}
/*---------------------------------------------------------------------------*/
static void
add_send_response(const linkaddr_t *peer_addr,
                  const sixp_pkt_cell_t *cell_list, uint16_t cell_list_len)
{
  tsch_link_t *reserved_cell;
  const sixp_pkt_cell_t *cell_list_to_return;

  assert(peer_addr != NULL);
  assert(cell_list != NULL);

  if(cell_list_len < (sizeof(sixp_pkt_cell_t) * MSF_6P_CELL_LIST_MIN_LEN)) {
    /* invalid CellList length; send RC_ERR */
    LOG_ERR("received an invalid CellList whose length is too short\n");
    if(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_ERR,
                   MSF_SFID, NULL, 0, peer_addr,
                   NULL, NULL, 0) < 0) {
      LOG_ERR("failed to send an RC_ERR response\n");
    } else {
      LOG_DBG("sent an RC_ERR response\n");
    }
  } else {
    reserved_cell = prepare_cell_list_to_return(peer_addr,
                                                cell_list, cell_list_len,
                                                &cell_list_to_return);
    if(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                   MSF_SFID, (const uint8_t *)cell_list_to_return,
                   cell_list_to_return == NULL ? 0 : sizeof(sixp_pkt_cell_t),
                   peer_addr,
                   add_callback_sent,
                   reserved_cell,
                   reserved_cell == NULL ? 0 : sizeof(tsch_link_t)) < 0) {
      LOG_ERR("failed to send an RC_SUCCESS response to ");
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("\n");
    } else {
      LOG_DBG("sent an RC_SUCCESS response");
      if(reserved_cell == NULL) {
        LOG_DBG_(" with an empty CellList to ");
      } else {
        LOG_DBG_(" to ");
      }
      LOG_DBG_LLADDR(peer_addr);
      LOG_DBG_("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
add_recv_response(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc,
                 const uint8_t *body, uint16_t body_len)
{
  const sixp_pkt_cell_t *cell_list;
  uint16_t cell_list_len;
  uint16_t timeslot;
  uint16_t channel_offset;
  struct tsch_slotframe *slotframe = get_slotframe();
  tsch_link_t *cell;

  assert(peer_addr != NULL);
  assert(slotframe != NULL);

  if(rc == SIXP_PKT_RC_SUCCESS) {
    LOG_DBG("received an RC_SUCCESS response from ");
    LOG_DBG_LLADDR(peer_addr);
    LOG_DBG_("\n");
    if(sixp_pkt_get_cell_list(SIXP_PKT_TYPE_RESPONSE,
                              (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                              (const uint8_t **)&cell_list, &cell_list_len,
                              body, body_len) < 0) {
      LOG_ERR("failed to process RC_SUCCESS response; parse error\n");
    } else if(cell_list_len == 0) {
      LOG_INFO("received an empty CellList; try another ADD request later\n");
    } else if(body_len > sizeof(sixp_pkt_cell_t)) {
      /* invalid length since MSF always requests one cell per ADD request */
      LOG_ERR("received an invalid CellList whose length is %u\n", body_len);
    } else {
      assert(cell_list_len == sizeof(sixp_pkt_cell_t));
      get_cell_params(cell_list, &timeslot, &channel_offset);
      cell = tsch_schedule_get_link_by_timeslot(slotframe, timeslot);
      /* this cell should be one we reserved beforehand */
      if(cell == NULL ||
         cell->timeslot != timeslot ||
         cell->channel_offset != channel_offset ||
         linkaddr_cmp(&cell->addr, peer_addr) == 0 ||
         cell->link_options != 0) {
        LOG_ERR("SCHEDULE INCONSISTENCY is likely to happen; ");
        LOG_ERR_("received a cell which is not a reserved one\n");
        release_reserved_cells(find_reserved_cell_head(peer_addr));
      } else {
        /* the peer accepts one of reserved cells; let's activate it */
        if(add_negotiated_cell(MSF_NEGOTIATED_TX_CELL,
                               peer_addr, timeslot, channel_offset) < 0) {
          LOG_ERR("SCHEDULE INCONSISTENCY is likely to happen\n");
          /* better to have CLEAR or DELETE? */
        } else {
          LOG_DBG("ADD transaction completes successfully\n");
        }
      }
    }
  } else {
    release_reserved_cells(find_reserved_cell_head(peer_addr));
    if(rc == SIXP_PKT_RC_ERR) {
      /* the peer doesn't accept our 6P Request as a whole */
      LOG_ERR("received RC_ERR from ");
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("; we may run a different SF\n");
    } else if(rc == SIXP_PKT_RC_ERR_SEQNUM) {
      LOG_ERR("received RC_ERR_SEQNUM from ");
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("; going to send a CLEAR request\n");
      sixp_trans_abort(sixp_trans_find(peer_addr));
      clear_send_request(peer_addr);
    } else if(rc == SIXP_PKT_RC_ERR_BUSY) {
      LOG_ERR("received RC_ERR_BUSY from ");
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("; retry later\n");
      set_random_wait_to_next_request();
    } else {
      LOG_ERR("received RC %u from", rc);
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("; action to take is not defined\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
add_callback_sent(void *arg, uint16_t arg_len, const linkaddr_t *dest_addr,
                  sixp_output_status_t status)
{
  tsch_link_t *reserved_cell_head = (tsch_link_t *)arg;

  assert(dest_addr != NULL);

  if(status == SIXP_OUTPUT_STATUS_SUCCESS) {
    if(NETSTACK_ROUTING.node_is_root() ||
       (parent_addr != NULL &&
        linkaddr_cmp(dest_addr, parent_addr) == 0)) {
      /* we're the responder of the ADD transaction */
      if(reserved_cell_head == NULL) {
        /* we've returned an empty CellList; do nothing */
        assert(arg_len == 0);
      } else {
        assert(arg_len == sizeof(tsch_link_t));
        if(add_negotiated_cell(MSF_NEGOTIATED_RX_CELL, dest_addr,
                               reserved_cell_head->timeslot,
                               reserved_cell_head->channel_offset) < 0) {
          /* this allocation failure could cause schedule inconsistency */
          LOG_ERR("SCHEDULE INCONSISTENCY is likely to happen\n");
          /* better to have CLEAR or DELETE? */
        } else {
          LOG_DBG("ADD transaction completes successfully\n");
        }
      }
    } else {
      /*
       * our request is acknowledged by MAC layer of the peer; nothing
       * to do.
       */
    }
  } else {
    assert(status == SIXP_OUTPUT_STATUS_FAILURE ||
           status == SIXP_OUTPUT_STATUS_ABORTED);
    LOG_ERR("ADD transaction failed\n");
    /* if failing to send a request or a response, release the reserved cells */
    release_reserved_cells(reserved_cell_head);
    if(NETSTACK_ROUTING.node_is_root() ||
       parent_addr == NULL ||
       (linkaddr_cmp(dest_addr, parent_addr) == 0)) {
      /* we're the responder or lost our parent; no need to retry */
    } else {
      /* we're the initiator */
      set_random_wait_to_next_request();
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
delete_send_request(msf_negotiated_cell_type_t cell_type)
{
  const sixp_pkt_type_t type = SIXP_PKT_TYPE_REQUEST;
  const sixp_pkt_code_t code = (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE;
  struct tsch_neighbor *nbr;
  const size_t cell_list_len = sizeof(sixp_pkt_cell_t);
  const size_t body_len = (sizeof(sixp_pkt_metadata_t) +
                           sizeof(sixp_pkt_cell_options_t) +
                           sizeof(sixp_pkt_num_cells_t) +
                           cell_list_len);
  sixp_pkt_cell_t cell_to_delete;
  uint8_t body[body_len];
  sixp_pkt_cell_options_t cell_options = SIXP_PKT_CELL_OPTION_TX;
  const sixp_pkt_num_cells_t num_cells = 1;

  assert(parent_addr != NULL);
  assert(get_num_negotiated_tx_cells(parent_addr) > 1);
  assert(cell_type == MSF_NEGOTIATED_TX_CELL);

  memset(body, 0, body_len);

  nbr = tsch_queue_get_nbr(parent_addr);
  if(nbr == NULL ||
     nbr->negotiated_tx_cell == NULL ||
     nbr->negotiated_tx_cell->data == NULL) {
    /* this shouldn't happen, by the way */
    LOG_ERR("failed to send a DELETE request; tsch_neighbor for ");
    LOG_ERR_LLADDR(parent_addr);
    LOG_ERR_("is unavailable or invalid\n");
  } else {
    /* delete the first one in nbr */
    set_cell_params(&cell_to_delete,
                    nbr->negotiated_tx_cell->timeslot,
                    nbr->negotiated_tx_cell->channel_offset);

    /* build a body of DELETE request */
    if(sixp_pkt_set_cell_options(type, code,
                                 cell_options, body, body_len) < 0 ||
       sixp_pkt_set_num_cells(type, code, num_cells, body, body_len) < 0 ||
       sixp_pkt_set_cell_list(type, code,
                              (const uint8_t *)&cell_to_delete, cell_list_len,
                              0, body, body_len) < 0) {
      LOG_ERR("failed to send a DELETE request; cannot build a body\n");
      set_random_wait_to_next_request();
    } else if(sixp_output(type, code, MSF_SFID, body, body_len, parent_addr,
                          NULL, NULL, 0) < 0) {
      LOG_ERR("failed to send a DELETE request\n");
      set_random_wait_to_next_request();
    } else {
      LOG_DBG("sent a DELETE request to the parent: ");
      LOG_DBG_LLADDR(parent_addr);
      LOG_DBG_("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
delete_recv_request(const linkaddr_t *peer_addr,
                    const uint8_t *body, uint16_t body_len)
{
  const sixp_pkt_type_t type = SIXP_PKT_TYPE_REQUEST;
  const sixp_pkt_code_t code = (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_DELETE;
  sixp_pkt_cell_options_t cell_options;
  sixp_pkt_num_cells_t num_cells;
  const sixp_pkt_cell_t *cell_list;
  uint16_t cell_list_len;

  assert(peer_addr != NULL);

  if(sixp_pkt_get_cell_options(type, code, &cell_options,
                               body, body_len) < 0 ||
     sixp_pkt_get_num_cells(type, code, &num_cells,
                            body, body_len) < 0 ||
     sixp_pkt_get_cell_list(type, code,
                            (const uint8_t **)&cell_list, &cell_list_len,
                            body, body_len) < 0) {
    LOG_ERR("failed to process a DELETE request; parse error\n");
  } else {
    assert(cell_options == SIXP_PKT_CELL_OPTION_TX);
    assert(num_cells == 1);
    assert(cell_list_len == sizeof(sixp_pkt_cell_t));
    LOG_DBG("received a DELETE request from ");
    LOG_DBG_LLADDR(peer_addr);
    LOG_DBG_("\n");
    delete_send_response(peer_addr, cell_list, cell_list_len);
  }
}
/*---------------------------------------------------------------------------*/
static void
delete_send_response(const linkaddr_t *peer_addr,
                     const sixp_pkt_cell_t *cell_list, uint16_t cell_list_len)
{
  struct tsch_slotframe *slotframe = get_slotframe();
  tsch_link_t *cell;
  uint16_t timeslot;
  uint16_t channel_offset;
  sixp_pkt_rc_t rc;
  const char *rc_str;

  assert(slotframe != NULL);
  assert(peer_addr != NULL);
  assert(cell_list != NULL);

  if(cell_list == NULL || cell_list_len != sizeof(sixp_pkt_cell_t)) {
    /* invalid value for DELETE request of MSF */
    rc = SIXP_PKT_RC_ERR;
    rc_str = "RC_ERR";
  } else {
    get_cell_params(cell_list, &timeslot, &channel_offset);
    cell = tsch_schedule_get_link_by_timeslot(slotframe, timeslot);
    if(cell == NULL ||
       cell->channel_offset != channel_offset ||
       linkaddr_cmp(&cell->addr, peer_addr) == 0) {
      rc = SIXP_PKT_RC_ERR_CELLLIST;
      rc_str = "RC_ERR_CELLLIST";
    } else {
      rc = SIXP_PKT_RC_SUCCESS;
      rc_str = "RC_SUCCESS";
    }
  }

  if(sixp_output(SIXP_PKT_TYPE_RESPONSE, (sixp_pkt_code_t)(uint8_t)rc,
                 MSF_SFID,
                 rc == SIXP_PKT_RC_SUCCESS ? (const uint8_t *)cell_list : NULL,
                 rc == SIXP_PKT_RC_SUCCESS ? cell_list_len : 0,
                 peer_addr,
                 delete_callback_sent, cell, sizeof(tsch_link_t)) < 0) {
    LOG_ERR("failed to send an %s response to ", rc_str);
    LOG_ERR_LLADDR(peer_addr);
    LOG_ERR_("\n");
  } else {
    LOG_DBG("sent an %s response to ", rc_str);
    LOG_DBG_LLADDR(peer_addr);
    LOG_DBG_("\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
delete_recv_response(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc,
                     const uint8_t *body, uint16_t body_len)
{
  struct tsch_neighbor *nbr = tsch_queue_get_nbr(parent_addr);
  const sixp_pkt_cell_t *cell_list;
  uint16_t cell_list_len;
  uint16_t timeslot;
  uint16_t channel_offset;

  assert(peer_addr != NULL);
  assert(nbr != NULL);
  assert(cell_list != NULL);

  if(rc == SIXP_PKT_RC_SUCCESS) {
    if(sixp_pkt_get_cell_list(SIXP_PKT_TYPE_RESPONSE,
                              (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                              (const uint8_t **)&cell_list, &cell_list_len,
                              body, body_len) < 0) {
      LOG_ERR("failed to process RC_SUCCESS response; parse error\n");
    } else if(cell_list_len == 0) {
      LOG_INFO("received an empty CellList; shouldn't happen\n");
    } else {
      tsch_link_t *cell_to_delete = NULL;
      get_cell_params(cell_list, &timeslot, &channel_offset);
      for(tsch_link_t *_cell = nbr->negotiated_tx_cell;
          _cell != NULL;
          _cell = (tsch_link_t *)_cell->data) {
        /* confirm we have the cell in our schedule */
        if((_cell->timeslot == timeslot) &&
           (_cell->channel_offset == channel_offset)) {
          cell_to_delete = _cell;
          break;
        }
      }
      if(cell_to_delete == NULL ||
         remove_negotiated_cell(cell_to_delete) < 0) {
        /*
         * when cell_to_delete is NULL, the peer is going to delete a
         * cell which is not a negotiated TX cell on our side
         */
        LOG_ERR("SCHEDULE INCONSISTENCY is likely to happen\n");
      } else {
        LOG_DBG("DELETE transaction completes successfully\n");
      }
    }
  } else {
    if(rc == SIXP_PKT_RC_ERR) {
      /* the peer doesn't accept our 6P Request as a whole */
      LOG_ERR("received RC_ERR; we may run a different SF\n");
    } else if(rc == SIXP_PKT_RC_ERR_CELLLIST) {
      LOG_ERR("received RC_ERR_CELLLIST; the peer doesn't have the cell\n");
      LOG_ERR("something is wrong; trigger a CLEAR request\n");
      sixp_trans_abort(sixp_trans_find(peer_addr));
      clear_send_request(peer_addr);
    } else {
      LOG_ERR("received RC %u from", rc);
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("; action to take is not defined\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
delete_callback_sent(void *arg, uint16_t arg_len, const linkaddr_t *dest_addr,
                     sixp_output_status_t status)
{
  tsch_link_t *cell = (tsch_link_t *)arg;

  assert(dest_addr != NULL);
  /* this callback is used only by the responder */
  assert(NETSTACK_ROUTING.node_is_root() || (dest_addr != parent_addr));

  /* we are the responder of the DELETE transaction */
  if(status == SIXP_OUTPUT_STATUS_SUCCESS &&
     cell != NULL &&
     remove_negotiated_cell(cell) == 0) {
    assert(arg_len == sizeof(tsch_link_t));
    LOG_DBG("DELETE transaction completes successfully\n");
  } else {
    /* if their ACK may be lost, schedule inconsistency happens */
    LOG_ERR("SCHEDULE INCONSISTENCY may happen\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
relocate_send_request(const tsch_link_t *target_cell)
{
  const sixp_pkt_type_t type = SIXP_PKT_TYPE_REQUEST;
  const sixp_pkt_code_t code = (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_RELOCATE;
  const size_t relocation_cell_list_len = sizeof(sixp_pkt_cell_t);
  const size_t candidate_cell_list_len = (sizeof(sixp_pkt_cell_t) *
                                          MSF_6P_CELL_LIST_LEN);
  const size_t body_len = (sizeof(sixp_pkt_metadata_t) +
                           sizeof(sixp_pkt_cell_options_t) +
                           sizeof(sixp_pkt_num_cells_t) +
                           relocation_cell_list_len +
                           candidate_cell_list_len);
  sixp_pkt_cell_t relocation_cell_list[1];
  sixp_pkt_cell_t candidate_cell_list[MSF_6P_CELL_LIST_LEN];
  uint8_t body[body_len];
  sixp_pkt_cell_options_t cell_options = SIXP_PKT_CELL_OPTION_TX;
  const sixp_pkt_num_cells_t num_cells = 1;
  tsch_link_t *reserved_cell_head;

  assert(parent_addr != NULL);
  assert(target_cell != NULL);

  memset(body, 0, body_len);
  set_cell_params(relocation_cell_list,
                  target_cell->timeslot, target_cell->channel_offset);
  reserved_cell_head = prepare_candidate_cell_list(parent_addr,
                                                   candidate_cell_list,
                                                   MSF_6P_CELL_LIST_LEN);

  if(reserved_cell_head == NULL) {
    LOG_ERR("failed to send a RELOCATE request; "
            "cannot make a CandidateCellListt\n");
    set_random_wait_to_next_request();
    return;
  } else if(
    sixp_pkt_set_cell_options(type, code, cell_options, body, body_len) < 0 ||
    sixp_pkt_set_num_cells(type, code, num_cells, body, body_len) < 0 ||
    sixp_pkt_set_rel_cell_list(type, code,
                               (const uint8_t *)relocation_cell_list,
                               relocation_cell_list_len, 0,
                               body, body_len) < 0 ||
    sixp_pkt_set_cand_cell_list(type, code,
                                (const uint8_t *)candidate_cell_list,
                                candidate_cell_list_len, 0,
                                body, body_len) < 0) {
    LOG_ERR("failed to send a RELOCATE request; cannot build a body\n");
    release_reserved_cells(reserved_cell_head);
    set_random_wait_to_next_request();
  } else if(sixp_output(type, code, MSF_SFID, body, body_len, parent_addr,
                        relocate_callback_sent,
                        reserved_cell_head, sizeof(tsch_link_t)) < 0) {
    LOG_ERR("failed to send a RELOCATE request\n");
    release_reserved_cells(reserved_cell_head);
    set_random_wait_to_next_request();
  } else {
    LOG_DBG("sent a RELOCATE request to the parent: ");
    LOG_DBG_LLADDR(parent_addr);
    LOG_DBG_("\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
relocate_recv_request(const linkaddr_t *peer_addr,
                      const uint8_t *body, uint16_t body_len)
{
  const sixp_pkt_type_t type = SIXP_PKT_TYPE_REQUEST;
  const sixp_pkt_code_t code = (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_RELOCATE;
  sixp_pkt_cell_options_t cell_options;
  sixp_pkt_num_cells_t num_cells;
  const sixp_pkt_cell_t *relocation_cell_list;
  const sixp_pkt_cell_t *candidate_cell_list;
  uint16_t relocation_cell_list_len;
  uint16_t candidate_cell_list_len;

  assert(peer_addr != NULL);

  if(
    sixp_pkt_get_cell_options(type, code, &cell_options, body, body_len) < 0 ||
    sixp_pkt_get_num_cells(type, code, &num_cells, body, body_len) < 0 ||
    sixp_pkt_get_rel_cell_list(type, code,
                               (const uint8_t **)&relocation_cell_list,
                               &relocation_cell_list_len,
                               body, body_len) < 0 ||
    sixp_pkt_get_cand_cell_list(type, code,
                                (const uint8_t **)&candidate_cell_list,
                                &candidate_cell_list_len,
                                body, body_len) < 0) {
    LOG_ERR("failed to process a RELOCATE request; parse error\n");
  } else {
    assert(cell_options == SIXP_PKT_CELL_OPTION_TX);
    assert(num_cells == 1);
    assert(relocation_cell_list_len == sizeof(sixp_pkt_cell_t));
    assert(candidate_cell_list_len >=
           (sizeof(sixp_pkt_cell_t) * MSF_6P_CELL_LIST_MIN_LEN));
    LOG_DBG("received a RELOCATE request from ");
    LOG_DBG_LLADDR(peer_addr);
    LOG_DBG_("\n");
    relocate_send_response(peer_addr,
                           relocation_cell_list, relocation_cell_list_len,
                           candidate_cell_list, candidate_cell_list_len);
  }
}
/*---------------------------------------------------------------------------*/
static void
relocate_send_response(const linkaddr_t *peer_addr,
                       const sixp_pkt_cell_t *relocation_cell_list,
                       uint16_t relocation_cell_list_len,
                       const sixp_pkt_cell_t *candidate_cell_list,
                       uint16_t candidate_cell_list_len)
{
  struct tsch_slotframe *slotframe = get_slotframe();
  tsch_link_t *target_cell;
  uint16_t slot_offset;
  uint16_t channel_offset;
  tsch_link_t *reserved_cell;
  const sixp_pkt_cell_t *cell_list_to_return;

  assert(slotframe != NULL);
  assert(peer_addr != NULL);
  assert(relocation_cell_list != NULL);
  assert(candidate_cell_list != NULL);

  get_cell_params(relocation_cell_list, &slot_offset, &channel_offset);
  target_cell = tsch_schedule_get_link_by_timeslot(slotframe, slot_offset);
  if(target_cell == NULL ||
     linkaddr_cmp(&target_cell->addr, peer_addr) == 0 ||
     target_cell->channel_offset != channel_offset ||
     target_cell->link_options != LINK_OPTION_RX ||
     target_cell->link_type != LINK_TYPE_NORMAL) {
    LOG_ERR("received an invalid cell to relocate, "
            "slot_offset: %u, channel_offset: %u\n",
            slot_offset, channel_offset);
    if(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_ERR_CELLLIST,
                   MSF_SFID, NULL, 0, peer_addr,
                   NULL, NULL, 0) < 0) {
      LOG_ERR("failed to send an RC_ERR_CELLLIST response\n");
    } else {
      LOG_DBG("sent an RC_ERR_CELLLIST response\n");
    }
  } else {
    reserved_cell = prepare_cell_list_to_return(peer_addr,
                                                candidate_cell_list,
                                                candidate_cell_list_len,
                                                &cell_list_to_return);
    /* associate target_cell with reserved_cell to memorize it */
    if(reserved_cell != NULL) {
      reserved_cell->data = (void *)target_cell;
    }
    if(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                   (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                   MSF_SFID, (const uint8_t *)cell_list_to_return,
                   cell_list_to_return == NULL ? 0 : sizeof(sixp_pkt_cell_t),
                   peer_addr,
                   relocate_callback_sent,
                   reserved_cell,
                   reserved_cell == NULL ? 0 : sizeof(tsch_link_t)) < 0) {
      LOG_ERR("failed to send an RC_SUCCESS response to ");
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("\n");
    } else {
      LOG_DBG("sent an RC_SUCCESS response");
      if(reserved_cell == NULL) {
        LOG_DBG_(" with an empty CellList to ");
      } else {
        LOG_DBG_(" to ");
      }
      LOG_DBG_LLADDR(peer_addr);
      LOG_DBG_("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
relocate_recv_response(const linkaddr_t *peer_addr, sixp_pkt_rc_t rc,
                       const uint8_t *body, uint16_t body_len)
{
  const sixp_pkt_cell_t *cell_list;
  uint16_t cell_list_len;
  uint16_t timeslot;
  uint16_t channel_offset;
  struct tsch_slotframe *slotframe = get_slotframe();
  tsch_link_t *cell;

  assert(peer_addr != NULL);
  assert(slotframe != NULL);

  if(rc == SIXP_PKT_RC_SUCCESS) {
    LOG_DBG("received an RC_SUCCESS response from ");
    LOG_DBG_LLADDR(peer_addr);
    LOG_DBG_("\n");
    if(sixp_pkt_get_cell_list(SIXP_PKT_TYPE_RESPONSE,
                              (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                              (const uint8_t **)&cell_list, &cell_list_len,
                              body, body_len) < 0) {
      LOG_ERR("failed to process RC_SUCCESS response; parse error\n");
    } else if(cell_list_len == 0) {
      LOG_INFO("received an empty CellList; "
               "try another RELOCATE request later\n");
    } else if(body_len > sizeof(sixp_pkt_cell_t)) {
      /* invalid length; MSF always requests one cell per RELOCATE request */
      LOG_ERR("received an invalid CellList whose length is %u\n", body_len);
    } else {
      assert(cell_list_len == sizeof(sixp_pkt_cell_t));
      get_cell_params(cell_list, &timeslot, &channel_offset);
      cell = tsch_schedule_get_link_by_timeslot(slotframe, timeslot);
      /* this cell should be one we reserved beforehand */
      if(cell == NULL ||
         cell->timeslot != timeslot ||
         cell->channel_offset != channel_offset ||
         linkaddr_cmp(&cell->addr, peer_addr) == 0 ||
         cell->link_options != 0) {
        LOG_ERR("SCHEDULE INCONSISTENCY is likely to happen; ");
        LOG_ERR_("received a cell which is not a reserved one\n");
        release_reserved_cells(find_reserved_cell_head(peer_addr));
      } else {
        /* the peer accepts one of reserved cells; let's perform relocation */
        if(remove_negotiated_cell(cell_to_relocate) < 0 ||
           add_negotiated_cell(MSF_NEGOTIATED_TX_CELL,
                               peer_addr, timeslot, channel_offset) < 0) {
          LOG_ERR("SCHEDULE INCONSISTENCY is likely to happen\n");
          /* better to have CLEAR or DELETE? */
        } else {
          cell_to_relocate = NULL;
          LOG_DBG("RELOCATE transaction completes successfully\n");
        }
      }
    }
  } else {
    release_reserved_cells(find_reserved_cell_head(peer_addr));
    if(rc == SIXP_PKT_RC_ERR) {
      /* the peer doesn't accept our 6P Request as a whole */
      cell_to_relocate = NULL;
      LOG_ERR("received RC_ERR from ");
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("; we may run a different SF\n");
    } else if(rc == SIXP_PKT_RC_ERR_SEQNUM) {
      cell_to_relocate = NULL;
      LOG_ERR("received RC_ERR_SEQNUM from ");
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("; going to send a CLEAR request\n");
      sixp_trans_abort(sixp_trans_find(peer_addr));
      clear_send_request(peer_addr);
    } else if(rc == SIXP_PKT_RC_ERR_BUSY) {
      LOG_ERR("received RC_ERR_BUSY from ");
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("; retry later\n");
      set_random_wait_to_next_request();
    } else if(rc == SIXP_PKT_RC_ERR_CELLLIST) {
      cell_to_relocate = NULL;
      LOG_ERR("received RC_ERR_CELLLIST from ");
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("; we don't retry RELOCATE\n");
    } else {
      cell_to_relocate = NULL;
      LOG_ERR("received RC %u from", rc);
      LOG_ERR_LLADDR(peer_addr);
      LOG_ERR_("; action to take is not defined\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
relocate_callback_sent(void *arg, uint16_t arg_len, const linkaddr_t *dest_addr,
                       sixp_output_status_t status)
{
  tsch_link_t *reserved_cell_head = (tsch_link_t *)arg;

  assert(dest_addr != NULL);

  if(status == SIXP_OUTPUT_STATUS_SUCCESS) {
    if(NETSTACK_ROUTING.node_is_root() ||
       (parent_addr == NULL &&
        linkaddr_cmp(dest_addr, parent_addr) == 0)) {
      /* our response is acknowledged nothing to do */
      if(reserved_cell_head == NULL) {
        /* we've returned an empty CellList; do nothing */
        assert(arg_len == 0);
      } else {
        tsch_link_t *cell_to_delete = (tsch_link_t *)reserved_cell_head->data;
        reserved_cell_head->data = NULL;
        assert(arg_len == sizeof(tsch_link_t));

        if(remove_negotiated_cell(cell_to_delete) < 0 ||
           add_negotiated_cell(MSF_NEGOTIATED_RX_CELL, dest_addr,
                               reserved_cell_head->timeslot,
                               reserved_cell_head->channel_offset) < 0) {
          /* this allocation failure could cause schedule inconsistency */
          LOG_ERR("SCHEDULE INCONSISTENCY is likely to happen\n");
          /* better to have CLEAR or DELETE? */
        } else {
          LOG_DBG("RELOCATE transaction completes successfully\n");
        }
      }
    } else {
      /* our request is acknowledged nothing to do */
    }
  } else {
    assert(status == SIXP_OUTPUT_STATUS_FAILURE ||
           status == SIXP_OUTPUT_STATUS_ABORTED);
    LOG_ERR("RELOCATE transaction failed\n");
    assert(reserved_cell_head != NULL);
    if(reserved_cell_head->data != NULL &&
       ((tsch_link_t *)reserved_cell_head->data)->link_options != 0) {
      /*
       *  the next cell of the reserved_cell_head is not a "reserved"
       *  cell, which must be the cell to delete. we want to keep that
       *  one.
       */
      reserved_cell_head->data = NULL;
    }
    release_reserved_cells(reserved_cell_head);
    if(NETSTACK_ROUTING.node_is_root() ||
       parent_addr == NULL ||
       (linkaddr_cmp(dest_addr, parent_addr) == 0)) {
      /* do nothing */
    } else {
      /* we're the initiator still have the parent; retry later */
      set_random_wait_to_next_request();
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
clear_send_request(const linkaddr_t *peer_addr)
{
  uint8_t body[sizeof(sixp_pkt_metadata_t)];
  memset(body, 0, sizeof(body));

  assert(peer_addr != NULL);
  if(sixp_output(SIXP_PKT_TYPE_REQUEST,
                 (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_CLEAR,
                 MSF_SFID, body, sizeof(body), peer_addr,
                 clear_callback_sent, NULL, 0) < 0) {
    LOG_ERR("failed to send a CLEAR request to ");
    LOG_ERR_LLADDR(peer_addr);
    LOG_ERR_("\n");
    /* remove all the negotiated TX cells without retry */
    msf_negotiated_cell_remove_all(peer_addr);
  } else {
    LOG_DBG("sent a CLEAR request to ");
    LOG_DBG_LLADDR(peer_addr);
    LOG_DBG_("\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
clear_recv_request(const linkaddr_t *peer_addr)
{
  assert(peer_addr != NULL);
  LOG_DBG("received a CLEAR request from ");
  LOG_DBG_LLADDR(peer_addr);
  LOG_DBG_("\n");
  clear_send_response(peer_addr);
  /* in this case, cells to remove are all RX; we can remove them now */
  msf_negotiated_cell_remove_all(peer_addr);
}
/*---------------------------------------------------------------------------*/
static void
clear_send_response(const linkaddr_t *peer_addr)
{
  if(sixp_output(SIXP_PKT_TYPE_RESPONSE,
                 (sixp_pkt_code_t)(uint8_t)SIXP_PKT_RC_SUCCESS,
                 MSF_SFID, NULL, 0, peer_addr,
                 NULL, NULL, 0) < 0) {
    LOG_ERR("failed to send an RC_SUCCESS response to ");
    LOG_ERR_LLADDR(peer_addr);
    LOG_ERR_("\n");
  } else {
    LOG_DBG("sent an  RC_SUCCESS response to ");
    LOG_DBG_LLADDR(peer_addr);
    LOG_DBG_("\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
clear_recv_response(const linkaddr_t *peer_addr)
{
  /* nothing to do */
  LOG_DBG("CLEAR transaction completes successfully\n");
}
/*---------------------------------------------------------------------------*/
static void
clear_callback_sent(void *arg, uint16_t arg_len, const linkaddr_t *dest_addr,
                    sixp_output_status_t status)
{
  /* this callback is expected to be set only for request */
  /*
   * in any case, remove all the negotiated TX cells without waiting
   * for a response
   */
  if(status == SIXP_OUTPUT_STATUS_SUCCESS) {
    LOG_DBG("CLEAR transaction completes successfully\n");
  } else {
    LOG_ERR("CLEAR transaction ends with an error\n");
  }
  msf_negotiated_cell_remove_all(dest_addr);
}
/*---------------------------------------------------------------------------*/
static void
count_send_request(const linkaddr_t *peer_addr)
{
  assert(peer_addr != NULL);
  assert(peer_addr != parent_addr);
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  parent_addr = NULL;
  memb_init(&msf_negotiated_tx_cell_data_memb);
  num_required_upward_cells = 1;
  num_cells_elapse = 0;
  num_cells_used = 0;
  cell_to_relocate = NULL;
  process_start(&msf_negotiated_cell_management_process, NULL);
}
/*---------------------------------------------------------------------------*/
static void
input_handler(sixp_pkt_type_t type, sixp_pkt_code_t code,
              const uint8_t *body, uint16_t body_len,
              const linkaddr_t *src_addr)
{
  sixp_trans_t *trans;
  sixp_pkt_cmd_t cmd;

  if(msf_is_activated == true && get_slotframe() == NULL) {
    if(add_slotframe() == NULL) {
      LOG_ERR("failed to add the slotframe for the negotiated cell\n");
    } else {
      LOG_DBG("added the slotframe for the negotiated cell\n");
    }
  }

  if((trans = sixp_trans_find(src_addr)) == NULL) {
    LOG_ERR("cannot find a 6P transaction of a received 6P packet\n");
    return;
  } else {
    cmd = sixp_trans_get_cmd(trans);
  }

  if(type == SIXP_PKT_TYPE_REQUEST) {
    assert(code.value == cmd);
    if(code.value == SIXP_PKT_CMD_ADD) {
      add_recv_request(src_addr, body, body_len);
    } else if(code.value == SIXP_PKT_CMD_DELETE) {
      delete_recv_request(src_addr, body, body_len);
    } else if(code.value == SIXP_PKT_CMD_RELOCATE) {
      relocate_recv_request(src_addr, body, body_len);
    } else if(code.value == SIXP_PKT_CMD_CLEAR) {
      clear_recv_request(src_addr);
    }
  } else if(type == SIXP_PKT_TYPE_RESPONSE) {
    if(cmd == SIXP_PKT_CMD_ADD) {
      add_recv_response(src_addr, (sixp_pkt_rc_t)code.value, body, body_len);
    } else if(cmd == SIXP_PKT_CMD_DELETE) {
      delete_recv_response(src_addr, (sixp_pkt_rc_t)code.value, body, body_len);
    } else if(cmd == SIXP_PKT_CMD_RELOCATE) {
      relocate_recv_response(src_addr,
                             (sixp_pkt_rc_t)code.value, body, body_len);
    } else if(cmd == SIXP_PKT_CMD_CLEAR) {
      clear_recv_response(src_addr);
    }
  } else {
    /* MSF doesn't use 3-step transactions */
    LOG_ERR("received a 6P Confirmation, which is not supported by MSF\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
timeout_handler(sixp_pkt_cmd_t cmd, const linkaddr_t *peer_addr)
{
  assert(peer_addr != NULL);
  if(cmd == SIXP_PKT_CMD_ADD) {
    LOG_ERR("ADD transaction ends because of timeout\n");
    release_reserved_cells(find_reserved_cell_head(peer_addr));
  }

  if(peer_addr == parent_addr) {
    /* we are the initiator */
  } else {
    /* we are the responder */
    /*
     * scheduling inconsistency may happen because of this timeout of
     * the transaction, where the peer completes the transaction by
     * our L2 MAC, but we don't. Better to confirm if there is
     * scheduling consistency.
     */
    count_send_request(peer_addr);
  }

  /* retry */
}
/*---------------------------------------------------------------------------*/
static void
error_handler(sixp_error_t err, sixp_pkt_cmd_t cmd, uint8_t seqno,
              const linkaddr_t *peer_addr)
{

}
/*---------------------------------------------------------------------------*/
void
msf_negotiated_cell_set_parent(const linkaddr_t *new_parent)
{
  const linkaddr_t *old_parent = parent_addr;
  parent_addr = new_parent;
  assert(msf_is_activated == true);
  assert(old_parent != new_parent);

  LOG_INFO("change our parent to ");
  LOG_INFO_LLADDR(parent_addr);
  LOG_INFO_("\n");

  if(old_parent != NULL && get_slotframe() != NULL) {
    /* CLEAR all the cells scheduled with old_parent */
    sixp_trans_t *trans = sixp_trans_find(old_parent);
    if(trans != NULL) {
      /* abort the ongoing transaction */
      sixp_trans_abort(trans);
    }

    if(get_num_negotiated_tx_cells(old_parent) > 0) {
      clear_send_request(old_parent);
    }
  }

  if(new_parent != NULL) {
    if(get_slotframe() == NULL && add_slotframe() == NULL) {
      LOG_ERR("slotframe for the negotiated cell is not available\n");
    } else {
      /* reset the counters */
      num_cells_elapse = 0;
      num_cells_used = 0;
      cell_to_relocate = NULL;
      /* start allocating negotiated cells with new_parent */
      /* reset the timer so as to send a request immediately */
      timer_set(&request_wait_timer, 0);
      assert(timer_expired(&request_wait_timer) != 0);
      /* keep num_required_upward_cells and use it for the new parent */
      LOG_DBG("we're going to schedule %u negotiated TX cell%s",
              num_required_upward_cells,
              num_required_upward_cells == 1 ? " " : "s ");
      LOG_DBG_("with the new parent\n");
      if(sixp_trans_find(new_parent) == NULL) {
        add_send_request(MSF_NEGOTIATED_TX_CELL);
      } else {
        /* we may have a transaction with the new parent */
        set_random_wait_to_next_request();
      }
    }
  } else {
    /* reset the counter of required negotiated TX cells */
    LOG_DBG("resetting num_required_upward_cells to 1\n");
    num_required_upward_cells = 1;
  }
}
/*---------------------------------------------------------------------------*/
void
msf_negotiated_cell_update_num_cells_used(uint16_t count)
{
  if(parent_addr != NULL &&
     get_num_negotiated_tx_cells(parent_addr) > 0) {
    num_cells_used += count;
  }
}
/*---------------------------------------------------------------------------*/
void
msf_negotiated_cell_update_num_tx(uint16_t slot_offset, uint16_t num_tx,
                                  uint8_t mac_tx_status)
{
  struct tsch_neighbor *nbr;
  if(num_tx == 0) {
    /* nothing to do */
  } else if(parent_addr == NULL ||
     (nbr = tsch_queue_get_nbr(parent_addr)) == NULL ||
     nbr->negotiated_tx_cell == NULL) {
    /* we don't have negotiated TX cells */
  } else if(tsch_get_lock()) {
    tsch_link_t * cell;
    tsch_link_t *last_used = NULL;
    /* identify the cell that is used for the last transmission */
    for(cell = nbr->negotiated_tx_cell;
        cell != NULL;
        cell = ((msf_negotiated_tx_cell_data_t *)cell->data)->next) {
      if(cell->timeslot == slot_offset) {
        last_used = cell;
        break;
      }
    }
    if(last_used == NULL) {
      /* we don't have a negotiated cell on the slot offset; do nothing */
    } else {
      uint16_t i;
      msf_negotiated_tx_cell_data_t *cell_data;
      cell = last_used;
      assert(cell != NULL);
      assert(num_tx > 0);
      /* update the counters */
      for(i = 0; i < num_tx; i++) {
        assert(cell->data != NULL);
        cell_data = (msf_negotiated_tx_cell_data_t *)cell->data;
        if(cell_data->num_tx == 255) {
          cell_data->num_tx = 128;
          cell_data->num_tx_ack /= 2;
        } else {
          cell_data->num_tx++;
        }
        if(((msf_negotiated_tx_cell_data_t *)cell->data)->next == NULL) {
          cell = nbr->negotiated_tx_cell;
        } else {
          cell = ((msf_negotiated_tx_cell_data_t *)cell->data)->next;
        }
      }
      if(mac_tx_status == MAC_TX_OK) {
        ((msf_negotiated_tx_cell_data_t *)last_used->data)->num_tx_ack++;
      }
    }
    tsch_release_lock();
  } else {
    LOG_ERR("failed to update NumTx/NumTxAck because of tsch_lock\n");
  }
}
/*---------------------------------------------------------------------------*/
void
msf_negotiated_cell_remove_all(const linkaddr_t *peer_addr)
{
  struct tsch_slotframe *slotframe = get_slotframe();
  struct tsch_neighbor *nbr;
  tsch_link_t *cell;
  tsch_link_t *next_cell;

  if(slotframe == NULL) {
    LOG_INFO("we don't have the slotframe for the negotiated cell\n");
  } else if (peer_addr != NULL) {
    nbr = tsch_queue_get_nbr(peer_addr);
    if(nbr == NULL) {
      /* nothing to do */
    } else {
      next_cell = nbr->negotiated_tx_cell;
      while((cell = next_cell) != NULL) {
        next_cell = ((msf_negotiated_tx_cell_data_t *)cell->data)->next;
        (void)remove_negotiated_cell(cell);
      }
      nbr->negotiated_tx_cell = NULL;
    }
  } else {
    LOG_INFO("remove all the negotiated cells\n");
    next_cell = list_head(slotframe->links_list);
    while((cell = next_cell) != NULL) {
      next_cell = list_item_next(cell);
      nbr = tsch_queue_get_nbr(&cell->addr);
      if(nbr == NULL) {
        LOG_ERR("cannot find nbr for a scheduled negotiated cell for");
        LOG_ERR_LLADDR(&cell->addr);
        LOG_ERR_("\n");
      } else {
        /*
         * although nbr->negotiated_tx_cell doesn't necessarily have
         * the address of the current "cell", we set it to NULL since
         * we're going to remove all the negotiated cells in this
         * for-loop, anyway.
         */
        nbr->negotiated_tx_cell = NULL;
        (void)remove_negotiated_cell(cell);
      }
    }
    if(tsch_schedule_remove_slotframe(slotframe) == 0) {
      LOG_ERR("failed to remove the slotframe for the negotiated cell\n");
    } else {
      LOG_INFO("removed the slotframe for the negotiated cell\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
const sixtop_sf_t msf = {
  MSF_SFID,
  (((2 << (TSCH_MAC_MAX_BE - 1)) - 1) *
   TSCH_MAC_MAX_FRAME_RETRIES *
   MSF_SLOTFRAME_LENGTH * MSF_SLOT_LENGTH_MS / 1000 * CLOCK_SECOND),
  init,
  input_handler,
  timeout_handler,
  error_handler,
};
/*---------------------------------------------------------------------------*/
