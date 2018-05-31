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
 * \addtogroup sensortag-cc26xx-reed-relay
 * @{
 *
 * \file
 *  Driver for the Sensortag Reed Relay
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/clock.h"
#include "sys/timer.h"
#include "lib/sensors.h"
#include "sys/timer.h"
#include "reed-relay.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>
#include <ti/drivers/PIN.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
static struct timer debouncetimer;
/*---------------------------------------------------------------------------*/
static PIN_Config reed_pin_table[] = {
  Board_RELAY | PIN_INPUT_EN | PIN_PULLDOWN | PIN_IRQ_DIS,
  PIN_TERMINATE
};

static PIN_State  pin_state;
static PIN_Handle pin_handle;
/*---------------------------------------------------------------------------*/
static bool
sensor_init(void)
{
  if (pin_handle) {
    return true;
  }

  pin_handle = PIN_open(&pin_state, reed_pin_table);
  return pin_handle != NULL;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Handler for Sensortag-CC26XX reed interrupts
 */
static void
reed_relay_isr(PIN_Handle handle, PIN_Id pinId)
{
  (void)handle;
  (void)pinId;

  if (!timer_expired(&debouncetimer)) {
    return;
  }

  timer_set(&debouncetimer, CLOCK_SECOND / 2);
  sensors_changed(&reed_relay_sensor);
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  return (int)PIN_getInputValue(Board_RELAY);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Configuration function for the button sensor for all buttons.
 *
 * \param type SENSORS_HW_INIT: Initialise. SENSORS_ACTIVE: Enables/Disables
 *        depending on 'value'
 * \param value 0: disable, non-zero: enable
 * \return Always returns 1
 */
static int
configure(int type, int value)
{
  switch(type) {
  case SENSORS_HW_INIT:
    if (!sensor_init()) {
      return REED_RELAY_READING_ERROR;
    }

    PIN_setInterrupt(pin_handle, Board_RELAY | PIN_IRQ_DIS);
    PIN_registerIntCb(pin_handle, reed_relay_isr);
    break;

  case SENSORS_ACTIVE:
    if (value) {
      PIN_setInterrupt(pin_handle, Board_RELAY | PIN_IRQ_NEGEDGE);
    } else {
      PIN_setInterrupt(pin_handle, Board_RELAY | PIN_IRQ_DIS);
    }
    break;

  default:
    break;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Status function for the reed
 * \param type SENSORS_ACTIVE or SENSORS_READY
 * \return 1 Interrupt enabled, 0: Disabled
 */
static int
status(int type)
{
  switch (type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return (PIN_getConfig(Board_RELAY) & PIN_BM_IRQ) != 0;
    break;

  default:
    break;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(reed_relay_sensor, "REED", value, configure, status);
/*---------------------------------------------------------------------------*/
/** @} */
