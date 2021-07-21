/* Copyright (c) 2015, Zolertia - http://www.zolertia.com
 * Copyright (c) 2015, University of Bristol - http://www.bristol.ac.uk
 * 
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
 *    contributors may be used to endorse or prowinos products derived
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
 * \addtogroup owinos-sensors
 * @{
 *
 * \defgroup owinos-adc-interface Owinos Generic ADC interface
 *
 * Driver for the owinos ADC interface
 * @{
 *
 * \file
 * Header file for the Owinos ADC interface
 */
/*---------------------------------------------------------------------------*/
#ifndef ADC_OWINOS_H_
#define ADC_OWINOS_H_
/*---------------------------------------------------------------------------*/
#include "lib/sensors.h"
#include "dev/soc-adc.h"
/*---------------------------------------------------------------------------*/
/**
 * \name Generic ADC sensors
 * @{
 */
#define ADC_OWINOS "ADC sensor interface"
#define ADC_SENSORS_PORT_BASE    GPIO_PORT_TO_BASE(ADC_SENSORS_PORT)

#ifdef ADC_SENSORS_CONF_REFERENCE
#define ADC_SENSORS_REFERENCE ADC_SENSORS_CONF_REFERENCE
#else
#define ADC_SENSORS_REFERENCE SOC_ADC_ADCCON_REF_AVDD5
#endif

/*
 * PA0-PA1 are hardcoded to UART0 and PA3 is hardcoded to BSL button
 */
#define OWINOS_SENSORS_ADC_MIN     2  /**< PA1 pin mask */

/* ADC1 is connected with LDR sensor*/
#if ADC_SENSORS_ADC1_PIN >= OWINOS_SENSORS_ADC_MIN
#define OWINOS_SENSORS_ADC1        GPIO_PIN_MASK(ADC_SENSORS_ADC1_PIN)
#else
#define OWINOS_SENSORS_ADC1        0
#endif
/* ADC2 */
#if ADC_SENSORS_ADC2_PIN >= OWINOS_SENSORS_ADC_MIN
#define OWINOS_SENSORS_ADC2        GPIO_PIN_MASK(ADC_SENSORS_ADC2_PIN)
#else
#define OWINOS_SENSORS_ADC2        0
#endif
/* ADC3 */
#if ADC_SENSORS_ADC3_PIN >= OWINOS_SENSORS_ADC_MIN
#define OWINOS_SENSORS_ADC3        GPIO_PIN_MASK(ADC_SENSORS_ADC3_PIN)
#else
#define OWINOS_SENSORS_ADC3        0
#endif
/* ADC4 */
#if ADC_SENSORS_ADC4_PIN >= OWINOS_SENSORS_ADC_MIN
#define OWINOS_SENSORS_ADC4        GPIO_PIN_MASK(ADC_SENSORS_ADC4_PIN)
#else
#define OWINOS_SENSORS_ADC4        0
#endif
/* ADC5 */
#if ADC_SENSORS_ADC5_PIN >= OWINOS_SENSORS_ADC_MIN
#define OWINOS_SENSORS_ADC5        GPIO_PIN_MASK(ADC_SENSORS_ADC5_PIN)
#else
#define OWINOS_SENSORS_ADC5        0
#endif
/*
 * This is safe as the disabled sensors should have a zero value thus not
 * affecting the mask operations
 */
#define OWINOS_SENSORS_ADC_ALL     (OWINOS_SENSORS_ADC1 + OWINOS_SENSORS_ADC2 + \
                                  OWINOS_SENSORS_ADC3 + OWINOS_SENSORS_ADC4 + \
                                  OWINOS_SENSORS_ADC5 )
/** @} */
/*---------------------------------------------------------------------------*/
extern const struct sensors_sensor adc_owinos;
/*---------------------------------------------------------------------------*/
#endif /* ADC_OWINOS_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

