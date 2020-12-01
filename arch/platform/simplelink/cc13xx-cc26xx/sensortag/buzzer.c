/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup sensortag-buzzer
 * @{
 *
 * \file
 *        Driver for the Sensortag Buzzer.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "buzzer.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/timer/GPTimerCC26XX.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/* Configure BUZZER pin */
#ifndef Board_BUZZER
#error "Board file doesn't define pin Board_BUZZER"
#endif
#define BUZZER_PIN          Board_BUZZER
/*---------------------------------------------------------------------------*/
static const PIN_Config pin_table[] = {
  BUZZER_PIN | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW,
  PIN_TERMINATE
};

static PIN_State pin_state;
static PIN_Handle pin_handle;

static GPTimerCC26XX_Handle gpt_handle;

static bool has_init;
static volatile bool is_running;
/*---------------------------------------------------------------------------*/
bool
buzzer_init()
{
  if(has_init) {
    return true;
  }

  GPTimerCC26XX_Params gpt_params;
  GPTimerCC26XX_Params_init(&gpt_params);

  gpt_params.width = GPT_CONFIG_16BIT;
  gpt_params.mode = GPT_MODE_PWM;
  gpt_params.debugStallMode = GPTimerCC26XX_DEBUG_STALL_OFF;

  gpt_handle = GPTimerCC26XX_open(Board_GPTIMER0A, &gpt_params);
  if(!gpt_handle) {
    return false;
  }

  is_running = false;

  has_init = true;
  return true;
}
/*---------------------------------------------------------------------------*/
bool
buzzer_running()
{
  return is_running;
}
/*---------------------------------------------------------------------------*/
bool
buzzer_start(uint32_t freq)
{
  if(!has_init) {
    return false;
  }

  if(freq == 0) {
    return false;
  }

  if(is_running) {
    return true;
  }

  pin_handle = PIN_open(&pin_state, pin_table);
  if(!pin_handle) {
    return false;
  }

  Power_setDependency(PowerCC26XX_XOSC_HF);

  PINCC26XX_setMux(pin_handle, BUZZER_PIN, GPT_PIN_0A);

  /* MCU runs at 48 MHz */
  GPTimerCC26XX_Value load_value = (48 * 1000 * 1000) / freq;

  GPTimerCC26XX_setLoadValue(gpt_handle, load_value);
  GPTimerCC26XX_setMatchValue(gpt_handle, load_value / 2);
  GPTimerCC26XX_start(gpt_handle);

  is_running = true;
  return true;
}
/*---------------------------------------------------------------------------*/
void
buzzer_stop()
{
  if(!gpt_handle) {
    return;
  }

  if(!is_running) {
    return;
  }

  Power_releaseDependency(PowerCC26XX_XOSC_HF);

  GPTimerCC26XX_stop(gpt_handle);

  PIN_close(pin_handle);
  pin_handle = NULL;

  is_running = false;
}
/*---------------------------------------------------------------------------*/
/** @} */
