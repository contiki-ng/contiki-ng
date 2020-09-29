/*
 * Copyright (c) 2020, Oppila Microsystems - http://www.oppila.in
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
 * \addtogroup oppila-sensors
 * @{
 *
 * \defgroup oppila-adc-sensors OMote adc wrapper to use analogue sensors
 *
 * The driver allows to reuse the adc-wrapper implementation and
 * add sensors easily, without duplicating code, providing also a simplified
 * interface and exposing the available ADC assigned channels by a given
 * platform.
 * @{
 *
 * \file
 * Header file for the OMote ADC sensors API
 */
/*---------------------------------------------------------------------------*/
#ifndef ADC_SENSORS_H_
#define ADC_SENSORS_H_
/*---------------------------------------------------------------------------*/
#include "lib/sensors.h"
#include "dev/soc-adc.h"
#include "dev/adc-omote.h"
/*---------------------------------------------------------------------------*/
#define ADC_WRAPPER_SUCCESS                 0x00
#define ADC_WRAPPER_ERROR                   (-1)
#define ADC_WRAPPER_EXTERNAL_VREF           5000
#define ADC_WRAPPER_EXTERNAL_VREF_CROSSVAL  3000
/*---------------------------------------------------------------------------*/
#define ANALOG_GROVE_LIGHT                  0x01
/* -------------------------------------------------------------------------- */
#define ADC_SENSORS "ADC sensors API"
/* -------------------------------------------------------------------------- */
extern const struct sensors_sensor adc_sensors;
/*---------------------------------------------------------------------------*/
#endif /* ADC_SENSORS_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

