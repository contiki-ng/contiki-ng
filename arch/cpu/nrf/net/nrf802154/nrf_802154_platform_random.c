/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
/*---------------------------------------------------------------------------*/
/**
 * \file
 *      Random number platform for nrf_802154 on the nRF5340 network core.
 *      Backed by the Contiki-NG random module.
 *
 *      The library only calls nrf_802154_random_get() from its CSMA-CA
 *      backoff, which this port disables (NRF_802154_CSMA_CA_ENABLED 0;
 *      Contiki-NG does CSMA backoff on the app core). It is therefore
 *      currently dormant and provided for completeness / future features.
 */
/*---------------------------------------------------------------------------*/
#include "platform/nrf_802154_random.h"
#include "lib/random.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
void
nrf_802154_random_init(void)
{
  /* Contiki-NG random is initialized during boot. */
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_random_deinit(void)
{
}
/*---------------------------------------------------------------------------*/
uint32_t
nrf_802154_random_get(void)
{
  return (uint32_t)random_rand() | ((uint32_t)random_rand() << 16);
}
/*---------------------------------------------------------------------------*/
