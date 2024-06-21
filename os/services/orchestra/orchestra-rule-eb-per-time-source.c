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
 *         Orchestra: a slotframe dedicated to transmission of EBs.
 *         Nodes transmit at a timeslot defined as hash(MAC) % ORCHESTRA_EBSF_PERIOD
 *         Nodes listen at a timeslot defined as hash(time_source.MAC) % ORCHESTRA_EBSF_PERIOD
 * \author Simon Duquennoy <simonduq@sics.se>
 *         Atis Elsts <atis.elsts@edi.lv>
 */

#include "contiki.h"
#include "orchestra.h"
#include "net/packetbuf.h"

static uint16_t slotframe_handle = 0;
static uint16_t channel_offset = 0;
static struct tsch_slotframe *sf_eb;
static struct tsch_link *timesource_link;

/*---------------------------------------------------------------------------*/
static uint16_t
get_node_timeslot(const linkaddr_t *addr) {
#if ORCHESTRA_EBSF_PERIOD > 0
    return ORCHESTRA_LINKADDR_HASH(addr) % ORCHESTRA_EBSF_PERIOD;
#else
    return 0xffff;
#endif
}

/*---------------------------------------------------------------------------*/
static uint16_t
get_node_channel_offset(const linkaddr_t *addr) {
#if ORCHESTRA_EBSF_PERIOD > 0
    if (ORCHESTRA_EB_MAX_CHANNEL_OFFSET >= ORCHESTRA_EB_MIN_CHANNEL_OFFSET) {
        return ORCHESTRA_LINKADDR_HASH(addr) %
               (ORCHESTRA_EB_MAX_CHANNEL_OFFSET - ORCHESTRA_EB_MIN_CHANNEL_OFFSET + 1)
               + ORCHESTRA_EB_MIN_CHANNEL_OFFSET;
    } else {
        /* ORCHESTRA_EB_MIN_CHANNEL_OFFSET configure larger than max offset, this is an error */
        return 0xffff;
    }
#else
    return 0xffff;
#endif
}

/*---------------------------------------------------------------------------*/
static int
select_packet(uint16_t *slotframe, uint16_t *timeslot, uint16_t *channel_offset) {
    /* Select EBs only */
    if (packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_BEACONFRAME) {
        if (slotframe != NULL) {
            *slotframe = slotframe_handle;
        }
        if (timeslot != NULL) {
            *timeslot = get_node_timeslot(&linkaddr_node_addr);
        }
        /* no need to set the channel offset: it's taken automatically from the link */
        return 1;
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
static void
new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new) {
    uint16_t old_ts = 0xffff;
    uint16_t new_ts = 0xffff;
    uint16_t old_channel_offset = 0xffff;
    uint16_t new_channel_offset = 0xffff;

    if (old != NULL) {
        const linkaddr_t *addr = tsch_queue_get_nbr_address(old);
        old_ts = get_node_timeslot(addr);
        old_channel_offset = get_node_channel_offset(addr);
    }

    if (new != NULL) {
        const linkaddr_t *addr = tsch_queue_get_nbr_address(new);
        new_ts = get_node_timeslot(addr);
        new_channel_offset = get_node_channel_offset(addr);
    }

    if (new_ts == old_ts && old_channel_offset == new_channel_offset) {
        return;
    }

    if (timesource_link != NULL) {
        /* Stop listening to the old time source's EBs */
        tsch_schedule_remove_link(sf_eb, timesource_link);
        timesource_link = NULL;
    }
    if (new_ts != 0xffff) {
        /* Listen to the time source's EBs */
        timesource_link = tsch_schedule_add_link(sf_eb, LINK_OPTION_RX, LINK_TYPE_ADVERTISING_ONLY,
                                                 &tsch_broadcast_address, new_ts, new_channel_offset, 0);
    }
}

/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle) {
    const uint16_t local_ts = get_node_timeslot(&linkaddr_node_addr);
    const uint16_t local_channel_offset = get_node_channel_offset(&linkaddr_node_addr);
    slotframe_handle = sf_handle;
    channel_offset = sf_handle;
    sf_eb = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_EBSF_PERIOD);
    /* EB link: every neighbor uses its own to avoid contention */
    tsch_schedule_add_link(sf_eb,
                           LINK_OPTION_TX,
                           LINK_TYPE_ADVERTISING_ONLY, &tsch_broadcast_address,
                           local_ts, local_channel_offset, 0);
}

/*---------------------------------------------------------------------------*/
struct orchestra_rule eb_per_time_source = {
        init,
        new_time_source,
        select_packet,
        NULL,
        NULL,
        NULL,
        NULL,
        "EB per time source",
        ORCHESTRA_EBSF_PERIOD,
};
