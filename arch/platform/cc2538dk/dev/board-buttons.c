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
 * \addtogroup cc2538-smartrf
 * @{
 *
 * \defgroup cc2538-smartrf-buttons SmartRF06EB Buttons
 *
 * Generic module controlling buttons on the SmartRF06EB
 * @{
 *
 * \file
 * Defines SmartRF06EB buttons for use with the button HAL
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/button-hal.h"
/*---------------------------------------------------------------------------*/
BUTTON_HAL_BUTTON(key_left, "Key Left", \
                  GPIO_PORT_PIN_TO_GPIO_HAL_PIN(BUTTON_LEFT_PORT, BUTTON_LEFT_PIN), \
                  GPIO_HAL_PIN_CFG_PULL_UP, BUTTON_HAL_ID_BUTTON_ZERO, true);
BUTTON_HAL_BUTTON(key_right, "Key Right", \
                  GPIO_PORT_PIN_TO_GPIO_HAL_PIN(BUTTON_RIGHT_PORT, BUTTON_RIGHT_PIN), \
                  GPIO_HAL_PIN_CFG_PULL_UP, BUTTON_HAL_ID_BUTTON_ONE, true);
BUTTON_HAL_BUTTON(key_up, "Key Up", \
                  GPIO_PORT_PIN_TO_GPIO_HAL_PIN(BUTTON_UP_PORT, BUTTON_UP_PIN), \
                  GPIO_HAL_PIN_CFG_PULL_UP, BUTTON_HAL_ID_BUTTON_TWO, true);
BUTTON_HAL_BUTTON(key_down, "Key Down", \
                  GPIO_PORT_PIN_TO_GPIO_HAL_PIN(BUTTON_DOWN_PORT, BUTTON_DOWN_PIN), \
                  GPIO_HAL_PIN_CFG_PULL_UP, BUTTON_HAL_ID_BUTTON_THREE, true);
BUTTON_HAL_BUTTON(key_select, "Key Select", \
                  GPIO_PORT_PIN_TO_GPIO_HAL_PIN(BUTTON_SELECT_PORT, BUTTON_SELECT_PIN), \
                  GPIO_HAL_PIN_CFG_PULL_UP, BUTTON_HAL_ID_BUTTON_FOUR, true);
/*---------------------------------------------------------------------------*/
BUTTON_HAL_BUTTONS(&key_left, &key_right, &key_up, &key_down, &key_select);
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
