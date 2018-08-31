/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
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
/**
 * \addtogroup srf06-peripherals
 * @{
 *
 * \file
 *        Button HAL definitions for the SmartRF06 Evaluation Board's buttons.
 *        Common across all CC13xx/CC26xx devices on SmartRF06 EB.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/button-hal.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>
/*---------------------------------------------------------------------------*/
/* Key select button */
BUTTON_HAL_BUTTON(
  key_select,                   /**< Name */
  "Key Select",                 /**< Description */
  Board_KEY_SELECT,             /**< PIN */
  GPIO_HAL_PIN_CFG_PULL_UP |
  GPIO_HAL_PIN_CFG_HYSTERESIS,  /**< Pull configuration */
  BUTTON_HAL_ID_KEY_SELECT,     /**< Unique ID */
  true);                        /**< Negative logic */

/* Key up button */
BUTTON_HAL_BUTTON(
  key_up,                       /**< Name */
  "Key Up",                     /**< Description */
  Board_KEY_UP,                 /**< PIN */
  GPIO_HAL_PIN_CFG_PULL_UP |
  GPIO_HAL_PIN_CFG_HYSTERESIS,  /**< Pull configuration */
  BUTTON_HAL_ID_KEY_UP,         /**< Unique ID */
  true);                        /**< Negative logic */

/* Key down button */
BUTTON_HAL_BUTTON(
  key_down,                     /**< Name */
  "Key Down",                   /**< Description */
  Board_KEY_DOWN,               /**< PIN */
  GPIO_HAL_PIN_CFG_PULL_UP |
  GPIO_HAL_PIN_CFG_HYSTERESIS,  /**< Pull configuration */
  BUTTON_HAL_ID_KEY_DOWN,       /**< Unique ID */
  true);                        /**< Negative logic */

/* Key left button */
BUTTON_HAL_BUTTON(
  key_left,                     /**< Name */
  "Key Left",                   /**< Description */
  Board_KEY_LEFT,               /**< PIN */
  GPIO_HAL_PIN_CFG_PULL_UP |
  GPIO_HAL_PIN_CFG_HYSTERESIS,  /**< Pull configuration */
  BUTTON_HAL_ID_KEY_LEFT,       /**< Unique ID */
  true);                        /**< Negative logic */

/* Key right button */
BUTTON_HAL_BUTTON(
  key_right,                    /**< Name */
  "Key Right",                  /**< Description */
  Board_KEY_RIGHT,              /**< PIN */
  GPIO_HAL_PIN_CFG_PULL_UP |
  GPIO_HAL_PIN_CFG_HYSTERESIS,  /**< Pull configuration */
  BUTTON_HAL_ID_KEY_RIGHT,      /**< Unique ID */
  true);                        /**< Negative logic */
/*---------------------------------------------------------------------------*/
BUTTON_HAL_BUTTONS(&key_select, &key_up, &key_down, &key_left, &key_right);
/*---------------------------------------------------------------------------*/
/** @} */
