/*
 * Copyright (C) 2021 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
/*---------------------------------------------------------------------------*/

/**
 * \file
 *      DHT 11 sensor example
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

#include "contiki.h"

#include <stdio.h>

#include "dht11-sensor.h"
/*---------------------------------------------------------------------------*/
PROCESS(dht11_process, "DHT 11 process");
AUTOSTART_PROCESSES(&dht11_process);
/*---------------------------------------------------------------------------*/
#define DHT11_GPIO_PORT (1)
#define DHT11_GPIO_PIN  (12)
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(dht11_process, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  dht11_sensor.configure(DHT11_CONFIGURE_GPIO_PORT, DHT11_GPIO_PORT);
  dht11_sensor.configure(DHT11_CONFIGURE_GPIO_PIN, DHT11_GPIO_PIN);
  dht11_sensor.configure(SENSORS_HW_INIT, 0);

  /* Wait one second for the DHT11 sensor to be ready */
  etimer_set(&timer, CLOCK_SECOND * 1);

  /* Wait for the periodic timer to expire */
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  /* Setup a periodic timer that expires after 5 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 5);
  while(1) {
    /*
     * Request a fresh read
     */
    SENSORS_ACTIVATE(dht11_sensor);

    printf("%ld ", (unsigned long)clock_time());
    switch(dht11_sensor.status(0)) {
    case DHT11_STATUS_OKAY:
      printf("Humidity %d.%d %% ",
             dht11_sensor.value(DHT11_VALUE_HUMIDITY_INTEGER),
             dht11_sensor.value(DHT11_VALUE_HUMIDITY_DECIMAL));
      printf("Temperature = %d.%d *C\n",
             dht11_sensor.value(DHT11_VALUE_TEMPERATURE_INTEGER),
             dht11_sensor.value(DHT11_VALUE_TEMPERATURE_DECIMAL));
      break;
    case DHT11_STATUS_CHECKSUM_FAILED:
      printf("Check sum failed\n");
      break;
    case DHT11_STATUS_TIMEOUT:
      printf("Reading timed out\n");
      break;
    default:
      break;
    }

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
