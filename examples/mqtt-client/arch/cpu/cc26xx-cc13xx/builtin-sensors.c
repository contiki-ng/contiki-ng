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
#include "contiki.h"
#include "batmon-sensor.h"
#include "mqtt-client.h"

#include <string.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
#define TMP_BUF_SZ 32
/*---------------------------------------------------------------------------*/
char tmp_buf[TMP_BUF_SZ];
/*---------------------------------------------------------------------------*/
static char *
temp_reading(void)
{
  memset(tmp_buf, 0, TMP_BUF_SZ);
  snprintf(tmp_buf, TMP_BUF_SZ, "\"On-Chip Temp (mC)\":%d",
           batmon_sensor.value(BATMON_SENSOR_TYPE_TEMP));
  return tmp_buf;
}
/*---------------------------------------------------------------------------*/
static void
temp_init(void)
{
  SENSORS_ACTIVATE(batmon_sensor);
}
/*---------------------------------------------------------------------------*/
const mqtt_client_extension_t builtin_sensors_batmon_temp = {
  temp_init,
  temp_reading,
};
/*---------------------------------------------------------------------------*/
static char *
volt_reading(void)
{
  memset(tmp_buf, 0, TMP_BUF_SZ);
  snprintf(tmp_buf, TMP_BUF_SZ, "\"Volt (mV)\":%d",
           (batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT) * 125) >> 5);
  return tmp_buf;
}
/*---------------------------------------------------------------------------*/
static void
volt_init(void)
{
  SENSORS_ACTIVATE(batmon_sensor);
}
/*---------------------------------------------------------------------------*/
const mqtt_client_extension_t builtin_sensors_batmon_volt = {
  volt_init,
  volt_reading,
};
/*---------------------------------------------------------------------------*/
