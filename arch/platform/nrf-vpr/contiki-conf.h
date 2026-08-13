/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONTIKI_CONF_H_
#define CONTIKI_CONF_H_

#include <stdint.h>

#define CLOCK_CONF_SIZE        4
#define CLOCK_CONF_SECOND      1000

#define RTIMER_CONF_CLOCK_SIZE 4

#define LINKADDR_CONF_SIZE     2

#define PLATFORM_CONF_PROVIDES_MAIN_LOOP 1

typedef unsigned int uip_stats_t;

#define LOG_CONF_LEVEL_MAIN    0
#define LOG_CONF_LEVEL_NONE    0

#endif /* CONTIKI_CONF_H_ */
