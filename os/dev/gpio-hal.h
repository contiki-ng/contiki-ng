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
/**
 * \addtogroup dev
 * @{
 *
 * \defgroup gpio-hal GPIO Hardware Abstraction Layer
 *
 * The GPIO HAL provides a set of common functions that can be used in a
 * platform-independent fashion.
 *
 * Internally, the GPIO HAL handles edge detection handling and also provides
 * fallback functions for GPIO pin toggling if the hardware does not have
 * a direct method of toggling pins through direct register access.
 *
 * @{
 *
 * \file
 *     Header file for the GPIO HAL
 */
/*---------------------------------------------------------------------------*/
#ifndef GPIO_HAL_H_
#define GPIO_HAL_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
/**
 * \brief Specifies whether the HAL should support a port/pin convention
 *
 * Some MCUs specify GPIOs as a port/pin combination, whereas some others
 * only use a pin number. Our GPIO HAL supports both conventions in a portable
 * fashion and this define is used to set the HAL in the desired of the two
 * modes.
 *
 * The port developer should define GPIO_HAL_CONF_PORT_PIN_NUMBERING as a if
 * the platform uses port/pin numbering, or to 0 if the platform only uses
 * a simple number.
 */
#ifdef GPIO_HAL_CONF_PORT_PIN_NUMBERING
#define GPIO_HAL_PORT_PIN_NUMBERING GPIO_HAL_CONF_PORT_PIN_NUMBERING
#else
#define GPIO_HAL_PORT_PIN_NUMBERING 1
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Specifies whether software-based pin toggle is required
 *
 * Some MCUs allow GPIO pin toggling via direct register access. For these
 * MCUs, define GPIO_HAL_CONF_ARCH_SW_TOGGLE to 0 and then implement
 * gpio_hal_arch_toggle_pin() and gpio_hal_arch_toggle_pins()
 *
 * \sa gpio_hal_arch_toggle_pin()
 * \sa gpio_hal_arch_toggle_pins()
 */
#ifdef GPIO_HAL_CONF_ARCH_SW_TOGGLE
#define GPIO_HAL_ARCH_SW_TOGGLE GPIO_HAL_CONF_ARCH_SW_TOGGLE
#else
#define GPIO_HAL_ARCH_SW_TOGGLE 1
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Convenience macro to use this as the port argument of macros
 *
 * Use this as the port \e argument of macros when GPIO_HAL_PORT_PIN_NUMBERING
 * is zero
 */
#define GPIO_HAL_NULL_PORT      0
/*---------------------------------------------------------------------------*/
/**
 * \brief GPIO pin number representation
 */
typedef uint8_t gpio_hal_pin_t;

/**
 * \brief A data structure that represents ports.
 *
 * This is only relevant if GPIO_HAL_PORT_PIN_NUMBERING is non-zero
 */
typedef uint8_t gpio_hal_port_t;

/**
 * \brief GPIO pin configuration
 *
 * A logical representation of a pin's configuration. It is an OR combination
 * of GPIO_HAL_PIN_CFG_xyz macros.
 */
typedef uint32_t gpio_hal_pin_cfg_t;

/**
 * \brief Specifies the total number of pins on a device
 *
 * This macro has no effect if GPIO_HAL_PORT_PIN_NUMBERING is non-zero.
 */
#ifdef GPIO_HAL_CONF_PIN_COUNT
#define GPIO_HAL_PIN_COUNT GPIO_HAL_CONF_PIN_COUNT
#else
#define GPIO_HAL_PIN_COUNT 32
#endif

#if GPIO_HAL_PIN_COUNT > 32 && !GPIO_HAL_PORT_PIN_NUMBERING
typedef uint64_t gpio_hal_pin_mask_t;
#else
/**
 * \brief GPIO pin mask representation
 *
 * A mask that can be used to represent multiple pins using a single variable.
 *
 * When GPIO_HAL_PORT_PIN_NUMBERING is non-zero, such variables can only be
 * used to represent pins within the same port.
 */
typedef uint32_t gpio_hal_pin_mask_t;
#endif
/*---------------------------------------------------------------------------*/
#if GPIO_HAL_PORT_PIN_NUMBERING
typedef void (*gpio_hal_callback_t)(gpio_hal_port_t port,
                                    gpio_hal_pin_mask_t pin_mask);
#else
typedef void (*gpio_hal_callback_t)(gpio_hal_pin_mask_t pin_mask);
#endif
/*---------------------------------------------------------------------------*/
#define GPIO_HAL_PIN_CFG_PULL_NONE        0x00
#define GPIO_HAL_PIN_CFG_PULL_UP          0x01
#define GPIO_HAL_PIN_CFG_PULL_DOWN        0x02
#define GPIO_HAL_PIN_CFG_PULL_MASK        (GPIO_HAL_PIN_CFG_PULL_UP | \
                                           GPIO_HAL_PIN_CFG_PULL_DOWN)

#define GPIO_HAL_PIN_CFG_HYSTERESIS       0x10

#define GPIO_HAL_PIN_CFG_EDGE_NONE        0x00
#define GPIO_HAL_PIN_CFG_EDGE_RISING      0x04
#define GPIO_HAL_PIN_CFG_EDGE_FALLING     0x08
#define GPIO_HAL_PIN_CFG_EDGE_BOTH        (GPIO_HAL_PIN_CFG_EDGE_RISING | \
                                           GPIO_HAL_PIN_CFG_EDGE_FALLING)

#define GPIO_HAL_PIN_CFG_INT_DISABLE      0x00
#define GPIO_HAL_PIN_CFG_INT_ENABLE       0x80
#define GPIO_HAL_PIN_CFG_INT_MASK         0x80
/*---------------------------------------------------------------------------*/
/**
 * \brief Datatype for GPIO event handlers
 *
 * A GPIO event handler is a function that gets called whenever a pin triggers
 * an event. The same handler can be registered to handle events for more than
 * one pin by setting the respective pin's position but in \e pin_mask.
 *
 * If GPIO_HAL_PORT_PIN_NUMBERING is non-zero, a separate handler is required
 * per port.
 */
typedef struct gpio_hal_event_handler_s {
  struct gpio_hal_event_handler_s *next;
  gpio_hal_callback_t handler;
#if GPIO_HAL_PORT_PIN_NUMBERING
  gpio_hal_port_t port;
#endif
  gpio_hal_pin_mask_t pin_mask;
} gpio_hal_event_handler_t;
/*---------------------------------------------------------------------------*/
/**
 * \brief Unknown GPIO
 *
 * A default GPIO value for unknown GPIO
 */
#define GPIO_HAL_PIN_UNKNOWN              0xFF
/*---------------------------------------------------------------------------*/
/**
 * \name Core GPIO functions
 *
 * Functions implemented by the HAL itself
 * @{
 */
/**
 * \brief Initialise the GPIO HAL
 */
void gpio_hal_init(void);

/**
 * \brief Register a function to be called whenever a pin triggers an event
 * \param handler The handler representation
 *
 * The handler must be pre-allocated statically by the caller.
 *
 * This function can be used to register a function to be called by the HAL
 * whenever a GPIO interrupt occurs.
 *
 * \sa gpio_hal_event_handler
 */
void gpio_hal_register_handler(gpio_hal_event_handler_t *handler);

#if GPIO_HAL_PORT_PIN_NUMBERING
/**
 * \brief The platform-independent GPIO event handler
 * \param port The GPIO port, if applicable
 * \param pins OR mask of pins that generated an event
 *
 * Whenever a GPIO input interrupt occurs (edge or level detection) and an ISR
 * is triggered, the ISR must call this function, passing as argument an ORd
 * mask of the pins that triggered the interrupt. This function will then
 * call the registered event handlers (if any) for the pins that triggered the
 * event. The platform code should make no assumptions as to the order that
 * the handlers will be called.
 *
 * If a pin set in the mask has an event handler registered, this function
 * will call the registered handler.
 *
 * If GPIO_HAL_PORT_PIN_NUMBERING is non-zero the function will also accept
 * as its first argument the port associated to the pins that triggered the
 * edge detection.
 *
 * This function will not clear any CPU interrupt flags, this should be done
 * by the calling ISR.
 *
 * \sa gpio_hal_register_handler
 */
void gpio_hal_event_handler(gpio_hal_port_t port, gpio_hal_pin_mask_t pins);
#else
void gpio_hal_event_handler(gpio_hal_pin_mask_t pins);
#endif

/**
 * \brief Convert a pin to a pin mask
 * \param pin The pin
 * \return The corresponding mask
 */
#define gpio_hal_pin_to_mask(pin) ((gpio_hal_pin_mask_t)1 << (pin))
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name GPIO pin manipulation functions to be provided by the platform code.
 *
 * All functions have two flavours:
 * - gpio_hal_arch_port_foo are used when GPIO_HAL_PORT_PIN_NUMBERING is
 *   non-zero and expect a gpio_hal_port_t as one of their arguments
 * - gpio_hal_arch_no_port_foo are used when GPIO_HAL_PORT_PIN_NUMBERING is 0
 *   and do _not_ expect a gpio_hal_port_t as one of their arguments
 *
 * Macros are provided that automatically expand to the desirable prototype
 * depending on the value of GPIO_HAL_PORT_PIN_NUMBERING. In order to achieve
 * code portability, all platform-independent code should use those macros to
 * manipulate GPIOs instead of using the port_ / no_port_ functions directly.
 * A convenience macro GPIO_HAL_NULL_PORT is provided to be used as the port
 * argument of macros when GPIO_HAL_PORT_PIN_NUMBERING is zero.
 *
 * All the functions below must be provided by the platform's developer. The
 * HAL offers the developer a number of options of how to provide the required
 * functionality.
 *
 * - The developer can provide a symbol. For example, the developer can create
 *   a .c file and implement a function called gpio_hal_arch_set_port_pin().
 *   In this scenario the developer only needs to provide a symbol for the
 *   gpio_hal_arch_port_foo / gpio_hal_arch_no_port_foo that applies.
 *
 * - The developer can provide a function-like macro that has the same name as
 *   one of the manipulation macros declared here. In this scenario, the
 *   declaration here will be removed by the pre-processor. For example, the
 *   developer can do something like:
 *
 *   \code
 *   #define gpio_hal_arch_write_pin(port, pin, v) sdk_function(port, pin, v)
 *   \endcode
 *
 * - The developer can provide a static inline implementation. For this to
 *   work, the developer can do something like:
 *
 *   \code
 *   #define gpio_hal_arch_set_pin(port, pin) set_pin(port, pin)
 *   static inline void set_pin(gpio_hal_port_t port, gpio_hal_pin_t pin) { ... }
 *   \endcode
 *
 * In the latter two cases, the developer will likely provide implementations
 * in a header file. In this scenario, one of the platform's configuration
 * files must define GPIO_HAL_CONF_ARCH_HDR_PATH to the name of this header
 * file. For example:
 *
 * \code
 * #define GPIO_HAL_CONF_ARCH_HDR_PATH          "dev/gpio-hal-arch.h"
 * \endcode
 * @{
 */
/*---------------------------------------------------------------------------*/
/* Include Arch-Specific conf */
#ifdef GPIO_HAL_CONF_ARCH_HDR_PATH
#include GPIO_HAL_CONF_ARCH_HDR_PATH
#endif /* GPIO_HAL_CONF_ARCH_HDR_PATH */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_init
/**
 * \brief Perform architecture specific gpio initaliaztion
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 */
void gpio_hal_arch_init(void);
#endif
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_interrupt_enable
/**
 * \brief Enable interrupts for a gpio pin
 * \param port The GPIO port
 * \param pin The GPIO pin number
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_interrupt_enable(gpio_hal_port_t port,
                                         gpio_hal_pin_t pin);

/**
 * \brief Enable interrupts for a gpio pin
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_interrupt_enable(gpio_hal_pin_t pin);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_interrupt_enable(port, pin) \
  gpio_hal_arch_port_interrupt_enable(port, pin)
#else
#define gpio_hal_arch_interrupt_enable(port, pin) \
  gpio_hal_arch_no_port_interrupt_enable(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_interrupt_enable */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_interrupt_disable
/**
 * \brief Disable interrupts for a gpio pin
 * \param port The GPIO port
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_interrupt_disable(gpio_hal_port_t port,
                                          gpio_hal_pin_t pin);

/**
 * \brief Disable interrupts for a gpio pin
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_interrupt_disable(gpio_hal_pin_t pin);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_interrupt_disable(port, pin) \
  gpio_hal_arch_port_interrupt_disable(port, pin)
#else
#define gpio_hal_arch_interrupt_disable(port, pin) \
  gpio_hal_arch_no_port_interrupt_disable(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_interrupt_disable */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_pin_cfg_set
/**
 * \brief Configure a gpio pin
 * \param port The GPIO port
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 * \param cfg The configuration
 *
 * \e cfg is an OR mask of GPIO_HAL_PIN_CFG_xyz
 *
 * The implementation of this function also has to make sure that \e pin is
 * configured as software-controlled GPIO.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_pin_cfg_set(gpio_hal_port_t port,
                                    gpio_hal_pin_t pin,
                                    gpio_hal_pin_cfg_t cfg);

/**
 * \brief Configure a gpio pin
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 * \param cfg The configuration
 *
 * \e cfg is an OR mask of GPIO_HAL_PIN_CFG_xyz
 *
 * The implementation of this function also has to make sure that \e pin is
 * configured as software-controlled GPIO.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_pin_cfg_set(gpio_hal_pin_t pin,
                                       gpio_hal_pin_cfg_t cfg);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_pin_cfg_set(port, pin, cfg) \
  gpio_hal_arch_port_pin_cfg_set(port, pin, cfg)
#else
#define gpio_hal_arch_pin_cfg_set(port, pin, cfg) \
  gpio_hal_arch_no_port_pin_cfg_set(pin, cfg)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_pin_cfg_set */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_pin_cfg_get
/**
 * \brief Read the configuration of a GPIO pin
 * \param port The GPIO port
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 * \return An OR mask of GPIO_HAL_PIN_CFG_xyz
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
gpio_hal_pin_cfg_t gpio_hal_arch_port_pin_cfg_get(gpio_hal_port_t port,
                                                  gpio_hal_pin_t pin);

/**
 * \brief Read the configuration of a GPIO pin
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 * \return An OR mask of GPIO_HAL_PIN_CFG_xyz
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
gpio_hal_pin_cfg_t gpio_hal_arch_no_port_pin_cfg_get(gpio_hal_pin_t pin);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_pin_cfg_get(port, pin) \
  gpio_hal_arch_port_pin_cfg_get(port, pin)
#else
#define gpio_hal_arch_pin_cfg_get(port, pin) \
  gpio_hal_arch_no_port_pin_cfg_get(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_pin_cfg_get */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_pin_set_input
/**
 * \brief Configure a pin as GPIO input
 * \param port The GPIO port
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * The implementation of this function also has to make sure that \e pin is
 * configured as software-controlled GPIO.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_pin_set_input(gpio_hal_port_t port,
                                      gpio_hal_pin_t pin);

/**
 * \brief Configure a pin as GPIO input
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * The implementation of this function also has to make sure that \e pin is
 * configured as software-controlled GPIO.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_pin_set_input(gpio_hal_pin_t pin);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_pin_set_input(port, pin) \
  gpio_hal_arch_port_pin_set_input(port, pin)
#else
#define gpio_hal_arch_pin_set_input(port, pin) \
  gpio_hal_arch_no_port_pin_set_input(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_pin_set_input */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_pin_set_output
/**
 * \brief Configure a pin as GPIO output
 * \param port The GPIO port
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * The implementation of this function also has to make sure that \e pin is
 * configured as software-controlled GPIO.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_pin_set_output(gpio_hal_port_t port,
                                       gpio_hal_pin_t pin);

/**
 * \brief Configure a pin as GPIO output
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * The implementation of this function also has to make sure that \e pin is
 * configured as software-controlled GPIO.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_pin_set_output(gpio_hal_pin_t pin);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_pin_set_output(port, pin) \
  gpio_hal_arch_port_pin_set_output(port, pin)
#else
#define gpio_hal_arch_pin_set_output(port, pin) \
  gpio_hal_arch_no_port_pin_set_output(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_pin_set_output */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_set_pin
/**
 * \brief Set a GPIO pin to logical high
 * \param port The GPIO port
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_set_pin(gpio_hal_port_t port, gpio_hal_pin_t pin);

/**
 * \brief Set a GPIO pin to logical high
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_set_pin(gpio_hal_pin_t pin);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_set_pin(port, pin) \
  gpio_hal_arch_port_set_pin(port, pin)
#else
#define gpio_hal_arch_set_pin(port, pin) \
  gpio_hal_arch_no_port_set_pin(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_set_pin */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_clear_pin
/**
 * \brief Clear a GPIO pin (logical low)
 * \param port The GPIO port
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_clear_pin(gpio_hal_port_t port, gpio_hal_pin_t pin);

/**
 * \brief Clear a GPIO pin (logical low)
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_clear_pin(gpio_hal_pin_t pin);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_clear_pin(port, pin) \
  gpio_hal_arch_port_clear_pin(port, pin)
#else
#define gpio_hal_arch_clear_pin(port, pin) \
  gpio_hal_arch_no_port_clear_pin(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_clear_pin */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_toggle_pin
/**
 * \brief Toggle a GPIO pin
 * \param port The GPIO port
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * Some MCUs allow GPIO pin toggling directly via register access. In this
 * case, it is a good idea to provide an implementation of this function.
 * However, a default, software-based implementation is also provided by the
 * HAL and can be used if the MCU does not have a pin toggle register. To use
 * the HAL function, define GPIO_HAL_ARCH_SW_TOGGLE as 1. To provide your own
 * implementation, define GPIO_HAL_ARCH_SW_TOGGLE as 0.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_toggle_pin(gpio_hal_port_t port, gpio_hal_pin_t pin);

/**
 * \brief Toggle a GPIO pin
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 *
 * Some MCUs allow GPIO pin toggling directly via register access. In this
 * case, it is a good idea to provide an implementation of this function.
 * However, a default, software-based implementation is also provided by the
 * HAL and can be used if the MCU does not have a pin toggle register. To use
 * the HAL function, define GPIO_HAL_ARCH_SW_TOGGLE as 1. To provide your own
 * implementation, define GPIO_HAL_ARCH_SW_TOGGLE as 0.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_toggle_pin(gpio_hal_pin_t pin);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_toggle_pin(port, pin) \
  gpio_hal_arch_port_toggle_pin(port, pin)
#else
#define gpio_hal_arch_toggle_pin(port, pin) \
  gpio_hal_arch_no_port_toggle_pin(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_toggle_pin */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_read_pin
/**
 * \brief Read a GPIO pin
 * \param port The GPIO port
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 * \retval 0 The pin is logical low
 * \retval 1 The pin is logical high
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
uint8_t gpio_hal_arch_port_read_pin(gpio_hal_port_t port, gpio_hal_pin_t pin);

/**
 * \brief Read a GPIO pin
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 * \retval 0 The pin is logical low
 * \retval 1 The pin is logical high
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
uint8_t gpio_hal_arch_no_port_read_pin(gpio_hal_pin_t pin);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_read_pin(port, pin) \
  gpio_hal_arch_port_read_pin(port, pin)
#else
#define gpio_hal_arch_read_pin(port, pin) \
  gpio_hal_arch_no_port_read_pin(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_read_pin */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_write_pin
/**
 * \brief Write a GPIO pin
 * \param port The GPIO port
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 * \param value 0: Logical low; 1: Logical high
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_write_pin(gpio_hal_port_t port,
                                  gpio_hal_pin_t pin,
                                  uint8_t value);

/**
 * \brief Write a GPIO pin
 * \param pin The GPIO pin number (0...GPIO_HAL_PIN_COUNT - 1)
 * \param value 0: Logical low; 1: Logical high
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_write_pin(gpio_hal_pin_t pin, uint8_t value);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_write_pin(port, pin, value) \
  gpio_hal_arch_port_write_pin(port, pin, value)
#else
#define gpio_hal_arch_write_pin(port, pin, value) \
  gpio_hal_arch_no_port_write_pin(pin, value)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_write_pin */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_set_pins
/**
 * \brief Set multiple pins to logical high
 * \param port The GPIO port
 * \param pins An ORd pin mask of the pins to set
 *
 * A pin will be set to logical high if its position in \e pins is set. For
 * example you can set pins 0 and 3 by passing 0x09.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_set_pins(gpio_hal_port_t port,
                                 gpio_hal_pin_mask_t pins);

/**
 * \brief Set multiple pins to logical high
 * \param pins An ORd pin mask of the pins to set
 *
 * A pin will be set to logical high if its position in \e pins is set. For
 * example you can set pins 0 and 3 by passing 0x09.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_set_pins(gpio_hal_pin_mask_t pins);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_set_pins(port, pin) \
  gpio_hal_arch_port_set_pins(port, pin)
#else
#define gpio_hal_arch_set_pins(port, pin) \
  gpio_hal_arch_no_port_set_pins(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_set_pins */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_clear_pins
/**
 * \brief Clear multiple pins to logical low
 * \param port The GPIO port
 * \param pins An ORd pin mask of the pins to clear
 *
 * A pin will be set to logical low if its position in \e pins is set. For
 * example you can clear pins 0 and 3 by passing 0x09.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_clear_pins(gpio_hal_port_t port,
                                   gpio_hal_pin_mask_t pins);

/**
 * \brief Clear multiple pins to logical low
 * \param pins An ORd pin mask of the pins to clear
 *
 * A pin will be set to logical low if its position in \e pins is set. For
 * example you can clear pins 0 and 3 by passing 0x09.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_clear_pins(gpio_hal_pin_mask_t pins);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_clear_pins(port, pin) \
  gpio_hal_arch_port_clear_pins(port, pin)
#else
#define gpio_hal_arch_clear_pins(port, pin) \
  gpio_hal_arch_no_port_clear_pins(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_clear_pins */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_toggle_pins
/**
 * \brief Toggle multiple pins
 * \param port The GPIO port
 * \param pins An ORd pin mask of the pins to toggle
 *
 * A pin will be toggled if its position in \e pins is set. For example you
 * can toggle pins 0 and 3 by passing 0x09.
 *
 * Some MCUs allow GPIO pin toggling directly via register access. In this
 * case, it is a good idea to provide an implementation of this function.
 * However, a default, software-based implementation is also provided by the
 * HAL and can be used if the MCU does not have a pin toggle register. To use
 * the HAL function, define GPIO_HAL_ARCH_SW_TOGGLE as 1. To provide your own
 * implementation, define GPIO_HAL_ARCH_SW_TOGGLE as 0.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_toggle_pins(gpio_hal_port_t port,
                                    gpio_hal_pin_mask_t pins);

/**
 * \brief Toggle multiple pins
 * \param pins An ORd pin mask of the pins to toggle
 *
 * A pin will be toggled if its position in \e pins is set. For example you
 * can toggle pins 0 and 3 by passing 0x09.
 *
 * Some MCUs allow GPIO pin toggling directly via register access. In this
 * case, it is a good idea to provide an implementation of this function.
 * However, a default, software-based implementation is also provided by the
 * HAL and can be used if the MCU does not have a pin toggle register. To use
 * the HAL function, define GPIO_HAL_ARCH_SW_TOGGLE as 1. To provide your own
 * implementation, define GPIO_HAL_ARCH_SW_TOGGLE as 0.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_toggle_pins(gpio_hal_pin_mask_t pins);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_toggle_pins(port, pin) \
  gpio_hal_arch_port_toggle_pins(port, pin)
#else
#define gpio_hal_arch_toggle_pins(port, pin) \
  gpio_hal_arch_no_port_toggle_pins(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_toggle_pins */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_read_pins
/**
 * \brief Read multiple pins
 * \param port The GPIO port
 * \param pins An ORd pin mask of the pins to read
 * \retval An ORd mask of the pins that are high
 *
 * If the position of the pin in \e pins is set and the pin is logical high
 * then the position of the pin in the return value will be set. For example,
 * if you pass 0x09 as the value of \e pins and the return value is 0x08 then
 * pin 3 is logical high and pin 0 is logical low.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
gpio_hal_pin_mask_t gpio_hal_arch_port_read_pins(gpio_hal_port_t port,
                                                 gpio_hal_pin_mask_t pins);

/**
 * \brief Read multiple pins
 * \param pins An ORd pin mask of the pins to read
 * \retval An ORd mask of the pins that are high
 *
 * If the position of the pin in \e pins is set and the pin is logical high
 * then the position of the pin in the return value will be set. For example,
 * if you pass 0x09 as the value of \e pins and the return value is 0x08 then
 * pin 3 is logical high and pin 0 is logical low.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
gpio_hal_pin_mask_t gpio_hal_arch_no_port_read_pins(gpio_hal_pin_mask_t pins);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_read_pins(port, pin) \
  gpio_hal_arch_port_read_pins(port, pin)
#else
#define gpio_hal_arch_read_pins(port, pin) \
  gpio_hal_arch_no_port_read_pins(pin)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_read_pins */
/*---------------------------------------------------------------------------*/
#ifndef gpio_hal_arch_write_pins
/**
 * \brief Write multiple pins
 * \param port The GPIO port
 * \param pins An ORd pin mask of the pins to write
 * \param value An ORd mask of the value to write
 *
 * The function will modify GPIO pins that have their position in the mask set.
 * pins, the function will write the value specified in the corresponding
 * position in \e value.

 * For example, you can set pin 3 and clear pin 0 by a single call to this
 * function. To achieve this, \e pins must be 0x09 and \e value 0x08.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * There is no guarantee that this function will result in an atomic operation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_port_write_pins(gpio_hal_port_t port,
                                   gpio_hal_pin_mask_t pins,
                                   gpio_hal_pin_mask_t value);

/**
 * \brief Write multiple pins
 * \param pins An ORd pin mask of the pins to write
 * \param value An ORd mask of the value to write
 *
 * The function will modify GPIO pins that have their position in the mask set.
 * pins, the function will write the value specified in the corresponding
 * position in \e value.

 * For example, you can set pin 3 and clear pin 0 by a single call to this
 * function. To achieve this, \e pins must be 0x09 and \e value 0x08.
 *
 * It is the platform developer's responsibility to provide an implementation.
 *
 * There is no guarantee that this function will result in an atomic operation.
 *
 * The implementation can be provided as a global symbol, an inline function
 * or a function-like macro, as described above.
 *
 * \note Code should not call this function directly. Use GPIO manipulation
 * macros instead.
 */
void gpio_hal_arch_no_port_write_pins(gpio_hal_pin_mask_t pins,
                                      gpio_hal_pin_mask_t value);

#if GPIO_HAL_PORT_PIN_NUMBERING
#define gpio_hal_arch_write_pins(port, pin, value) \
  gpio_hal_arch_port_write_pins(port, pin, value)
#else
#define gpio_hal_arch_write_pins(port, pin, value) \
  gpio_hal_arch_no_port_write_pins(pin, value)
#endif /* GPIO_HAL_PORT_PIN_NUMBERING */
#endif /* gpio_hal_arch_write_pins */
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* GPIO_HAL_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
