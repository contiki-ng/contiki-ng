/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Temperature platform for nrf_802154 on nRF54L15.
 * Returns a fixed 20 C initially; can be improved to read TEMP peripheral.
 */

#include "platform/nrf_802154_temperature.h"
/*---------------------------------------------------------------------------*/
void
nrf_802154_temperature_init(void)
{
  /* Nothing to do. */
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_temperature_deinit(void)
{
  /* Nothing to do. */
}
/*---------------------------------------------------------------------------*/
int8_t
nrf_802154_temperature_get(void)
{
  return 20; /* Fixed 20 degrees Celsius */
}
/*---------------------------------------------------------------------------*/
