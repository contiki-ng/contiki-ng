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
 * \addtogroup dht11-sensor
 * @{
 * \file
 *      DHT 11 sensor implementation
 *
 * \see https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf
 *
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

#include "contiki.h"
#include "dht11-sensor.h"
#include <string.h>
#include "dev/gpio-hal.h"

/*---------------------------------------------------------------------------*/
/**
 * @brief GPIO High
 *
 */
#define DHT11_SIGNAL_HIGH           (1)

/**
 * @brief GPIO Low
 *
 */
#define DHT11_SIGNAL_LOW            (0)

/**
 * @brief Duration of signal start phase 1 according to data sheet
 *
 */
#define DHT11_SIGNAL_START_PHASE1_DURATION (40)

/**
 * @brief Duration of signal start phase 2 according to data sheet
 *
 */
#define DHT11_SIGNAL_START_PHASE2_DURATION (80)

/**
 * @brief Duration of signal start phase 3 according to data sheet
 *
 */
#define DHT11_SIGNAL_START_PHASE3_DURATION (80)

/**
 * @brief Duration of signal response phase 1 according to data sheet
 *
 */
#define DHT11_SIGNAL_RESPONSE_PHASE1_DURATION (50)

/**
 * @brief Duration of signal response if bit is set to 0, according to data sheet
 *
 */
#define DHT11_SIGNAL_RESPONSE_BIT_0_DURATION  (28)

/**
 * @brief Duration of signal response if bit is set to 1, according to data sheet
 *
 */
#define DHT11_SIGNAL_RESPONSE_BIT_1_DURATION  (70)

/**
 * @brief Sensor timer drift in ticks
 *
 * DHT uses 1us granularity and rtimer granularity is higher.
 * So, allow the reading to drift by 1 tick
 *
 */
#define DHT11_TICKS_GUARD  (1)

/**
 * @brief Sensor timer drift in us from rtimer
 *
 * DHT uses 1us granularity and rtimer granularity is higher.
 * So, allow the reading to drift by 1 tick in us
 *
 */
#define DHT11_US_GUARD     RTIMERTICKS_TO_US(1)

/**
 * @brief Number of data requests
 *
 */
#define DHT11_DATA_SAMPLES  (40)

/**
 * @brief Number of bytes in data
 *
 */
#define DHT11_DATA_SIZE     (5)

/**
 * @brief DHT11 maximum sample rate is 1 Hz (1 second)
 *
 */
#define DHT11_SAMPLING_RATE_SECONDS (1)
/*---------------------------------------------------------------------------*/
/**
 * @brief DHT struct
 *
 */
typedef struct {
  /**
   * @brief GPIO Port
   *
   */
  gpio_hal_port_t port;
  /**
   * @brief GPIO Pin
   *
   */
  gpio_hal_pin_t pin;
  /**
   * @brief DH status
   *
   */
  uint8_t status;
  /**
   * @brief Time of last read
   *
   */
  clock_time_t last_read;
  /**
   * @brief Data array
   *
   */
  uint8_t data[DHT11_DATA_SIZE];
} dht_t;

/**
 * @brief DHT struct
 *
 */
static dht_t dht;
/*---------------------------------------------------------------------------*/
static int
dht11_humidity_integer(void)
{
  return dht.data[0];
}
/*---------------------------------------------------------------------------*/
static int
dht11_humidity_decimal(void)
{
  return dht.data[1];
}
/*---------------------------------------------------------------------------*/
static int
dht11_temperature_integer(void)
{
  return dht.data[2];
}
/*---------------------------------------------------------------------------*/
static int
dht11_temperature_decimal(void)
{
  return dht.data[3];
}
/*---------------------------------------------------------------------------*/
static int8_t
dht_signal_duration(uint8_t active, uint32_t max_duration)
{
  rtimer_clock_t elapsed_ticks;
  rtimer_clock_t max_wait_ticks = US_TO_RTIMERTICKS(max_duration) + DHT11_TICKS_GUARD;
  rtimer_clock_t start_ticks = RTIMER_NOW();

  /* Wait for signal to change */
  RTIMER_BUSYWAIT_UNTIL(gpio_hal_arch_read_pin(dht.port, dht.pin) != active, max_wait_ticks);

  elapsed_ticks = RTIMER_NOW() - start_ticks;

  if(elapsed_ticks > max_wait_ticks) {
    return -1;
  }

  return RTIMERTICKS_TO_US(elapsed_ticks);
}
/*---------------------------------------------------------------------------*/
static int8_t
dht_signal_transition(uint8_t active, uint32_t max_duration)
{
  return dht_signal_duration(active, max_duration);
}
/*---------------------------------------------------------------------------*/
static uint8_t
dht_verify_checksum(void)
{
  return ((dht.data[0] + dht.data[1] + dht.data[2] + dht.data[3]) & 0xFF) == dht.data[4];
}
/*---------------------------------------------------------------------------*/
static uint8_t
dht_read(void)
{
  uint8_t j, i;
  /* Array to store the duration of each data signal to be calculated later */
  int8_t data_signal_duration[DHT11_DATA_SAMPLES];

  /**
   * Data Single-bus free status is at high voltage level. When the communication
   * between MCU and DHT11 begins, the programme of MCU will set Data Single-bus
   * voltage level from high to low and this process must take at least 18ms to
   * ensure DHT’s detection of MCU's signal, then MCU will pull up voltage and
   * wait 20-40us for DHT’s response.
   */
  gpio_hal_arch_pin_set_output(dht.port, dht.pin);
  gpio_hal_arch_clear_pin(dht.port, dht.pin);
  RTIMER_BUSYWAIT(US_TO_RTIMERTICKS(18000UL));
  gpio_hal_arch_set_pin(dht.port, dht.pin);
  gpio_hal_arch_pin_set_input(dht.port, dht.pin);

  if(dht_signal_transition(DHT11_SIGNAL_HIGH, DHT11_SIGNAL_START_PHASE1_DURATION) == -1) {
    return DHT11_STATUS_TIMEOUT;
  }

  /**
   * Once DHT detects the start signal,it will send out a low-voltage-level response
   * signal, which lasts 80us. Then the programme of DHT sets Data Single-bus voltage
   * level from low to high and keeps it for 80us for DHT’s preparation for sending data.
   */
  if(dht_signal_transition(DHT11_SIGNAL_LOW, DHT11_SIGNAL_START_PHASE2_DURATION) == -1) {
    return DHT11_STATUS_TIMEOUT;
  }

  if(dht_signal_transition(DHT11_SIGNAL_HIGH, DHT11_SIGNAL_START_PHASE3_DURATION) == -1) {
    return DHT11_STATUS_TIMEOUT;
  }

  for(i = 0; i < DHT11_DATA_SAMPLES; i++) {
    /**
     * When DHT is sending data to MCU, every bit of data begins with the 50us
     * low-voltage-level and the length of the following high-voltage-level signal
     * determines whether data bit is "0" or "1"
     */
    if(dht_signal_transition(DHT11_SIGNAL_LOW, DHT11_SIGNAL_RESPONSE_PHASE1_DURATION) == -1) {
      return DHT11_STATUS_TIMEOUT;
    }

    /*
     * Save in array and calculate later.
     * Should not spend time calculating in the loop else the bit bang timing gets lost
     * Use bit 0 and bit 1 duration summed up to improve timming
     */
    data_signal_duration[i] = dht_signal_duration(DHT11_SIGNAL_HIGH,
                                                  DHT11_SIGNAL_RESPONSE_BIT_0_DURATION
                                                  + DHT11_SIGNAL_RESPONSE_BIT_1_DURATION);
    if(data_signal_duration[i] == -1) {
      return DHT11_STATUS_TIMEOUT;
    }
  }

  memset(dht.data, 0, sizeof(uint8_t) * DHT11_DATA_SIZE);
  for(j = 0, i = 0; i < DHT11_DATA_SAMPLES; i++) {

    /**
     * 26-28us voltage-length means data "0"
     * 70us voltage-length means 1 bit data "1"
     */
    if(data_signal_duration[i] >= DHT11_SIGNAL_RESPONSE_BIT_0_DURATION + DHT11_US_GUARD) {
      dht.data[j] = (dht.data[j] << 1) | 1;
    } else {
      dht.data[j] = dht.data[j] << 1;
    }

    /* Next byte */
    if(i % 8 == 7U) {
      j++;
    }
  }

  /* Verify  checksum */
  if(!dht_verify_checksum()) {
    return DHT11_STATUS_CHECKSUM_FAILED;
  } else {
    return DHT11_STATUS_OKAY;
  }
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch(type) {
  case DHT11_VALUE_HUMIDITY_INTEGER:
    return dht11_humidity_integer();
  case DHT11_VALUE_HUMIDITY_DECIMAL:
    return dht11_humidity_decimal();
  case DHT11_VALUE_TEMPERATURE_INTEGER:
    return dht11_temperature_integer();
  case DHT11_VALUE_TEMPERATURE_DECIMAL:
    return dht11_temperature_decimal();
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  (void)type;

  return dht.status;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  switch(type) {
  case DHT11_CONFIGURE_GPIO_PORT:
    dht.port = c;
    break;
  case DHT11_CONFIGURE_GPIO_PIN:
    dht.pin = c;
    break;
  case SENSORS_HW_INIT:
    dht.last_read = 0;
  case SENSORS_ACTIVE:
    if(c == 1) {
      clock_time_t now;

      now = clock_seconds();
      if(now - dht.last_read < DHT11_SAMPLING_RATE_SECONDS) {
        return 0;
      }
      dht.last_read = now;
      dht.status = dht_read();
    }
  case SENSORS_READY:
    break;
  default:
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(dht11_sensor, "dht11", value, configure, status);
/*----------------------------------------------------------------------------*/
/** @} */
