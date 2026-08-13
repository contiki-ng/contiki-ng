/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RTIMER_ARCH_H_
#define RTIMER_ARCH_H_

#include "contiki.h"

#define RTIMER_ARCH_SECOND 16000000UL

rtimer_clock_t rtimer_arch_now(void);

#endif /* RTIMER_ARCH_H_ */
