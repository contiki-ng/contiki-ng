/*
 * Copyright (c) 2018, George Oikonomou - http://www.spd.gr
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
#ifndef RGB_LED_H_
#define RGB_LED_H_
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup dev
 * @{
 */
/*---------------------------------------------------------------------------*/
/**
 * \defgroup rgb-led Generic RGB LED driver
 *
 * This is a driver for a tri-color RGB LED part, such as for example the
 * Broadcom (ex Avago Technologies) PLCC-4 Tricolor Black Surface LED present
 * on all Zolertia Zoul-based boards.
 *
 *
 * This driver sits on top of the LED HAL. Therefore, any port that wishes to
 * use this driver should first implement the GPIO HAL and the new LED API.
 * This driver will set the colour of the RGB LED by using combinations of
 * LED_RED, LED_GREEN and LED_BLUE. Therefore, those must be correctly defined
 * by the platform configuration.
 * @{
 *
 * \file
 * Header file for the RGB LED driver.
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define RGB_LED_RED     1
#define RGB_LED_GREEN   2
#define RGB_LED_BLUE    4
#define RGB_LED_MAGENTA (RGB_LED_RED | RGB_LED_BLUE)
#define RGB_LED_YELLOW  (RGB_LED_RED | RGB_LED_GREEN)
#define RGB_LED_CYAN    (RGB_LED_GREEN | RGB_LED_BLUE )
#define RGB_LED_WHITE   (RGB_LED_RED | RGB_LED_GREEN | RGB_LED_BLUE)
/*---------------------------------------------------------------------------*/
/**
 * \brief Turn off the RGB LED
 */
void rgb_led_off(void);

/**
 * \brief Set the colour of the RGB LED
 * \param colour The colour to set
 *
 * \e colour can take the value of one of the RGB_LED_xyz defines.
 */
void rgb_led_set(uint8_t colour);
/*---------------------------------------------------------------------------*/
#endif /* RGB_LED_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
