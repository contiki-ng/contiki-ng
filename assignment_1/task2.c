/*
* Copyright (C) 2015, Intel Corporation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
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

#include <stdio.h>

#include "contiki.h"
#include "sys/rtimer.h"
#include "sys/etimer.h"
#include "buzzer.h"

#include "board-peripherals.h"

#include <stdint.h>

// PROCESS(process_rtimer, "RTimer");
PROCESS(process_main, "Main");
AUTOSTART_PROCESSES(&process_main);

static int loop_cnt;
static int counter_rtimer;
static int counter_etimer;
static struct rtimer timer_rtimer;
static struct etimer timer_etimer;
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND /4;
static int prv_lux_value = NULL;
static int buzzer_status = 0;
/*---------------------------------------------------------------------------*/
static int get_mpu_reading(void);
static void init_opt_reading(void);
static int get_light_reading(void);
static void init_mpu_reading(void);
static void schedule_rtimer(void);

/*---------------------------------------------------------------------------*/

static void
do_rtimer_timeout(struct rtimer *timer, void *ptr)
{

  rtimer_clock_t now=RTIMER_NOW();

  int s, ms1,ms2,ms3;
  s = now /RTIMER_SECOND;
  ms1 = (now% RTIMER_SECOND)*10/RTIMER_SECOND;
  ms2 = ((now% RTIMER_SECOND)*100/RTIMER_SECOND)%10;
  ms3 = ((now% RTIMER_SECOND)*1000/RTIMER_SECOND)%10;
  
  counter_rtimer++;
  printf("rtimer: %d (cnt) %d (ticks) %d.%d%d%d (sec) \n",counter_rtimer,now, s, ms1,ms2,ms3); 

  if (get_light_reading() || get_mpu_reading()) {
    process_poll(&process_main);
  } else {
    schedule_rtimer();
  }
}

static void
schedule_rtimer()
{
  rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);
}

static int
get_light_reading()
{
  int value, lux_value;

  value = opt_3001_sensor.value(0);
  if (value != CC26XX_SENSOR_READING_ERROR) {
    lux_value = value / 100;
    printf("OPT: Light=%d.%02d lux\n", lux_value, value % 100);

    if (prv_lux_value != NULL && abs(lux_value - prv_lux_value) >= 300) {
      return 1;
    }
    prv_lux_value = lux_value;

  } else {
    printf("OPT: Light Sensor's Warming Up\n\n");
  }

  init_opt_reading();
  return 0;
}

static void
init_opt_reading(void)
{
  SENSORS_ACTIVATE(opt_3001_sensor);
}

static int
get_mpu_reading()
{
  int value;

  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
  printf("MPU Gyro: X= %d.%02d deg/sec\n", value/100, abs(value)%100);

  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
  printf("MPU Gyro: Y= %d.%02d deg/sec\n", value/100, abs(value)%100);

  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);
  printf("MPU Gyro: Z= %d.%02d deg/sec\n", value/100, abs(value)%100);

  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
  printf("MPU Acc: X= %d.%02d G\n", value/100, abs(value)%100);

  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
  printf("MPU Acc: Y= %d.%02d G\n", value/100, abs(value)%100);

  value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
  printf("MPU Acc: Z= %d.%02d G\n", value/100, abs(value)%100);

  return 0;
}

static void
init_mpu_reading(void)
{
  mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL);
}

static void
toggle_buzzing()
{
  clock_time_t t;
  int s, ms1, ms2, ms3;
  t = clock_time();
  s = t / CLOCK_SECOND;
  ms1 = (t% CLOCK_SECOND)*10/CLOCK_SECOND;
  ms2 = ((t% CLOCK_SECOND)*100/CLOCK_SECOND)%10;
  ms3 = ((t% CLOCK_SECOND)*1000/CLOCK_SECOND)%10;

  counter_etimer++;
  printf("Toggling buzzer to %d at time(E): %d (cnt) %d (ticks) %d.%d%d%d (sec) \n",!buzzer_status,counter_etimer,t,s,ms1,ms2,ms3); 
  // Toggle the buzzer
  if (buzzer_status)
    buzzer_stop();
  else
    buzzer_start(1000);

  buzzer_status = !buzzer_status;
}

PROCESS_THREAD(process_main, ev, data)
{
  PROCESS_BEGIN();

  init_mpu_reading();
  buzzer_init();

  while (1) {
    prv_lux_value = NULL; // Reset prv_lux_value
    init_opt_reading(); // Reset opt reading
    schedule_rtimer(); // Restart sensors

    // Yield until polled
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

    // Start buzzing
    for (loop_cnt = 0; loop_cnt < 3; loop_cnt++) { // Needs to be odd number
      toggle_buzzing();

      etimer_set(&timer_etimer, CLOCK_SECOND * 2);  // 2s timer
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_etimer));
    }
    toggle_buzzing();
  }

  PROCESS_END();
}
