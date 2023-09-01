/*
 * Copyright (c) 2018, RISE SICS.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Benchmark: the root sends requests to all nodes in a randomized
 *         order, and receives resopnses back.
 * \author
 *         Simon Duquennoy <simon.duquennoy@ri.se>
 */

#include "contiki.h"
#include "contiki-net.h"
#include "sys/node-id.h"
#include "services/deployment/deployment.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "services/deployment/deployment.h"

/** \brief A mapping table for a 8-node Cooja mote simulation.
  * Define your own for any given deployment environment */
const struct id_mac deployment_cooja8[] = {
  {  1, {{0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01}}},
  {  2, {{0x00,0x02,0x00,0x02,0x00,0x02,0x00,0x02}}},
  {  3, {{0x00,0x03,0x00,0x03,0x00,0x03,0x00,0x03}}},
  {  4, {{0x00,0x04,0x00,0x04,0x00,0x04,0x00,0x04}}},
  {  5, {{0x00,0x05,0x00,0x05,0x00,0x05,0x00,0x05}}},
  {  6, {{0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06}}},
  {  7, {{0x00,0x07,0x00,0x07,0x00,0x07,0x00,0x07}}},
  {  8, {{0x00,0x08,0x00,0x08,0x00,0x08,0x00,0x08}}},
  {  0, {{0}}}
};

/** \brief An example mapping for Openmotes in Flocklab.
  * To use, set DEPLOYMENT_MAPPING to deployment_flocklab_openmotes */
const struct id_mac deployment_flocklab_openmotes[] = {
  {  3, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0x9b,0x29}}},
  {  6, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0x9b,0x34}}},
  {  8, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0x9b,0x1f}}},
  { 15, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0x9b,0x85}}},
  { 16, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0x9b,0x00}}},
  { 18, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0x9b,0x37}}},
  { 22, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0x9b,0x08}}},
  { 23, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0x9b,0x5f}}},
  { 31, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0x9b,0xb1}}},
  {  0, {{0}}}
};

/*---------------------------------------------------------------------------*/
PROCESS(app_process, "App process");
AUTOSTART_PROCESSES(&app_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_process, ev, data)
{
  static struct etimer timer;
  static uip_ipaddr_t ipaddr;
  static linkaddr_t lladdr;
  static int i;

  PROCESS_BEGIN();

  if(node_id == ROOT_ID) {
    /* We are the root, start a DAG */
    NETSTACK_ROUTING.root_start();
    /* Setup a periodic timer that expires after 10 seconds. */
    etimer_set(&timer, CLOCK_SECOND * 10);
    /* Wait until all nodes have joined */
    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
      etimer_reset(&timer);
      /* Log expected IPv6 addresses of all nodes */
      LOG_INFO("Node list:\n");
      for(i = 0; i<deployment_node_count(); i++) {
        int id;
        id = deployment_id_from_index(i);
        /* Set ipaddr with DODAG ID, so we get the prefix */
        NETSTACK_ROUTING.get_root_ipaddr(&ipaddr);
        /* Set IID */
        deployment_iid_from_id(&ipaddr, id);
        /* Get lladdr */
        deployment_lladdr_from_id(&lladdr, id);
        LOG_INFO("-- ID: %02u, Link-layer address: ", id);
        LOG_INFO_LLADDR(&lladdr);
        LOG_INFO_(", IPv6 address: ");
        LOG_INFO_6ADDR(&ipaddr);
        LOG_INFO_("\n");
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
