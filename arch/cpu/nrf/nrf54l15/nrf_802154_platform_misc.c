/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Miscellaneous platform callouts for nrf_802154 on nRF54L15.
 */

#include "nrf_802154.h"

/*
 * Called by the driver at the beginning of each new timeslot.
 * Can be used for additional RADIO register modifications.
 */
/*---------------------------------------------------------------------------*/
void
nrf_802154_custom_part_of_radio_init(void)
{
  /* No custom initialization needed. */
}
/*
 * Called when a TX ACK is started. We don't need to do anything here.
 */
/*---------------------------------------------------------------------------*/
void
nrf_802154_tx_ack_started(const uint8_t *p_data)
{
  (void)p_data;
}
/*---------------------------------------------------------------------------*/
