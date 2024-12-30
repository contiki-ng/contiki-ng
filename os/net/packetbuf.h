/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 *         Header file for the Packet buffer (packetbuf) management
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

/**
 * \addtogroup net
 * @{
 */

/**
 * \defgroup packetbuf Packet buffer
 * @{
 *
 * The packetbuf module does Contiki's buffer management.
 */

#ifndef PACKETBUF_H_
#define PACKETBUF_H_

#include "contiki.h"
#include "net/linkaddr.h"
#include "net/mac/llsec802154.h"
#include "net/mac/csma/csma-security.h"
#include "net/mac/tsch/tsch-conf.h"

/**
 * \brief      The size of the packetbuf, in bytes
 */
#ifdef PACKETBUF_CONF_SIZE
#define PACKETBUF_SIZE PACKETBUF_CONF_SIZE
#else
#define PACKETBUF_SIZE 128
#endif

/**
 * \brief      Clear and reset the packetbuf
 *
 *             This function clears the packetbuf and resets all
 *             internal state pointers (header size, header pointer,
 *             external data pointer). It is used before preparing a
 *             packet in the packetbuf.
 *
 */
void packetbuf_clear(void);

/**
 * \brief      Get a pointer to the data in the packetbuf
 * \return     Pointer to the packetbuf data
 *
 *             This function is used to get a pointer to the data in
 *             the packetbuf. The data is either stored in the packetbuf,
 *             or referenced to an external location.
 *
 */
void *packetbuf_dataptr(void);

/**
 * \brief      Get a pointer to the header in the packetbuf, for outbound packets
 * \return     Pointer to the packetbuf header
 *
 */
void *packetbuf_hdrptr(void);

/**
 * \brief      Get the length of the header in the packetbuf
 * \return     Length of the header in the packetbuf
 *
 */
uint8_t packetbuf_hdrlen(void);


/**
 * \brief      Get the length of the data in the packetbuf
 * \return     Length of the data in the packetbuf
 *
 */
uint16_t packetbuf_datalen(void);

/**
 * \brief      Get the total length of the header and data in the packetbuf
 * \return     Length of data and header in the packetbuf
 *
 */
uint16_t packetbuf_totlen(void);

/**
 * \brief      Get the total length of the remaining space in the packetbuf
 * \return     Length of the remaining space in the packetbuf
 *
 */
uint16_t packetbuf_remaininglen(void);

/**
 * \brief      Set the length of the data in the packetbuf
 * \param len  The length of the data
 */
void packetbuf_set_datalen(uint16_t len);

/**
 * \brief      Copy from external data into the packetbuf
 * \param from A pointer to the data from which to copy
 * \param len  The size of the data to copy
 * \retval     The number of bytes that was copied into the packetbuf
 *
 *             This function copies data from a pointer into the
 *             packetbuf. If the data that is to be copied is larger
 *             than the packetbuf, only the data that fits in the
 *             packetbuf is copied. The number of bytes that could be
 *             copied into the rimbuf is returned.
 *
 */
int packetbuf_copyfrom(const void *from, uint16_t len);

/**
 * \brief      Copy the entire packetbuf to an external buffer
 * \param to   A pointer to the buffer to which the data is to be copied
 * \retval     The number of bytes that was copied to the external buffer
 *
 *             This function copies the packetbuf to an external
 *             buffer. Both the data portion and the header portion of
 *             the packetbuf is copied.
 *
 *             The external buffer to which the packetbuf is to be
 *             copied must be able to accomodate at least
 *             PACKETBUF_SIZE bytes. The number of
 *             bytes that was copied to the external buffer is
 *             returned.
 *
 */
int packetbuf_copyto(void *to);

/**
 * \brief      Extend the header of the packetbuf, for outbound packets
 * \param size The number of bytes the header should be extended
 * \retval     Non-zero if the header could be extended, zero otherwise
 *
 *             This function is used to allocate extra space in the
 *             header portion in the packetbuf, when preparing outbound
 *             packets for transmission. If the function is unable to
 *             allocate sufficient header space, the function returns
 *             zero and does not allocate anything.
 *
 */
int packetbuf_hdralloc(int size);

/**
 * \brief      Reduce the header in the packetbuf, for incoming packets
 * \param size The number of bytes the header should be reduced
 * \retval     Non-zero if the header could be reduced, zero otherwise
 *
 *             This function is used to remove the first part of the
 *             header in the packetbuf, when processing incoming
 *             packets. If the function is unable to remove the
 *             requested amount of header space, the function returns
 *             zero and does not allocate anything.
 *
 */
int packetbuf_hdrreduce(int size);

/* Packet attributes stuff below: */

typedef uint16_t packetbuf_attr_t;

struct packetbuf_attr {
  packetbuf_attr_t val;
};
struct packetbuf_addr {
  linkaddr_t addr;
};

enum {
  PACKETBUF_ATTR_NONE,

  /* Scope 0 attributes: used only on the local node. */
  PACKETBUF_ATTR_CHANNEL,
  PACKETBUF_ATTR_NETWORK_ID,
  PACKETBUF_ATTR_LINK_QUALITY,
  PACKETBUF_ATTR_RSSI,
  PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS,
  PACKETBUF_ATTR_MAC_SEQNO,
  PACKETBUF_ATTR_MAC_ACK,
  PACKETBUF_ATTR_MAC_METADATA,
  PACKETBUF_ATTR_MAC_NO_SRC_ADDR,
  PACKETBUF_ATTR_MAC_NO_DEST_ADDR,
#if TSCH_WITH_LINK_SELECTOR
  PACKETBUF_ATTR_TSCH_SLOTFRAME,
  PACKETBUF_ATTR_TSCH_TIMESLOT,
  PACKETBUF_ATTR_TSCH_CHANNEL_OFFSET,
#endif /* TSCH_WITH_LINK_SELECTOR */

  /* Scope 1 attributes: used between two neighbors only. */
  PACKETBUF_ATTR_FRAME_TYPE,
#if LLSEC802154_USES_AUX_HEADER
  PACKETBUF_ATTR_SECURITY_LEVEL,
#endif /* LLSEC802154_USES_AUX_HEADER */
#if LLSEC802154_USES_EXPLICIT_KEYS
  PACKETBUF_ATTR_KEY_ID_MODE,
  PACKETBUF_ATTR_KEY_INDEX,
#endif /* LLSEC802154_USES_EXPLICIT_KEYS */

#if LLSEC802154_USES_FRAME_COUNTER
  PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1,
  PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3,
#endif /* LLSEC802154_USES_FRAME_COUNTER */

  /* Scope 2 attributes: used between end-to-end nodes. */
  /* These must be last */
  PACKETBUF_ADDR_SENDER,
  PACKETBUF_ADDR_RECEIVER,

  PACKETBUF_ATTR_MAX
};

#define PACKETBUF_NUM_ADDRS 2
#define PACKETBUF_NUM_ATTRS (PACKETBUF_ATTR_MAX - PACKETBUF_NUM_ADDRS)
#define PACKETBUF_ADDR_FIRST PACKETBUF_ADDR_SENDER

#define PACKETBUF_IS_ADDR(type) ((type) >= PACKETBUF_ADDR_FIRST)

void              packetbuf_set_attr(uint8_t type, const packetbuf_attr_t val);
packetbuf_attr_t packetbuf_attr(uint8_t type);
void              packetbuf_set_addr(uint8_t type, const linkaddr_t *addr);
const linkaddr_t *packetbuf_addr(uint8_t type);

/**
 * \brief       Checks whether the current packet is a broadcast.
 * \retval true iff the current packet is a broadcast
 */
bool              packetbuf_holds_broadcast(void);

void              packetbuf_attr_clear(void);

void              packetbuf_attr_copyto(struct packetbuf_attr *attrs,
                                        struct packetbuf_addr *addrs);
void              packetbuf_attr_copyfrom(struct packetbuf_attr *attrs,
                                          struct packetbuf_addr *addrs);

#define PACKETBUF_ATTR_SECURITY_LEVEL_DEFAULT 0xffff

#endif /* PACKETBUF_H_ */
/** @} */
/** @} */
