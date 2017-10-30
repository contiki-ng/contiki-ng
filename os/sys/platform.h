/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
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
 * \addtogroup sys
 * @{
 */
/*---------------------------------------------------------------------------*/
/**
 * \defgroup main The \os main function
 *
 * A platform-independent main function.
 *
 * \os provides a modular, platform-independent main function that aims to
 * support all hardware ports.
 *
 * In a nutshell, the main routine has the following steps below:
 *
 * - Calls a platform-provided function to initialise basic hardware drivers
 * - Initialises core \os modules, such as the process scheduler and timers
 * - Calls a platform-provided function to initialise more hardware drivers
 * - Initialises the network stack and TCP/IP
 * - Calls a platform-provided function to initialise any remaining drivers
 * - Enters the main loop
 *
 * The main loop will service all pending events and will then call a
 * platform-dependent function to put the hardware in a low-power state.
 *
 * For more detail of which hardware driver should be implemented in which
 * stage of the boot process, see the documentation of
 * \e platform_init_stage_one(), \e platform_init_stage_two() and
 * \e platform_init_stage_three()
 *
 * @{
 */
/*---------------------------------------------------------------------------*/
/**
 * \file
 *
 * Header file for the \os main routine
 */
/*---------------------------------------------------------------------------*/
#ifndef PLATFORM_H_
#define PLATFORM_H_
/*---------------------------------------------------------------------------*/
/**
 * Controls whether the platform provides a custom main loop
 *
 * By default we will use the main loop provided here. This however does not
 * work for some platforms, so we allow them to override it.
 */
#ifdef PLATFORM_CONF_PROVIDES_MAIN_LOOP
#define PLATFORM_PROVIDES_MAIN_LOOP PLATFORM_CONF_PROVIDES_MAIN_LOOP
#else
#define PLATFORM_PROVIDES_MAIN_LOOP 0
#endif
/*---------------------------------------------------------------------------*/
/**
 * Controls whether the main function accepts arguments
 *
 * By default our main does not accept arguments. However, when running on
 * native targets, command line arguments to main are required.
 */
#ifdef PLATFORM_CONF_MAIN_ACCEPTS_ARGS
#define PLATFORM_MAIN_ACCEPTS_ARGS PLATFORM_CONF_MAIN_ACCEPTS_ARGS
#else
#define PLATFORM_MAIN_ACCEPTS_ARGS 0
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Basic (Stage 1) platform driver initialisation.
 *
 * This function will get called early on in the \os boot sequence.
 *
 * In this function, the platform should initialise all core device drivers.
 * For example, this is where you will normally want to initialise hardware
 * timers/clocks, GPIO, LEDS. Normally this function will also enable the
 * MCU's global interrupt.
 *
 * The \os process scheduler, software clocks and timers will not be
 * running yet, so any platform drivers that rely on it should not be
 * initialised here. Instead, they should be initialised in
 * \e platform_init_stage_two() or in \e platform_init_stage_three()
 *
 * It is the port developer's responsibility to implement this function.
 *
 * \sa platform_init_stage_two()
 * \sa platform_init_stage_three()
 */
void platform_init_stage_one(void);
/*---------------------------------------------------------------------------*/
/**
 * \brief Stage 2 of platform driver initialisation.
 *
 * This function will be called by the \os boot sequence after parts of the
 * core have been initialised. More specifically, when this function gets
 * called, the following modules will be running:
 *
 * - Software clock
 * - Process scheduler
 * - Event timer (etimer)
 * - Callback timer (ctimer)
 * - rtimer
 * - Energest (if enabled)
 *
 * Therefore, any platform driver that relies on any of the above modules
 * should be initialised here or in \e platform_init_stage_three(),
 * but not in \e platform_init_stage_one()
 *
 * The \os network stack will not be running yet, so any platform drivers
 * that rely on networking should not be initialised here.
 *
 * When this function returns, the main routine will assume that the
 * platform has enabled character I/O and can print to console. When
 * this function returns, main() will attempt to initialise the network
 * stack. For this to work properly, this function should also populate
 * linkaddr_node_addr.
 *
 * It is the port developer's responsibility to implement this function.
 *
 * \sa platform_init_stage_one()
 * \sa platform_init_stage_three()
 */
void platform_init_stage_two(void);
/*---------------------------------------------------------------------------*/
/**
 * \brief Final stage of platform driver initialisation.
 *
 * Initialisation of platform-specific drivers that require networking to be
 * running. This is also a good place to initialise sensor drivers.
 *
 * When this function returns, the main routine will assume that the
 * hardware is fully initialised.
 *
 * It is the port developer's responsibility to implement this function.
 *
 * \sa platform_init_stage_one()
 * \sa platform_init_stage_two()
 */
void platform_init_stage_three(void);
/*---------------------------------------------------------------------------*/
/**
 * \brief The platform's idle/sleep function
 *
 * This function will be called as part of the main loop after all events
 * have been serviced. This is where you will normally put the device in a
 * low-power state. Waking up from this state and tidying up the hardware
 * is the port's responsibility.
 *
 * It is the port developer's responsibility to implement this function.
 */
void platform_idle(void);
/*---------------------------------------------------------------------------*/
/**
 * \brief The platform's main loop, if provided
 *
 * If the platform developer wishes to do so, it is possible to override the
 * main loop provided by \os's core. To do so, define
 * PLATFORM_CONF_PROVIDES_MAIN_LOOP as 1.
 *
 * It is the port developer's responsibility to implement this function.
 */
void platform_main_loop(void);
/*---------------------------------------------------------------------------*/
/**
 * \brief Allow the platform to process main's command line arguments
 *
 * If the platform wishes main() to accept arguments, then the \os main will
 * call this function here so that the platform can process/store those
 * arguments.
 *
 * This function will only get called if PLATFORM_MAIN_ACCEPTS_ARGS
 * is non-zero.
 *
 * It is the port developer's responsibility to implement this function.
 */
void platform_process_args(int argc, char**argv);
/*---------------------------------------------------------------------------*/
#endif /* PLATFORM_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
