/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
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
#include "dev/leds.h"
#include "dev/gpio-hal.h"
#include "dev/oscillators.h"
#include "dev/soc-rtc.h"
#include "dev/cc26xx-uart.h"
#include "dev/button-hal.h"

#include "ti-lib.h"

#include <stdio.h>
/*---------------------------------------------------------------------------*/
/** \brief Board specific iniatialisation */
void board_init(void);
/*---------------------------------------------------------------------------*/
#ifdef BOARD_CONF_HAS_SENSORS
#define BOARD_HAS_SENSORS BOARD_CONF_HAS_SENSORS
#else
#define BOARD_HAS_SENSORS 1
#endif
/*---------------------------------------------------------------------------*/
void
platform_init_stage_one()
{
  /* Enable flash cache and prefetch. */
  ti_lib_vims_mode_set(VIMS_BASE, VIMS_MODE_ENABLED);
  ti_lib_vims_configure(VIMS_BASE, true, true);

  ti_lib_int_master_disable();

  /* Set the LF XOSC as the LF system clock source */
  oscillators_select_lf_xosc();

  board_init();

//  gpio_hal_init();

//  leds_init();
//  leds_on(LEDS_RED);

  /*
   * Disable I/O pad sleep mode and open I/O latches in the AON IOC interface
   * This is only relevant when returning from shutdown (which is what froze
   * latches in the first place. Before doing these things though, we should
   * allow software to first regain control of pins
   */
  ti_lib_pwr_ctrl_io_freeze_disable();

  ti_lib_rom_int_enable(INT_AON_GPIO_EDGE);
  ti_lib_int_master_enable();

  soc_rtc_init();
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two()
{
  /* Character I/O Initialisation */
#if CC26XX_UART_CONF_ENABLE
  cc26xx_uart_init();
#endif

  button_hal_init();
}
/*---------------------------------------------------------------------------*/
