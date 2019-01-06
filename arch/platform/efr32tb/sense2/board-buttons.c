/*
 * Copyright (c) 2018, Joakim Eriksson, RISE AB
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
 * \addtogroup thunderboard peripherals
 * @{
 *
 * \file
 * Defines TB Sense 2's buttons for use with the button HAL
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/gpio-hal.h"
#include "dev/button-hal.h"
#include "em_gpio.h"
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
BUTTON_HAL_BUTTON(button_left, "Button Left", BOARD_BUTTON_PORT,        \
                  BOARD_BUTTON_LEFT_PIN,                                \
                  GPIO_HAL_PIN_CFG_PULL_DOWN, \
                  BOARD_BUTTON_LEFT_PIN, true);

BUTTON_HAL_BUTTON(button_right, "Button Right", BOARD_BUTTON_PORT,      \
                  BOARD_BUTTON_RIGHT_PIN,                               \
                  GPIO_HAL_PIN_CFG_PULL_DOWN, \
                  BOARD_BUTTON_RIGHT_PIN, true);
/*---------------------------------------------------------------------------*/
BUTTON_HAL_BUTTONS(&button_left, &button_right);
/*---------------------------------------------------------------------------*/
/** @} */
