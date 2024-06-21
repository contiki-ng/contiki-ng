/*
 * Copyright (c) 2015, Swedish Institute of Computer Science.
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
 *         Orchestra header file
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#ifndef ORCHESTRA_H_
#define ORCHESTRA_H_

#include "net/mac/tsch/tsch.h"
#include "orchestra-conf.h"

/* The structure of an Orchestra rule */
struct orchestra_rule {
    void (*init)(uint16_t slotframe_handle);

    void (*new_time_source)(const struct tsch_neighbor *old, const struct tsch_neighbor *

    new);

    int (*select_packet)(uint16_t *slotframe, uint16_t *timeslot, uint16_t *channel_offset);

    void (*child_added)(const linkaddr_t *addr);

    void (*child_removed)(const linkaddr_t *addr);

    void (*neighbor_updated)(const linkaddr_t *addr, uint8_t is_added);

    void (*root_node_updated)(const linkaddr_t *addr, uint8_t is_added);

    const char *const name;
    const int16_t slotframe_size;
};

extern struct orchestra_rule eb_per_time_source;
extern struct orchestra_rule unicast_per_neighbor_rpl_storing;
extern struct orchestra_rule unicast_per_neighbor_rpl_ns;
extern struct orchestra_rule unicast_per_neighbor_link_based;
extern struct orchestra_rule special_for_root;
extern struct orchestra_rule default_common;

extern linkaddr_t orchestra_parent_linkaddr;
extern int orchestra_parent_knows_us;

/* Call from application to start Orchestra */
void orchestra_init(void);
/* Callbacks requied for Orchestra to operate */
/* Set with #define TSCH_CALLBACK_PACKET_READY orchestra_callback_packet_ready */
int orchestra_callback_packet_ready(void);

/* Set with #define TSCH_CALLBACK_NEW_TIME_SOURCE orchestra_callback_new_time_source */
void orchestra_callback_new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *

new);

/* Set with #define NETSTACK_CONF_ROUTING_NEIGHBOR_ADDED_CALLBACK orchestra_callback_child_added */
void orchestra_callback_child_added(const linkaddr_t *addr);

/* Set with #define NETSTACK_CONF_ROUTING_NEIGHBOR_REMOVED_CALLBACK orchestra_callback_child_removed */
void orchestra_callback_child_removed(const linkaddr_t *addr);

/* Returns nonzero if the root slotframe should be used to transmit to the specific address */
uint8_t orchestra_is_root_schedule_active(const linkaddr_t *addr);

/* Set with #define TSCH_CALLBACK_ROOT_NODE_UPDATED orchestra_callback_root_node_updated */
void orchestra_callback_root_node_updated(const linkaddr_t *root, uint8_t is_added);

/* Set with #define NETSTACK_CONF_DS6_NEIGHBOR_UPDATED_CALLBACK orchestra_callback_neighbor_updated */
void orchestra_callback_neighbor_updated(const linkaddr_t *, uint8_t is_added);

#endif /* ORCHESTRA_H_ */
