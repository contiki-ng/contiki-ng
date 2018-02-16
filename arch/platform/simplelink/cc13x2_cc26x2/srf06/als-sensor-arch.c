/*
 * Copyright (c) 2016, University of Bristol - http://www.bris.ac.uk/
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
 * \addtogroup srf06-common-peripherals
 * @{
 *
 * \file
 *  Driver for the SmartRF06EB ALS when a CC13xx/CC26xxEM is mounted on it
 */
/*---------------------------------------------------------------------------*/
#include <contiki.h>
#include <lib/sensors.h>
#include <sys/clock.h>
/*---------------------------------------------------------------------------*/
#include <Board.h>
#include <driverlib/aux_adc.h>
#include <driverlib/ioc.h>
#include <driverlib/gpio.h>
/*---------------------------------------------------------------------------*/
#include "als-sensor.h"
#include "als-sensor-arch.h"
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/* SmartRF06 EB has one Analog Light Sensor (ALS) */
#define ALS_OUT_DIO  Board_ALS_OUT
#define ALS_PWR_DIO  Board_ALS_PWR
/*---------------------------------------------------------------------------*/
static int
config(int type, int value)
{
  switch (type) {
  case SENSORS_ACTIVE:
    IOCPinTypeGpioOutput(ALS_PWR_DIO);
    IOCPortConfigureSet(ALS_OUT_DIO, IOC_PORT_GPIO, IOC_STD_OUTPUT);
    IOCPinTypeGpioInput(ALS_OUT_DIO);

    if (value) {
      GPIO_setDio(ALS_PWR_DIO);
      AUXADCSelectInput(ALS_OUT_DIO);
      clock_delay_usec(2000);
    } else {
      GPIO_clearDio(ALS_PWR_DIO);
    }
    break;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  AUXADCEnableSync(AUXADC_REF_VDDS_REL, AUXADC_SAMPLE_TIME_2P7_US, AUXADC_TRIGGER_MANUAL);
  AUXADCGenManualTrigger();

  int val = AUXADCReadFifo();

  AUXADCDisable();

  return val;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(als_sensor, ALS_SENSOR, value, config, status);
/*---------------------------------------------------------------------------*/
/** @} */
