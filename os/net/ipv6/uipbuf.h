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

/* Get the next header given the buffer - start indicates that this is
   start of the IPv6 header - needs to be set to 0 when in an ext hdr */

uint8_t* uipbuf_get_next_header(uint8_t *buffer, uint16_t size, uint8_t *protocol, uint8_t start);
/* Get the final header given the buffer - that is assumed to be at start
   of an IPv6 header */
uint8_t* uipbuf_get_last_header(uint8_t *buffer, uint16_t size, uint8_t *protocol);

/* Attributes relating to the current packet in uipbuf */
uint16_t uipbuf_get_attr(uint8_t type);
void uipbuf_set_attr_flag(uint16_t flag);
void uipbuf_clr_attr_flag(uint16_t flag);
uint16_t uipbuf_is_attr_flag(uint16_t flag);
int uipbuf_set_attr(uint8_t type, uint16_t value);
void uipbuf_clear_attr(void);

/* These flags will be used for being */
#define UIPBUF_ATTR_FLAGS_6LOWPAN_NO_NHC_COMPRESSION      0x01
#define UIPBUF_ATTR_FLAGS_6LOWPAN_NO_PREFIX_COMPRESSION   0x02

enum {
  UIPBUF_ATTR_LLSEC_LEVEL,
  UIPBUF_ATTR_LLSEC_KEY_ID,
  UIPBUF_ATTR_INTERFACE_ID,
  UIPBUF_ATTR_PHYSICAL_NETWORK_ID,
  UIPBUF_ATTR_MAX_MAC_TRANSMISSIONS,
  UIPBUF_ATTR_FLAGS,
  UIPBUF_ATTR_MAX
};

#endif /* UIPBUF_H_ */
