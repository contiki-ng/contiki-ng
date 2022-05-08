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
 * \addtogroup gecko
 * @{
 *
 * \addtogroup gecko-sys System drivers
 * @{
 *
 * \addtogroup gecko-linkaddr Link Address driver
 * @{
 *
 * \file
 *         Link address implementation for the gecko.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 */
#include "contiki.h"

#include "linkaddr.h"

#include "linkaddr-arch.h"

#include <string.h>
#include <stdint.h>

/*---------------------------------------------------------------------------*/
#if defined(_SILICON_LABS_32B_SERIES_1)
#define SILABS_DEVINFO_EUI64_LOW   (DEVINFO->UNIQUEL)
#define SILABS_DEVINFO_EUI64_HIGH  (DEVINFO->UNIQUEH)
#elif defined(_SILICON_LABS_32B_SERIES_2)
#include "em_se.h"
#define SILABS_DEVINFO_EUI64_LOW   (DEVINFO->EUI64L)
#define SILABS_DEVINFO_EUI64_HIGH  (DEVINFO->EUI64H)
#else
#error "Undefined SILABS_DEVINFO_EUI64_LOW and SILABS_DEVINFO_EUI64_HIGH"
#endif
/*---------------------------------------------------------------------------*/
void
populate_link_address(void)
{
  uint32_t low = SILABS_DEVINFO_EUI64_LOW;
  uint32_t high = SILABS_DEVINFO_EUI64_HIGH;
  uint8_t i = 0U;

  while(i < 4U && i < LINKADDR_SIZE) {
    linkaddr_node_addr.u8[i] = low & 0xFFU;
    low >>= 8;
    i++;
  }
  while(i < 8U && i < LINKADDR_SIZE) {
    linkaddr_node_addr.u8[i] = high & 0xFFU;
    high >>= 8;
    i++;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
