/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Timestamper platform for nrf_802154 on nRF54L15.
 * Sets up DPPI connections for frame timestamping via TIMER10.
 *
 * On nRF54L15 all peripherals are in the same domain, so cross-domain
 * connections are no-ops. Local domain connections subscribe TIMER10
 * CAPTURE to the specified DPPI channel.
 */

#include "platform/nrf_802154_platform_timestamper.h"
#include "nrf.h"

#define HP_TIMER     NRF_TIMER10
#define TIMESTAMP_CC 3
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_timestamper_init(void)
{
  /* Nothing to do at init. */
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_timestamper_cross_domain_connections_setup(void)
{
  /* On nRF54L15 all radio peripherals are in the same domain. No-op. */
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_timestamper_cross_domain_connections_clear(void)
{
  /* No-op. */
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_timestamper_local_domain_connections_setup(uint32_t dppi_ch)
{
  /* Subscribe TIMER10 CAPTURE[3] to the specified DPPI channel. */
  HP_TIMER->SUBSCRIBE_CAPTURE[TIMESTAMP_CC] =
    ((uint32_t)TIMER_SUBSCRIBE_CAPTURE_EN_Enabled << TIMER_SUBSCRIBE_CAPTURE_EN_Pos) |
    (dppi_ch << TIMER_SUBSCRIBE_CAPTURE_CHIDX_Pos);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_timestamper_local_domain_connections_clear(uint32_t dppi_ch)
{
  (void)dppi_ch;
  HP_TIMER->SUBSCRIBE_CAPTURE[TIMESTAMP_CC] = 0;
}
/*---------------------------------------------------------------------------*/
bool
nrf_802154_platform_timestamper_captured_timestamp_read(uint64_t *p_captured)
{
  /* Read the value captured in TIMER10 CC[3] by the DPPI event. */
  *p_captured = (uint64_t)HP_TIMER->CC[TIMESTAMP_CC];
  return true;
}
/*---------------------------------------------------------------------------*/
