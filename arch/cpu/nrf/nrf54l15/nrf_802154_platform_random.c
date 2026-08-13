/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Random number generator platform for nrf_802154 on nRF54L15.
 */

#include "platform/nrf_802154_random.h"
#include "lib/random.h"
/*---------------------------------------------------------------------------*/
void
nrf_802154_random_init(void)
{
  /* Contiki-NG random is already initialized. */
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_random_deinit(void)
{
  /* Nothing to do. */
}
/*---------------------------------------------------------------------------*/
uint32_t
nrf_802154_random_get(void)
{
  return (uint32_t)random_rand() | ((uint32_t)random_rand() << 16);
}
/*---------------------------------------------------------------------------*/
