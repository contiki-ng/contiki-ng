/*
 * Copyright (c) 2015
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
 * \addtogroup sensorcontroller-sensor
 * @{
 *
 * \file
 *  Driver for the CC2650 Sensorcontroller
 */
/*---------------------------------------------------------------------------*/
#include <sensorcontroller/scif.h>

#include "contiki-conf.h"
#include "sys/ctimer.h"
#include "lib/sensors.h"
#include "dev/aux-ctrl.h"
#include "sensorcontroller-sensor.h"
#include "sensor-common.h"
#include "ti-lib.h"
#include "lpm.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Display error message if the SCIF driver has been generated with incorrect operating system setting
#ifndef SCIF_OSAL_NONE_H
    #error "Generated SCIF driver supports incorrect operating system. Please change to 'None' in the Sensor Controller Studio project panel and re-generate the driver."
#endif
#define BV(n)           (1 << (n))

/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/

/* Sensor selection/deselection */
#define SENSOR_SELECT()
#define SENSOR_DESELECT()

/*---------------------------------------------------------------------------*/
static aux_consumer_module_t sensorcontroller_sensor_aux = {
  .clocks = 	AUX_WUC_SMPH_CLOCK |
			AUX_WUC_AIODIO0_CLOCK |
			AUX_WUC_AIODIO1_CLOCK |
			AUX_WUC_TIMER_CLOCK |
  	  	  	AUX_WUC_ANAIF_CLOCK |
			AUX_WUC_TDCIF_CLOCK |
			AUX_WUC_OSCCTRL_CLOCK |
			AUX_WUC_ADI_CLOCK
};

static void
notify_ready(void *not_used);
/*---------------------------------------------------------------------------*/
/* Raw data */
static uint16_t cap_dp0;
/*---------------------------------------------------------------------------*/
static bool success;
/*---------------------------------------------------------------------------*/
static int enabled = SENSORCONTROLLER_SENSOR_STATUS_DISABLED;
/*---------------------------------------------------------------------------*/

void scTaskAlertCallback(void) {
    // Clear the ALERT interrupt source
    scifClearAlertIntSource();
    HWREG(AON_WUC_BASE + AON_WUC_O_AUXCTL) &= ~(AON_WUC_AUXCTL_SWEV);
    if ((scifGetAlertEvents() >> 8) & BV(SCIF_CAPACITIVE_SOIL_HUMIDITY_SENSOR_TASK_ID)) {
         // Send an error message over UART
    		PRINTF("Overflow error has occurred!\r\n");
    }

	// For each output buffer ready to be processed ...
	while (scifGetTaskIoStructAvailCount(SCIF_CAPACITIVE_SOIL_HUMIDITY_SENSOR_TASK_ID, SCIF_STRUCT_OUTPUT) != 0) {
		SCIF_CAPACITIVE_SOIL_HUMIDITY_SENSOR_OUTPUT_T* pOutput = (SCIF_CAPACITIVE_SOIL_HUMIDITY_SENSOR_OUTPUT_T*) scifGetTaskStruct(SCIF_CAPACITIVE_SOIL_HUMIDITY_SENSOR_TASK_ID, SCIF_STRUCT_OUTPUT);
		cap_dp0 = pOutput->pTdcValueFilt[0];
		// Hand the output buffer back to the Sensor Controller
		scifHandoffTaskStruct(SCIF_CAPACITIVE_SOIL_HUMIDITY_SENSOR_TASK_ID, SCIF_STRUCT_OUTPUT);
	}
    // Acknowledge the alert event
    scifAckAlertEvents();
    notify_ready(NULL);
} // taskAlertCallback


void scCtrlReadyCallback(void) {

} // ctrlReadyCallback


/*---------------------------------------------------------------------------*/
/**
 * \brief       Initialise the sensorcontroller driver
 * \return
 */
static bool
sensor_init(void)
{
	scifOsalRegisterCtrlReadyCallback(scCtrlReadyCallback);
	scifOsalRegisterTaskAlertCallback(scTaskAlertCallback);
	scifInit(&scifDriverSetup);

	// Start the Capacitive Touch Data Logger task
	scifStartTasksNbl(BV(SCIF_CAPACITIVE_SOIL_HUMIDITY_SENSOR_TASK_ID));
	success = true;
	return success;
}

/*---------------------------------------------------------------------------*/
static void
notify_ready(void *not_used)
{
	enabled = SENSORCONTROLLER_SENSOR_STATUS_READINGS_READY;
	sensors_changed(&sensorcontroller_sensor);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Returns a reading from the sensor
 */
static int
value(int type)
{
  int rv = 0;

  if(enabled != SENSORCONTROLLER_SENSOR_STATUS_READINGS_READY) {
    PRINTF("Sensor disabled or starting up (%d)\n", enabled);
    return CC26XX_SENSOR_READING_ERROR;
  }

  if (type == SENSORCONTROLLER_SENSOR_TYPE_CAP_DP0) {
    PRINTF("CAP_DP0: %d\n", cap_dp0);
    rv = cap_dp0;
  } else  {
	PRINTF("Invalid type\n");
	rv = CC26XX_SENSOR_READING_ERROR;
  }
  return rv;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int enable)
{
  switch(type) {
  case SENSORS_HW_INIT:
	cap_dp0 = 0;
	aux_ctrl_register_consumer(&sensorcontroller_sensor_aux);
    sensor_init();
    enabled = SENSORCONTROLLER_SENSOR_STATUS_INITIALISED;
    break;
  case SENSORS_ACTIVE:
    /* Must be initialised first */
    if(enabled == SENSORCONTROLLER_SENSOR_STATUS_DISABLED) {
      return SENSORCONTROLLER_SENSOR_STATUS_DISABLED;
    }
    if(enable) {
    	  // Trigger sensorcontroller task
  	  HWREG(AON_WUC_BASE + AON_WUC_O_AUXCTL) |= AON_WUC_AUXCTL_SWEV;
      enabled = SENSORCONTROLLER_SENSOR_STATUS_TAKING_READINGS;
    } else {
      enabled = SENSORCONTROLLER_SENSOR_STATUS_DISABLED;
      scifUninit();
      aux_ctrl_unregister_consumer(&sensorcontroller_sensor_aux);
    }
    break;
  default:
    break;
  }
  return enabled;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Returns the status of the sensor
 * \param type SENSORS_ACTIVE or SENSORS_READY
 * \return One of the SENSOR_STATUS_xyz defines
 */
static int
status(int type)
{
  switch(type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return enabled;
    break;
  default:
    break;
  }
  return SENSORCONTROLLER_SENSOR_STATUS_DISABLED;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(sensorcontroller_sensor, "SENSORCONTROLLER", value, configure, status);
/*---------------------------------------------------------------------------*/
/** @} */
