/*
 * Copyright (c) 2017, RISE SICS, Yanzi Networks
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
 * 3. The name of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 *
 */

#ifndef UIPBUF_H_
#define UIPBUF_H_

#include "contiki.h"

/**
 * \brief          Get the next IPv6 header.
 * \param buffer   A pointer to the buffer holding the IPv6 packet
 * \param size     The size of the data in the buffer
 * \param protocol A pointer to a variable where the protocol of the header will be stored
 * \param start    A flag that indicates if this is expected to be the IPv6 packet header or a later header (Extension header)
 * \retval         returns address of the starting position of the next header
 *
 *                 This function moves to the next header in a IPv6 packet.
 */
uint8_t* uipbuf_get_next_header(uint8_t *buffer, uint16_t size, uint8_t *protocol, uint8_t start);


/**
 * \brief          Get the last IPv6 header.
 * \param buffer   A pointer to the buffer holding the IPv6 packet
 * \param size     The size of the data in the buffer
 * \param protocol A pointer to a variable where the protocol of the header will be stored
 * \retval         returns address of the starting position of the next header
 *
 *                 This function moves to the last header of the IPv6 packet.
 */
uint8_t* uipbuf_get_last_header(uint8_t *buffer, uint16_t size, uint8_t *protocol);


/**
 * \brief          Get the value of the attribute
 * \param type     The attribute to get the value of
 * \retval         the value of the attribute
 *
 *                 This function gets the value of a specific uipbuf attribute.
 */
uint16_t uipbuf_get_attr(uint8_t type);


/**
 * \brief          Set the value of the attribute
 * \param type     The attribute to set the value of
 * \param value    The value to set
 * \retval         0 - indicates failure of setting the value
 * \retval         1 - indicates success of setting the value
 *
 *                 This function sets the value of a specific uipbuf attribute.
 */
int uipbuf_set_attr(uint8_t type, uint16_t value);

/**
 * \brief          Set bits in the uipbuf attribute flags.
 * \param flag_bits The bits to set in the flag.
 *
 *                 This function sets the uipbuf attributes flag of specified bits.
 */
void uipbuf_set_attr_flag(uint16_t flag_bits);

/**
 * \brief          Clear bits in the uipbuf attribute flags.
 * \param flag_bits The bits to clear in the flag.
 *
 *                 This function clears the uipbuf attributes flag of specified bits.
 */
void uipbuf_clr_attr_flag(uint16_t flag_bits);

/**
 * \brief          Check if bits in the uipbuf attribute flag are set.
 * \param flag_bits The bits to check in the flag.
 *
 *                 This function checks if the specified bits are set in the
 *                 uipbuf attributes flag.
 */
uint16_t uipbuf_is_attr_flag(uint16_t flag_bits);


/**
 * \brief          Clear all attributes.
 *
 *                 This function clear all attributes in the uipbuf attributes
 *                 including all flags.
 */
void uipbuf_clear_attr(void);

/**
 * \brief The bits defined for uipbuf attributes flag.
 *
 */
/* Avoid using NHC compression on the packet (6LoWPAN) */
#define UIPBUF_ATTR_FLAGS_6LOWPAN_NO_NHC_COMPRESSION      0x01
/* Avoid using prefix compression on the packet (6LoWPAN) */
#define UIPBUF_ATTR_FLAGS_6LOWPAN_NO_PREFIX_COMPRESSION   0x02

/**
 * \brief The attributes defined for uipbuf attributes function.
 *
 */
enum {
  UIPBUF_ATTR_LLSEC_LEVEL,      /**< Control link layer security level. */
  UIPBUF_ATTR_LLSEC_KEY_ID,     /**< Control link layer security key ID. */
  UIPBUF_ATTR_INTERFACE_ID,     /**< The interface to output packet on */
  UIPBUF_ATTR_PHYSICAL_NETWORK_ID, /**< Physical network ID (mapped to PAN ID)*/
  UIPBUF_ATTR_MAX_MAC_TRANSMISSIONS, /**< MAX transmissions of the packet MAC */
  UIPBUF_ATTR_FLAGS,   /**< Flags that can control lower layers.  see above. */
  UIPBUF_ATTR_MAX
};

#endif /* UIPBUF_H_ */
