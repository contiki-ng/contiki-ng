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
 * \addtogroup dev
 * @{
 *
 * \defgroup dht11-sensor DHT 11 Sensor driver
 * @{
 *
 * \file
 *      DHT 11 sensor header file
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

#ifndef DHT11_SENSOR_H_
#define DHT11_SENSOR_H_

#include "sensors.h"

extern const struct sensors_sensor dht11_sensor;

/**
 * @brief DHT11 Configuration type for GPIO Port
 *
 */
#define DHT11_CONFIGURE_GPIO_PORT   (0)

/**
 * @brief DHT11 Configuration type for GPIO Pin
 *
 */
#define DHT11_CONFIGURE_GPIO_PIN    (1)

/**
 * @brief DHT11 value type for humidity integer part
 *
 */
#define DHT11_VALUE_HUMIDITY_INTEGER       (0)

/**
 * @brief DHT11 value type for humidity decimal part
 *
 */
#define DHT11_VALUE_HUMIDITY_DECIMAL       (1)

/**
 * @brief DHT11 value type for temperature integer part
 *
 */
#define DHT11_VALUE_TEMPERATURE_INTEGER    (2)

/**
 * @brief DHT11 value type for temperature decimal part
 *
 */
#define DHT11_VALUE_TEMPERATURE_DECIMAL    (3)

/**
 * @brief DHT11 status okay
 *
 */
#define DHT11_STATUS_OKAY               (0)

/**
 * @brief DHT11 status timeout
 *
 */
#define DHT11_STATUS_TIMEOUT            (1)

/**
 * @brief DHT11 status checksum failed
 *
 */
#define DHT11_STATUS_CHECKSUM_FAILED    (2)

#endif /* DHT11_SENSOR_H_ */

/**
 * @}
 * @}
 */
