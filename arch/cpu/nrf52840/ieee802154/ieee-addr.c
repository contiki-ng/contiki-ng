/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
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
 * \addtogroup cc13xx-cc26xx-rf-ieee-addr
 * @{
 *
 * \file
 *        Implementation of the CC13xx/CC26xx IEEE addresses driver.
 * \author
 *        Carlo Vallati XXX
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "net/linkaddr.h"

#include "ieee-addr.h"

#include <stdint.h>
#include <string.h>

/*---------------------------------------------------------------------------*/

#undef IEEE_ADDR_CONF_HARDCODED
#define IEEE_ADDR_CONF_HARDCODED 1

#ifndef IEEE_ADDR_CONF_ADDRESS
#define IEEE_ADDR_CONF_ADDRESS {0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#endif
#define IEEE_ADDR_ADDRESS           IEEE_ADDR_CONF_ADDRESS

int
pan_id_le(uint8_t* dst, uint16_t pan){
	dst[0] = pan & 0xFF;
	dst[1] = pan >> 8;

	return 0;
}

int
ieee_addr_cpy_to(uint8_t *dst, uint8_t len)
{
  if(len > LINKADDR_SIZE) {
    return -1;
  }

    uint8_t ieee_addr_hc[LINKADDR_SIZE] = IEEE_ADDR_ADDRESS;

    // Add node id

#ifdef IEEE_ADDR_NODE_ID
    ieee_addr_hc[len - 1] = (IEEE_ADDR_NODE_ID >> 0) & 0xFF;
    ieee_addr_hc[len - 2] = (IEEE_ADDR_NODE_ID >> 8) & 0xFF;
#endif

    int i;

    /*
     * Invert byte order.
     */
    for(i = 0; i < len; i++) {
      dst[i] = ieee_addr_hc[len - 1 - i];

  }

  return 0;
}

int
ieee_addr_init(uint8_t *dst, uint8_t len)
{
  if(len > LINKADDR_SIZE) {
    return -1;
  }

    const uint8_t ieee_addr_hc[LINKADDR_SIZE] = IEEE_ADDR_ADDRESS;

    memcpy(dst, ieee_addr_hc,len);

#ifdef IEEE_ADDR_NODE_ID
  dst[len - 1] = (IEEE_ADDR_NODE_ID >> 0) & 0xFF;
  dst[len - 2] = (IEEE_ADDR_NODE_ID >> 8) & 0xFF;
#endif

  return 0;
}

/*---------------------------------------------------------------------------*/
/** @} */
