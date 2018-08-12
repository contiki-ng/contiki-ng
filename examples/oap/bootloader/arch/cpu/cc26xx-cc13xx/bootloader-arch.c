/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
 * Copyright (c) 2016, Mark Solters <msolters@gmail.com>
 * Copyright (c) 2018, George Oikonomou - http://www.spd.gr
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
/*---------------------------------------------------------------------------*/
void
bootloader_arch_jump_to_app()
{
  /* Load the address of the vector to R0 */
  __asm(" MOV R0, #0x2000");

  /* Load the address of the Reset Handler to R1:
   * Offset by 0x04 from the vector start */
  __asm(" LDR R1, [R0, #0x4]");

  /* Reset the stack pointer */
  __asm(" LDR SP, [R0, #0x0]");

  /* Make sure we are in thumb mode after the jump */
  __asm(" ORR R1, #1");

  /* And jump */
  __asm(" BX R1");
}
/*---------------------------------------------------------------------------*/
void board_init(void);
/*---------------------------------------------------------------------------*/
void
bootloader_arch_init()
{
  /* Enable flash cache and prefetch. */
  ti_lib_vims_mode_set(VIMS_BASE, VIMS_MODE_ENABLED);
  ti_lib_vims_configure(VIMS_BASE, true, true);

  ti_lib_int_master_disable();

  /* Set the LF XOSC as the LF system clock source */
  ti_lib_osc_clock_source_set(OSC_SRC_CLK_LF, OSC_XOSC_LF);

  /* Wait for LF clock source to become XOSC_LF */
  while(ti_lib_osc_clock_source_get(OSC_SRC_CLK_LF) != OSC_XOSC_LF);

  board_init();

  gpio_hal_init();

  leds_init();
  leds_on(LEDS_RED);

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
