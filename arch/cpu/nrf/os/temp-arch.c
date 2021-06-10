/*
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-os OS drivers
 * @{
 *
 * \addtogroup nrf-temp Temperature driver
 * @{
 * 
 * \file
 *         Temperatue implementation for the nRF.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "nrfx.h"

#include "lib/sensors.h"
/*---------------------------------------------------------------------------*/
#define TEMPERATURE_SENSOR    "Temperature"
/*---------------------------------------------------------------------------*/
#ifdef NRF_TEMP
/*---------------------------------------------------------------------------*/
#include "hal/nrf_temp.h"
/*---------------------------------------------------------------------------*/
#define TEMP_ARCH_WAIT_US 4
#define TEMP_ARCH_TRIES   10
/*---------------------------------------------------------------------------*/
/**
 * @brief Returns device temperature
 * @param type ignored
 * @return Device temperature in degrees Celsius
 */
static int
value(int type)
{
  uint8_t tries;

  (void) type;

  nrf_temp_event_clear(NRF_TEMP, NRF_TEMP_EVENT_DATARDY);
  nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_START);

  tries = TEMP_ARCH_TRIES;
  do {
    if(nrf_temp_event_check(NRF_TEMP, NRF_TEMP_EVENT_DATARDY)) {
      break;
    }
    NRFX_DELAY_US(TEMP_ARCH_WAIT_US);
  } while(--tries);

  nrf_temp_event_clear(NRF_TEMP, NRF_TEMP_EVENT_DATARDY);
  nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_STOP);

  return nrf_temp_result_get(NRF_TEMP);
}
/*---------------------------------------------------------------------------*/
/**
 * @brief Configures temperature sensor
 * @param type ignored
 * @param c ignored
 * @return 1
 */
static int
configure(int type, int c)
{
  (void) type;
  (void) c;

  return 1;
}
/**
 * \brief Return temperature sensor status
 * \param type ignored
 * \return 1
 */
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  (void) type;

  return 1;
}
/*---------------------------------------------------------------------------*/
#else /* NRF_TEMP */
/**
 * \brief Returns device temperature
 * \param type ignored
 * \return Device temperature in degrees Celsius
 */
static int
value(int type)
{
  (void) type;

  return 0;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Configures temperature sensor
 * \param type ignored
 * \param c ignored
 * \return 1
 */
static int
configure(int type, int c)
{
  (void) type;
  (void) c;

  return 0;
}
/**
 * \brief Return temperature sensor status
 * \param type ignored
 * \return 1
 */
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  (void) type;

  return 0;
}
/*---------------------------------------------------------------------------*/
#endif /* NRF_TEMP */
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(temperature_sensor, TEMPERATURE_SENSOR, value, configure, status);
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
