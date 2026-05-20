/*
 * Copyright (c) 2026, Contiki-NG.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "nrf54l15-radio-debug.h"

#include <string.h>

nrf54l15_radio_debug_t nrf54l15_radio_debug;

void
nrf54l15_radio_debug_reset(void)
{
  memset((void *)&nrf54l15_radio_debug, 0, sizeof(nrf54l15_radio_debug));
}
