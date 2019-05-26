/*
 * Copyright (c) 2019, Carlo Vallati - http://www.unipi.it
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
 * \addtogroup NRF52
 * @{
 *
 * \defgroup nrf52-buttons NRF52 Buttons
 *
 * Generic module controlling buttons on the NRF52
 * @{
 *
 * \file
 * Defines NRF52 buttons for use with the button HAL
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/button-hal.h"
/*---------------------------------------------------------------------------*/
BUTTON_HAL_BUTTON(key_1, "Key 1", \
				  BSP_BUTTON_0, \
                  GPIO_HAL_PIN_CFG_PULL_UP, BUTTON_HAL_ID_BUTTON_ZERO, true);
BUTTON_HAL_BUTTON(key_2, "Key 2", \
				  BSP_BUTTON_1, \
                  GPIO_HAL_PIN_CFG_PULL_UP, BUTTON_HAL_ID_BUTTON_ONE, true);
BUTTON_HAL_BUTTON(key_3, "Key 3", \
				  BSP_BUTTON_2, \
                  GPIO_HAL_PIN_CFG_PULL_UP, BUTTON_HAL_ID_BUTTON_TWO, true);
BUTTON_HAL_BUTTON(key_4, "Key 4", \
				  BSP_BUTTON_3, \
                  GPIO_HAL_PIN_CFG_PULL_UP, BUTTON_HAL_ID_BUTTON_THREE, true);
/*---------------------------------------------------------------------------*/
BUTTON_HAL_BUTTONS(&key_1, &key_2, &key_3, &key_4);
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
