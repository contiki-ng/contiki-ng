/*
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-sys System drivers
 * @{
 *
 * \addtogroup nrf-linkaddr Link Address driver
 * @{
 *
 * \file
 *         Link address implementation for the nRF.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 */
#include "contiki.h"

#include "linkaddr.h"

#include "nrf.h"
#include "nrf_ficr.h"

#include "linkaddr-arch.h"

#include <string.h>
#include <stdint.h>

/*---------------------------------------------------------------------------*/
/* Nordic semi OUI */
#define NORDIC_SEMI_VENDOR_OUI 0xF4CE36
/*---------------------------------------------------------------------------*/
void
populate_link_address(void)
{
  uint8_t device_address[8];
  uint32_t device_address_low;

  /*
   * Populate the link address' 3 MSBs using Nordic's OUI.
   * For the remaining 5 bytes just use any 40 of the 48 FICR->DEVICEADDR
   * Those are random, so endianness is irrelevant.
   */
  device_address[0] = (NORDIC_SEMI_VENDOR_OUI) >> 16 & 0xFF;
  device_address[1] = (NORDIC_SEMI_VENDOR_OUI) >> 8 & 0xFF;
  device_address[2] = NORDIC_SEMI_VENDOR_OUI & 0xFF;
#if defined(NRF_FICR)
  device_address[3] = nrf_ficr_deviceid_get(NRF_FICR, 1) & 0xFF;
  device_address_low = nrf_ficr_deviceid_get(NRF_FICR, 0);
#elif defined(NRF_FICR_S)
  /* Secure TrustZone builds are handled by the previous branch, this
   * branch puts in a dummy value for the non-secure TrustZone builds
   * since NRF_FICR is not available. */
  device_address[3] = 0;
  device_address_low = 0;
#endif

  memcpy(&device_address[4], &device_address_low, sizeof(device_address_low));

  memcpy(&linkaddr_node_addr, &device_address[8 - LINKADDR_SIZE],
         LINKADDR_SIZE);
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
