/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Top-level Contiki-NG configuration for the nRF54L15 platform.
 */
#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

#include <stdint.h>
#include <inttypes.h>

/* Project-specific overrides */
#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif /* PROJECT_CONF_PATH */

/* Common nRF definitions */
#include "nrf-def.h"

/* Board specific configuration */
#ifdef BOARD_CONF_PATH
#include BOARD_CONF_PATH
#else
#error "BOARD_CONF_PATH undefined"
#endif /* BOARD_CONF_PATH */

/* Board specific defines */
#ifdef BOARD_DEF_PATH
#include BOARD_DEF_PATH
#else
#error "BOARD_DEF_PATH undefined"
#endif /* BOARD_DEF_PATH */

/* CPU group configuration */
#include "nrf-conf.h"

#ifndef CLOCK_CONF_LFCLK_SRC_IS_RC
#define CLOCK_CONF_LFCLK_SRC_IS_RC 1
#endif

#ifndef CLOCK_CONF_LFCLK_SRC_IS_XTAL
#define CLOCK_CONF_LFCLK_SRC_IS_XTAL 0
#endif

#endif /* CONTIKI_CONF_H */
