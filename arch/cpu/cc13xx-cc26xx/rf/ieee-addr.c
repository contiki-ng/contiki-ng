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
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "net/linkaddr.h"

#include "rf/ieee-addr.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_memmap.h)
#include DeviceFamily_constructPath(inc/hw_fcfg1.h)
#include DeviceFamily_constructPath(inc/hw_ccfg.h)
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#define IEEE_ADDR_HARDCODED         IEEE_ADDR_CONF_HARDCODED
#define IEEE_ADDR_ADDRESS           IEEE_ADDR_CONF_ADDRESS
/*---------------------------------------------------------------------------*/
#define IEEE_MAC_PRIMARY_ADDRESS    (FCFG1_BASE + FCFG1_O_MAC_15_4_0)
#define IEEE_MAC_SECONDARY_ADDRESS  (CCFG_BASE + CCFG_O_IEEE_MAC_0)
/*---------------------------------------------------------------------------*/
int
ieee_addr_cpy_to(uint8_t *dst, uint8_t len)
{
  if(len > LINKADDR_SIZE) {
    return -1;
  }

  if(IEEE_ADDR_HARDCODED) {
    const uint8_t ieee_addr_hc[LINKADDR_SIZE] = IEEE_ADDR_ADDRESS;

    memcpy(dst, &ieee_addr_hc[LINKADDR_SIZE - len], len);
  } else {
    int i;

    volatile const uint8_t *const primary = (uint8_t *)IEEE_MAC_PRIMARY_ADDRESS;
    volatile const uint8_t *const secondary = (uint8_t *)IEEE_MAC_SECONDARY_ADDRESS;

    /* Reading from primary location... */
    volatile const uint8_t *ieee_addr = primary;

    /*
     * ...unless we can find a byte != 0xFF in secondary.
     *
     * Intentionally checking all 8 bytes here instead of len, because we
     * are checking validity of the entire address respective of the
     * actual number of bytes the caller wants to copy over.
     */
    for(i = 0; i < len; i++) {
      if(secondary[i] != 0xFF) {
        /* A byte in the secondary location is not 0xFF. Use the secondary */
        ieee_addr = secondary;
        break;
      }
    }

    /*
     * We have chosen what address to read the address from. Do so,
     * in inverted byte order.
     */
    for(i = 0; i < len; i++) {
      dst[i] = ieee_addr[len - 1 - i];
    }
  }

#ifdef IEEE_ADDR_NODE_ID
  dst[len - 1] = (IEEE_ADDR_NODE_ID >> 0) & 0xFF;
  dst[len - 2] = (IEEE_ADDR_NODE_ID >> 8) & 0xFF;
#endif

  return 0;
}
/*---------------------------------------------------------------------------*/
/** @} */
