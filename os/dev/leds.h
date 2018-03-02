/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
/**
 * \addtogroup dev
 * @{
 */
/*---------------------------------------------------------------------------*/
/**
 * \defgroup leds LED Hardware Abstraction Layer
 *
 * The LED HAL provides a set of functions that can manipulate LEDS.
 *
 * Currently, the LED HAL supports two APIs:
 *
 * - The new, platform-independent API (recommended for all new platforms).
 * - The legacy API (supported until all existing platforms have been ported
 *   to support the new API).
 *
 * The two APIs use very similar semantics and have an overlapping set of
 * function calls. This is done so that platform-independent examples can
 * work on all platforms, irrespective of which API each platform supports.
 *
 * The legacy API can be enabled by the platform code by defining
 * LEDS_CONF_LEGACY_API 1.
 *
 * Once all platforms supported in contiki-ng/contiki-ng have been ported to
 * the new API, the legacy API will be deleted without warning. For this
 * reason, it is strongly recommended to use the new API for new platforms and
 * for platforms hosted in external repositories.
 *
 * The new API provides a set of common LED manipulation functions that can be
 * used in a platform-independent fashion. Functions exist to manipulate one
 * LED at a time (\c leds_single_XYZ), as well as to manipulate multiple LEDs
 * at a time (\c leds_XYZ).
 *
 * The assumption is that each LED is connected to a GPIO pin using either
 * positive or negative logic.
 *
 * LEDs on a device are numbered incrementally, starting from 0 and counting
 * upwards. Thus, if a device has 3 LEDs they will be numbered 0, 1 and 2.
 * Convenience macros (LEDS_LED_n) are provided to refer to LEDs. These macros
 * can be used as arguments to functions that manipulate a single LED, such
 * as leds_single_on() but \e not leds_on().
 *
 * The legacy scheme that uses colours to refer to LEDs is maintained, without
 * semantic changes, but with minor changes in logic:
 *
 * - Firstly, we now define 5 LED colours: Red, Green, Blue, Yellow, Orange.
 *   These are sufficient to cover all currently-supported platforms.
 * - Secondly, unless a platform specifies that a LED of a specific colour
 *   exists, the HAL will assume that it does not.
 * - Trying to manipulate a non-existent LED will not cause build errors, but
 *   will not cause any changes to LED state either.
 * - We no longer map non-existing LED colours to existing ones.
 *
 * Note that, in order to avoid changes to LED colour semantics between the
 * two APIs, references to LED by colour are bitwise OR masks and should
 * therefore only be used as argument to functions that manipulate multiple
 * LEDS (e.g. leds_off() and \e not leds_single_off()).
 *
 * In terms of porting for new platforms, developers simply have to:
 *
 * - Define variables of type leds_t to represent their platform's LEDs
 * - Specify the number of LEDs on their device by defining LEDS_CONF_COUNT
 * - Map red colours to numbers (e.g. \#define LEDS_CONF_RED 1)
 *
 * \file
 *     Header file for the LED HAL
 * @{
 */
/*---------------------------------------------------------------------------*/
#ifndef LEDS_H_
#define LEDS_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/gpio-hal.h"

#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#if LEDS_CONF_LEGACY_API
/**
 * \brief Define to 1 to enabled the legacy LED API.
 */
#define LEDS_LEGACY_API LEDS_CONF_LEGACY_API
#else
#define LEDS_LEGACY_API 0
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief A default LED colour for non-existing LEDs
 */
#define LEDS_COLOUR_NONE 0x00
/*---------------------------------------------------------------------------*/
/* LED colour to number mappings. Applicable to both APIs */
#ifdef LEDS_CONF_RED
#define LEDS_RED LEDS_CONF_RED
#else
#define LEDS_RED LEDS_COLOUR_NONE
#endif

#ifdef LEDS_CONF_GREEN
#define LEDS_GREEN LEDS_CONF_GREEN
#else
#define LEDS_GREEN LEDS_COLOUR_NONE
#endif

#ifdef LEDS_CONF_BLUE
#define LEDS_BLUE LEDS_CONF_BLUE
#else
#define LEDS_BLUE LEDS_COLOUR_NONE
#endif

#ifdef LEDS_CONF_YELLOW
#define LEDS_YELLOW LEDS_CONF_YELLOW
#else
#define LEDS_YELLOW LEDS_COLOUR_NONE
#endif

#ifdef LEDS_CONF_ORANGE
#define LEDS_ORANGE LEDS_CONF_ORANGE
#else
#define LEDS_ORANGE LEDS_COLOUR_NONE
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief The LED number
 */
typedef uint8_t leds_num_t;

/**
 * \brief An OR mask datatype to represents multiple LEDs.
 */
typedef uint8_t leds_mask_t;
/*---------------------------------------------------------------------------*/
#if LEDS_LEGACY_API
/*---------------------------------------------------------------------------*/
#ifdef LEDS_CONF_ALL
#define LEDS_ALL    LEDS_CONF_ALL
#else
#define LEDS_ALL    7
#endif
/*---------------------------------------------------------------------------*/
void leds_blink(void);

/* Legacy LED API arch-specific functions */
void leds_arch_init(void);
leds_mask_t leds_arch_get(void);
void leds_arch_set(leds_mask_t leds);
/*---------------------------------------------------------------------------*/
#else /* LEDS_LEGACY_API */
/*---------------------------------------------------------------------------*/
#ifdef LEDS_CONF_COUNT
#define LEDS_COUNT LEDS_CONF_COUNT
#else
/**
 * \brief The number of LEDs present on a device
 */
#define LEDS_COUNT 0
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief The OR mask representation of all device LEDs
 */
#define LEDS_ALL ((1 << LEDS_COUNT) - 1)
/*---------------------------------------------------------------------------*/
#endif /* LEDS_LEGACY_API */
/*---------------------------------------------------------------------------*/
#define LEDS_LED1     0x00 /**< Convenience macro to refer to the 1st LED (LED 1) */
#define LEDS_LED2     0x01 /**< Convenience macro to refer to the 2nd LED (LED 2) */
#define LEDS_LED3     0x02 /**< Convenience macro to refer to the 3rd LED (LED 3) */
#define LEDS_LED4     0x03 /**< Convenience macro to refer to the 4th LED (LED 4) */
#define LEDS_LED5     0x04 /**< Convenience macro to refer to the 5th LED (LED 5) */
/*---------------------------------------------------------------------------*/
/**
 * \brief A LED logical representation
 *
 * \e pin corresponds to the GPIO pin a LED is driven by, using GPIO HAL pin
 * representation.
 *
 * \e negative_logic should be set to false if the LED is active low.
 */
typedef struct leds_s {
  gpio_hal_pin_t pin;
  bool negative_logic;
} leds_t;
/*---------------------------------------------------------------------------*/
/**
 * \brief Convert a LED number to a mask representation
 * \param l The pin number (normally a variable of type leds_num_t)
 * \return An OR mask of type leds_mask_t
 */
#define LEDS_NUM_TO_MASK(l) (1 << (l))
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialise the LED HAL
 *
 * This function will set corresponding LED GPIO pins to output and will also
 * set the initial state of all LEDs to off.
 */
void leds_init(void);

/**
 * \brief Turn a single LED on
 * \param led The led
 *
 * The \e led argument should be the LED's number, in other words one of the
 * LED_Ln macros.
 *
 * This function will not change the state of other LEDs.
 */
void leds_single_on(leds_num_t led);

/**
 * \brief Turn a single LED off
 * \param led The led
 *
 * The \e led argument should be the LED's number, in other words one of the
 * LED_Ln macros.
 *
 * This function will not change the state of other LEDs.
 */
void leds_single_off(leds_num_t led);

/**
 * \brief Toggle a single LED
 * \param led The led
 *
 * The \e led argument should be the LED's number, in other words one of the
 * LED_Ln macros.
 *
 * This function will not change the state of other LEDs.
 */
void leds_single_toggle(leds_num_t led);

/**
 * \brief Turn on multiple LEDs
 * \param leds The leds to be turned on as an OR mask
 *
 * The \e led argument should be a bitwise mask of the LEDs to be changed.
 * For example, to turn on LEDs 1 and 3, you should pass
 * LED_NUM_TO_MASK(LED_L1) | LED_NUM_TO_MASK(LED_L3) = 1 | 4 = 5
 *
 * This function will not change the state of other LEDs.
 */
void leds_on(leds_mask_t leds);

/**
 * \brief Turn off multiple LEDs
 * \param leds The leds to be turned off as an OR mask
 *
 * The \e led argument should be a bitwise mask of the LEDs to be changed.
 * For example, to turn on LEDs 1 and 3, you should pass
 * LED_NUM_TO_MASK(LED_L1) | LED_NUM_TO_MASK(LED_L3) = 1 | 4 = 5
 *
 * This function will not change the state of other LEDs.
 */
void leds_off(leds_mask_t leds);

/**
 * \brief Toggle multiple LEDs
 * \param leds The leds to be toggled as an OR mask
 *
 * The \e led argument should be a bitwise mask of the LEDs to be changed.
 * For example, to turn on LEDs 1 and 3, you should pass
 * LED_NUM_TO_MASK(LED_L1) | LED_NUM_TO_MASK(LED_L3) = 1 | 4 = 5
 *
 * This function will not change the state of other LEDs.
 */
void leds_toggle(leds_mask_t leds);

/**
 * \brief Set all LEDs to a specific state
 * \param leds The state of all LEDs afer this function returns
 *
 * The \e led argument should be a bitwise mask of the LEDs to be changed.
 * For example, to turn on LEDs 1 and 3, you should pass
 * LED_NUM_TO_MASK(LED_L1) | LED_NUM_TO_MASK(LED_L3) = 1 | 4 = 5
 *
 * This function will change the state of all LEDs. LEDs not set in the \e leds
 * mask will be turned off.
 */
void leds_set(leds_mask_t leds);

/**
 * \brief Get the status of LEDs
 * \return A bitwise mask indicating whether each individual LED is on or off
 *
 * The return value is a bitwise mask. If a bit is set then the corresponding
 * LED is on.
 */
leds_mask_t leds_get(void);
/*---------------------------------------------------------------------------*/
#endif /* LEDS_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
