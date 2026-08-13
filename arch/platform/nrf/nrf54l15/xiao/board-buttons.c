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
 *      Board-specific button initialization for Seeed XIAO nRF54L15
 */

#include "contiki.h"
#include "dev/button-hal.h"
/*---------------------------------------------------------------------------*/
/* The onboard user button pulls P0.0 low while pressed. */
BUTTON_HAL_BUTTON(usr_btn, "User", XIAO_NRF54L15_BUTTON_PORT,
                  XIAO_NRF54L15_BUTTON_PIN, GPIO_HAL_PIN_CFG_PULL_UP,
                  BUTTON_HAL_ID_USER_BUTTON, true);
/*---------------------------------------------------------------------------*/
BUTTON_HAL_BUTTONS(&usr_btn);
/*---------------------------------------------------------------------------*/
