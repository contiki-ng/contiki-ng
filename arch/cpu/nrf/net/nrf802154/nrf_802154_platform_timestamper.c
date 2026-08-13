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
 *      Frame timestamper for nrf_802154 on the nRF5340 network core.
 *
 *      Frame timestamping is disabled in the project config
 *      (NRF_802154_FRAME_TIMESTAMP_ENABLED 0), so these entry points are
 *      not exercised. They are provided as no-ops so the library links.
 *      A full implementation would subscribe the HP timer (TIMER2)
 *      CAPTURE task to the supplied DPPI channel, mirroring the nRF54L15
 *      port.
 */
/*---------------------------------------------------------------------------*/
#include "platform/nrf_802154_platform_timestamper.h"
#include "nrf.h"

#include <stdbool.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_timestamper_init(void)
{
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_timestamper_cross_domain_connections_setup(void)
{
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_timestamper_cross_domain_connections_clear(void)
{
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_timestamper_local_domain_connections_setup(uint32_t dppi_ch)
{
  (void)dppi_ch;
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_timestamper_local_domain_connections_clear(uint32_t dppi_ch)
{
  (void)dppi_ch;
}
/*---------------------------------------------------------------------------*/
bool
nrf_802154_platform_timestamper_captured_timestamp_read(uint64_t *p_captured)
{
  (void)p_captured;
  return false;
}
/*---------------------------------------------------------------------------*/
