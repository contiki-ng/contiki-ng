/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * \file
 *         Watchdog stub for nRF54L15 (no-op until the real driver lands).
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */
/*---------------------------------------------------------------------------*/
#include "dev/watchdog.h"
/*---------------------------------------------------------------------------*/
void
watchdog_init(void)
{
}
/*---------------------------------------------------------------------------*/
void
watchdog_start(void)
{
}
/*---------------------------------------------------------------------------*/
void
watchdog_periodic(void)
{
}
/*---------------------------------------------------------------------------*/
void
watchdog_stop(void)
{
}
/*---------------------------------------------------------------------------*/
void
watchdog_reboot(void)
{
  while(1) {
  }
}
/*---------------------------------------------------------------------------*/
