/*
 * Copyright (c) 2020, George Oikonomou - https://spd.gr
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
#include "contiki.h"
#include "dev/gpio-hal.h"
/*---------------------------------------------------------------------------*/
/*
 * LEDs on the nRF52840DK (PCA10056) are connected to pins P0.13...P0.16
 *
 * LEDs on the dongle (PCA10059) are connected as follows:
 * - LED1 --> P0.6
 * - LED2 --> P0.8
 * - LED3 --> P1.9
 * - LED4 --> P0.12
 *
 * Here we'll use LEDs 3, 1 and 2 - in that order such that out_port2_3 will
 * be port 0.
 */
#if CONTIKI_BOARD_DK
gpio_hal_port_t out_port1 = 0;

gpio_hal_pin_t out_pin1 = 15;
gpio_hal_pin_t out_pin2 = 13;
gpio_hal_pin_t out_pin3 = 14;
#else
gpio_hal_port_t out_port1 = 1;

gpio_hal_pin_t out_pin1 = 9;
gpio_hal_pin_t out_pin2 = 6;
gpio_hal_pin_t out_pin3 = 8;
#endif

gpio_hal_port_t out_port2_3 = 0;
/*---------------------------------------------------------------------------*/
#if CONTIKI_BOARD_DK
gpio_hal_port_t btn_port = 0;
gpio_hal_pin_t btn_pin = 11;
#else
gpio_hal_port_t btn_port = 1;
gpio_hal_pin_t btn_pin = 6;
#endif
/*---------------------------------------------------------------------------*/
