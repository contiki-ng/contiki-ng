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
 *         MSF external APIs
 * \author
 *         Yasuyuki Tanaka <yasuyuki.tanaka@inria.fr>
 */

#include <stdbool.h>

#include <contiki.h>
#include <lib/assert.h>
#include <sys/log.h>

#include <net/linkaddr.h>
#include <net/mac/mac.h>
#include <net/mac/tsch/tsch.h>
#include <net/mac/tsch/sixtop/sixtop.h>

#include "msf-autonomous-cell.h"
#include "msf-negotiated-cell.h"

#define LOG_MODULE "MSF"
#define LOG_LEVEL LOG_LEVEL_6TOP

/* variables */
struct tsch_link *msf_autonomous_rx_cell = NULL;
bool msf_is_activated = false;

/* static functions */
const linkaddr_t *get_linkaddr_from_rpl_parent(rpl_parent_t *parent);

/*---------------------------------------------------------------------------*/
const linkaddr_t *
get_linkaddr_from_rpl_parent(rpl_parent_t *parent)
{
  uip_ipaddr_t *ipaddr;
  const linkaddr_t *linkaddr;

  if(parent == NULL) {
    ipaddr = NULL;
  } else {
    ipaddr = rpl_parent_get_ipaddr(parent);
  }

  if(ipaddr == NULL) {
    linkaddr = NULL;
  } else {
    linkaddr = (const linkaddr_t *)uip_ds6_nbr_lladdr_from_ipaddr(ipaddr);
  }

  return linkaddr;
}
/*---------------------------------------------------------------------------*/
void
msf_callback_joining_network(void)
{
  tsch_rpl_callback_joining_network();
  msf_activate();
}
/*---------------------------------------------------------------------------*/
void
msf_callback_leavning_network(void)
{
  tsch_rpl_callback_leaving_network();
  msf_deactivate();
}
/*---------------------------------------------------------------------------*/
void
msf_callback_packet_ready(void)
{
  /*
   * we're going to add an autonomous TX cell to the link-layer
   * destination if necessary.
   */
  const linkaddr_t *dest_mac_addr;
  struct tsch_neighbor *nbr;

  if(msf_is_activated == false) {
    /* MSF is deactivated; nothing to do */
    return;
  }

  dest_mac_addr = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  assert(dest_mac_addr != NULL);

  /*
   * packetbuf is expected to have a packet (frame) which is about to
   * be enqueued to the TX queue.
   */
  if((nbr = tsch_queue_get_nbr(dest_mac_addr)) == NULL &&
     (nbr = tsch_queue_add_nbr(dest_mac_addr)) == NULL) {
    LOG_ERR("cannot perform any operation on the autonomous cell for ");
    LOG_ERR_LLADDR(dest_mac_addr);
    LOG_ERR_("because of lack of memory\n");
    return;
  } else if(nbr == n_broadcast ||
     linkaddr_cmp(dest_mac_addr, &linkaddr_null) == 1) {
    /* we care only about unicast; do nothing. */
  } else if(nbr->autonomous_tx_cell != NULL ||
            nbr->negotiated_tx_cell != NULL) {
    /* we've already had at least one TX cell; do nothing. */
  } else {
    /*
     * need to allocate an autonomous TX cell to the destination. If
     * we fail to add an autonomous cell, nbr->autonomoustx_cell will
     * have NULL.
     */
    nbr->autonomous_tx_cell = msf_autonomous_cell_add(MSF_AUTONOMOUS_TX_CELL,
                                                      dest_mac_addr);
  }
}
/*---------------------------------------------------------------------------*/
void
msf_callback_packet_sent(uint8_t mac_tx_status, int num_tx,
                         const linkaddr_t *dest_mac_addr)
{
  /*
   * what we're going to do is basically either:
   *
   * - if MSF is not activated, do nothing
   * - elif we have negotiated cells to the neighbor, update the counters
   * - elif we don't have any packet in TX queue, delete the autonomous cell
   * - else keep the autonomous cell for a next unicast frame to the neighbor
   *
   */
  struct tsch_neighbor *nbr;

  assert(dest_mac_addr != NULL);
  nbr = tsch_queue_get_nbr(dest_mac_addr);

  if(msf_is_activated == false || nbr == NULL || nbr == n_broadcast ||
     linkaddr_cmp(dest_mac_addr, &linkaddr_null) == 1) {
    /* do nothing */
  } else {
    if(tsch_queue_is_empty(nbr) && nbr->autonomous_tx_cell != NULL) {
      /* we don't need to keep the autonomous TX cell */
      assert(nbr->autonomous_tx_cell != NULL);
      msf_autonomous_cell_delete(nbr->autonomous_tx_cell);
      nbr->autonomous_tx_cell = NULL;
    }

    if(nbr->negotiated_tx_cell != NULL) {
      /* update the counters for the negotiated TX cells */
      msf_negotiated_cell_update_num_cells_used(num_tx);
      msf_negotiated_cell_update_num_tx(
        packetbuf_attr(PACKETBUF_ATTR_TSCH_TIMESLOT), num_tx, mac_tx_status);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
msf_callback_parent_switch(rpl_parent_t *old, rpl_parent_t *new)
{
  tsch_rpl_callback_parent_switch(old, new);
  msf_negotiated_cell_set_parent(get_linkaddr_from_rpl_parent(new));
}
/*---------------------------------------------------------------------------*/
int
msf_is_negotiated_tx_scheduled(void)
{
  struct tsch_neighbor *nbr;
  const uip_ipaddr_t *defrt;
  const linkaddr_t *parent_addr;

  if(msf_is_activated == true) {
    defrt = uip_ds6_defrt_choose();
    if(defrt == NULL) {
      parent_addr = NULL;
    } else {
      parent_addr = (const linkaddr_t *)uip_ds6_nbr_lladdr_from_ipaddr(defrt);
    }
    if(parent_addr == NULL) {
      nbr = NULL;
    } else {
      nbr = tsch_queue_get_nbr(parent_addr);
    }
  } else {
    nbr = NULL;
  }

  return nbr != NULL && nbr->negotiated_tx_cell != NULL;
}
/*---------------------------------------------------------------------------*/
void
msf_activate(void)
{
  assert(msf_is_activated == false);

  /* install our autonomous RX cell */
  msf_autonomous_rx_cell = msf_autonomous_cell_add(MSF_AUTONOMOUS_RX_CELL,
                                                   &linkaddr_node_addr);
  if(msf_autonomous_rx_cell == NULL) {
    LOG_ERR("cannot add the autonomous RX cell; failed to activate MSF\n");
  } else {
    msf_is_activated = true;
    LOG_INFO("MSF is activated\n");
  }
}
/*---------------------------------------------------------------------------*/
void
msf_deactivate(void)
{
  if(msf_is_activated == false) {

  } else {
    /* abort all on-going 6P transactions */

    /* remove the autonomous RX cell */
    msf_autonomous_cell_delete(msf_autonomous_rx_cell);
    msf_autonomous_rx_cell = NULL;

    /* remove all the negotiated/reserved cells */
    msf_negotiated_cell_remove_all(NULL);

    msf_is_activated = false;
    LOG_INFO("MSF is deactivated\n");
  }
}
/*---------------------------------------------------------------------------*/
