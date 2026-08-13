/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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

/**
 * \file
 *         Button HAL definitions for MSP-EXP430FR5969 LaunchPad
 *
 *         S1 -> P4.5, S2 -> P1.1. Both buttons short to ground when
 *         pressed and rely on the MCU's internal pull-up.
 */

#include "contiki.h"
#include "dev/button-hal.h"

BUTTON_HAL_BUTTON(button_s1, "Button S1",
                  BUTTON_S1_PORT, BUTTON_S1_PIN,
                  GPIO_HAL_PIN_CFG_PULL_UP,
                  BUTTON_HAL_ID_BUTTON_ZERO, true);

BUTTON_HAL_BUTTON(button_s2, "Button S2",
                  BUTTON_S2_PORT, BUTTON_S2_PIN,
                  GPIO_HAL_PIN_CFG_PULL_UP,
                  BUTTON_HAL_ID_BUTTON_ONE, true);

BUTTON_HAL_BUTTONS(&button_s1, &button_s2);
