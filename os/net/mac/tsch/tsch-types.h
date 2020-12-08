/*
 * Copyright (c) 2015, SICS Swedish ICT.
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
* \addtogroup tsch
* @{
 * \file
 *         TSCH types
 * \author
 *         Simon Duquennoy <simonduq@sics.se>
 */

#ifndef __TSCH_TYPES_H__
#define __TSCH_TYPES_H__

/********** Includes **********/

#include "net/mac/tsch/tsch-asn.h"
#include "lib/list.h"
#include "lib/ringbufindex.h"

/********** Data types **********/

/** \brief 802.15.4e link types. LINK_TYPE_ADVERTISING_ONLY is an extra one: for EB-only links. */
enum link_type { LINK_TYPE_NORMAL, LINK_TYPE_ADVERTISING, LINK_TYPE_ADVERTISING_ONLY };

/** \brief An IEEE 802.15.4-2015 TSCH link (also called cell or slot) */
struct tsch_link {
  /* Links are stored as a list: "next" must be the first field */
  struct tsch_link *next;
  /* Unique identifier */
  uint16_t handle;
  /* MAC address of neighbor */
  linkaddr_t addr;
  /* Slotframe identifier */
  uint16_t slotframe_handle;
  /* Identifier of Slotframe to which this link belongs
   * Unused. */
  /* uint8_t handle; */
  /* Timeslot for this link */
  uint16_t timeslot;
  /* Channel offset for this link */
  uint16_t channel_offset;
  /* A bit string that defines
   * b0 = Transmit, b1 = Receive, b2 = Shared, b3 = Timekeeping, b4 = reserved */
  uint8_t link_options;
  /* Type of link. NORMAL = 0. ADVERTISING = 1, and indicates
     the link may be used to send an Enhanced beacon. */
  enum link_type link_type;
  /* Any other data for upper layers */
  void *data;
};

/** \brief 802.15.4e slotframe (contains links) */
struct tsch_slotframe {
  /* Slotframes are stored as a list: "next" must be the first field */
  struct tsch_slotframe *next;
  /* Unique identifier */
  uint16_t handle;
  /* Number of timeslots in the slotframe.
   * Stored as struct asn_divisor_t because we often need ASN%size */
  struct tsch_asn_divisor_t size;
  /* List of links belonging to this slotframe */
  LIST_STRUCT(links_list);
};

/** \brief TSCH packet information */
struct tsch_packet {
  struct queuebuf *qb;  /* pointer to the queuebuf to be sent */
  mac_callback_t sent; /* callback for this packet */
  void *ptr; /* MAC callback parameter */
  uint8_t transmissions; /* #transmissions performed for this packet */
  uint8_t max_transmissions; /* maximal number of Tx before dropping the packet */
  uint8_t ret; /* status -- MAC return code */
  uint8_t header_len; /* length of header and header IEs (needed for link-layer security) */
  uint8_t tsch_sync_ie_offset; /* Offset within the frame used for quick update of EB ASN and join priority */
};

/** \brief TSCH neighbor information */
struct tsch_neighbor {
  uint8_t is_broadcast; /* is this neighbor a virtual neighbor used for broadcast (of data packets or EBs) */
  uint8_t is_time_source; /* is this neighbor a time source? */
  uint8_t backoff_exponent; /* CSMA backoff exponent */
  uint8_t backoff_window; /* CSMA backoff window (number of slots to skip) */
  uint8_t last_backoff_window; /* Last CSMA backoff window */
  uint8_t tx_links_count; /* How many links do we have to this neighbor? */
  uint8_t dedicated_tx_links_count; /* How many dedicated links do we have to this neighbor? */
  /* Array for the ringbuf. Contains pointers to packets.
   * Its size must be a power of two to allow for atomic put */
  struct tsch_packet *tx_array[TSCH_QUEUE_NUM_PER_NEIGHBOR];
  /* Circular buffer of pointers to packet. */
  struct ringbufindex tx_ringbuf;
};

/** \brief TSCH timeslot timing elements. Used to index timeslot timing
 * of different units, such as rtimer tick or micro-second */
enum tsch_timeslot_timing_elements {
  tsch_ts_cca_offset,
  tsch_ts_cca,
  tsch_ts_tx_offset,
  tsch_ts_rx_offset,
  tsch_ts_rx_ack_delay,
  tsch_ts_tx_ack_delay,
  tsch_ts_rx_wait,
  tsch_ts_ack_wait,
  tsch_ts_rx_tx,
  tsch_ts_max_ack,
  tsch_ts_max_tx,
  tsch_ts_timeslot_length,
  tsch_ts_elements_count, /* Not a timing element */
};

/** \brief TSCH timeslot timing elements in rtimer ticks */
typedef rtimer_clock_t tsch_timeslot_timing_ticks[tsch_ts_elements_count];

/** \brief TSCH timeslot timing elements in micro-seconds */
typedef uint16_t tsch_timeslot_timing_usec[tsch_ts_elements_count];

/** \brief Stores data about an incoming packet */
struct input_packet {
  uint8_t payload[TSCH_PACKET_MAX_LEN]; /* Packet payload */
  struct tsch_asn_t rx_asn; /* ASN when the packet was received */
  int len; /* Packet len */
  int16_t rssi; /* RSSI for this packet */
  uint8_t channel; /* Channel we received the packet on */
};

#endif /* __TSCH_CONF_H__ */
/** @} */
