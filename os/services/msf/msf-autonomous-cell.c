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
 *         MSF Autonomous Cell APIs
 * \author
 *         Yasuyuki Tanaka <yasuyuki.tanaka@inria.fr>
 */

#include <stddef.h>

#include <contiki.h>
#include <lib/assert.h>
#include <sys/log.h>

#include "msf.h"
#include "msf-autonomous-cell.h"
#include "sax.h"

#define LOG_MODULE "MSF"
#define LOG_LEVEL LOG_LEVEL_6TOP

/* variables */
extern struct tsch_asn_divisor_t tsch_hopping_sequence_length;

/* static functions */
static struct tsch_slotframe *get_slotframe(void);
static struct tsch_slotframe *add_slotframe(void);

/*---------------------------------------------------------------------------*/
static struct tsch_slotframe *
get_slotframe(void)
{
  const uint16_t slotframe_handle = MSF_SLOTFRAME_HANDLE_AUTONOMOUS_CELLS;
  return tsch_schedule_get_slotframe_by_handle(slotframe_handle);
}
/*---------------------------------------------------------------------------*/
static struct tsch_slotframe *
add_slotframe(void)
{
  return tsch_schedule_add_slotframe(MSF_SLOTFRAME_HANDLE_AUTONOMOUS_CELLS,
                                     MSF_SLOTFRAME_LENGTH);
}
/*---------------------------------------------------------------------------*/
struct tsch_link *
msf_autonomous_cell_add(msf_autonomous_cell_type_t type,
                        const linkaddr_t *mac_addr)
{
  struct tsch_slotframe *slotframe = get_slotframe();
  const char *type_str;
  uint8_t link_options;
  uint16_t timeslot;
  uint16_t channel_offset;
  uint16_t num_channels;
  struct tsch_link *cell;

  assert(mac_addr != NULL);

  if(type == MSF_AUTONOMOUS_TX_CELL) {
    assert(slotframe != NULL);
    type_str = "TX";
    link_options = LINK_OPTION_TX | LINK_OPTION_SHARED;
  } else {
    assert(type == MSF_AUTONOMOUS_RX_CELL);
    assert(slotframe == NULL);
    type_str = "RX";
    link_options = LINK_OPTION_RX;
    slotframe = add_slotframe();
  }

  if(slotframe == NULL) {
    LOG_ERR("cannot add an autonomous %s cell for", type_str);
    LOG_ERR_LLADDR(mac_addr);
    LOG_ERR(" because slotframe is not available\n");
    cell = NULL;
  } else {
    /*
     * o  slotOffset(MAC)    = 1 + hash(EUI64, length(Slotframe_1) - 1)
     * o  channelOffset(MAC) = hash(EUI64, NUM_CH_OFFSET)
     */
    num_channels = tsch_hopping_sequence_length.val;

    timeslot = 1 + sax(slotframe->size.val - 1,
                       mac_addr->u8, sizeof(linkaddr_t),
                       MSF_SAX_H0, MSF_SAX_L_BIT, MSF_SAX_R_BIT);
    channel_offset = sax(num_channels, mac_addr->u8, sizeof(linkaddr_t),
                         MSF_SAX_H0, MSF_SAX_L_BIT, MSF_SAX_R_BIT);

    if((cell = tsch_schedule_add_link(slotframe,
                                      link_options, LINK_TYPE_NORMAL, mac_addr,
                                      timeslot, channel_offset)) == NULL) {
      LOG_ERR("failed to add the autonomous %s cell for ", type_str);
      LOG_ERR_LLADDR(mac_addr);
      LOG_ERR_("\n");
    } else {
      LOG_DBG("added an autonomous %s cell for ", type_str);
      LOG_DBG_LLADDR(mac_addr);
      LOG_DBG_(" at slot_offset:%u, channel_offset:%u\n",
               timeslot, channel_offset);
    }
  }

  return cell;
}
/*---------------------------------------------------------------------------*/
void
msf_autonomous_cell_delete(struct tsch_link *autonomous_cell)
{
  struct tsch_slotframe *slotframe = get_slotframe();

  assert(slotframe != NULL);
  assert(autonomous_cell != NULL);

  if(autonomous_cell->link_options & LINK_OPTION_TX) {
    assert(autonomous_cell->link_options & LINK_OPTION_SHARED);
    if(tsch_schedule_remove_link(slotframe, autonomous_cell) == 0) {
      LOG_ERR("failed to remove the autonomous TX cell for ");
      LOG_ERR_LLADDR(&autonomous_cell->addr);
      LOG_ERR_("\n");
    } else {
      LOG_DBG("removed an autonomous TX cell for ");
      LOG_DBG_LLADDR(&autonomous_cell->addr);
      LOG_DBG_("\n");
    }
  } else {
    assert(autonomous_cell->link_options & LINK_OPTION_RX);
    if(tsch_schedule_remove_slotframe(slotframe) == 0) {
      LOG_ERR("failed to remove the autonomous RX cell and the slotframe\n");
    } else {
      LOG_DBG("removed the slotframe for the autonomous cell instead of "
              "removing the autonomous RX cell alone\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
