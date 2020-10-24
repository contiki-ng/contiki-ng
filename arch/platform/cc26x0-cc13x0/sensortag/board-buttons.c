/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup sensortag-cc26xx-peripherals
 * @{
 *
 * \file
 * Defines Sensortag buttons for use with the button HAL
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/gpio-hal.h"
#include "dev/button-hal.h"

#include "ti-lib.h"

#include <stdbool.h>
/*---------------------------------------------------------------------------*/
BUTTON_HAL_BUTTON(reed_relay, "Reed Relay", BOARD_IOID_REED_RELAY, \
                  GPIO_HAL_PIN_CFG_PULL_DOWN, \
                  BOARD_BUTTON_HAL_INDEX_REED_RELAY, false);

BUTTON_HAL_BUTTON(key_left, "Key Left", BOARD_IOID_KEY_LEFT, \
                  GPIO_HAL_PIN_CFG_PULL_UP, BOARD_BUTTON_HAL_INDEX_KEY_LEFT, \
                  true);

BUTTON_HAL_BUTTON(key_right, "Key Right", BOARD_IOID_KEY_RIGHT, \
                  GPIO_HAL_PIN_CFG_PULL_UP, BOARD_BUTTON_HAL_INDEX_KEY_RIGHT, \
                  true);
/*---------------------------------------------------------------------------*/
BUTTON_HAL_BUTTONS(&reed_relay, &key_left, &key_right);
/*---------------------------------------------------------------------------*/
/** @} */
