/*
 * Copyright (C) 2024 Marcel Graber <marcel@clever.design>
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
/**
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-sys System drivers
 * @{
 *
 * \file
 *         Minimal Low Power Mode (LPM) driver for the nRF54L15.
 *
 *         The RTC-based LPM framework in arch/cpu/nrf/sys/lpm.c relies on the
 *         legacy RTC peripheral and NRF_TIMER0, neither of which exist on the
 *         nRF54L15 (it uses the GRTC instead). This implementation provides the
 *         lpm_init()/lpm_drop() entry points expected by the platform code with
 *         a simple WFI-based idle.
 *
 * \author
 *         Marcel Graber <marcel@clever.design>
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lpm.h"

#include "sys/energest.h"
#include "sys/critical.h"
#include "sys/process.h"

#include "nrf.h"
/*---------------------------------------------------------------------------*/
/* Defined in arch/cpu/nrf/nrf54l15/clock-arch.c */
extern void clock_arch_check_and_recover(void);
/*---------------------------------------------------------------------------*/
void
lpm_init(void)
{
}
/*---------------------------------------------------------------------------*/
void
lpm_drop(void)
{
  int_master_status_t status;
  int abort;

  status = critical_enter();
  abort = process_nevents();
  if(!abort) {
    ENERGEST_SWITCH(ENERGEST_TYPE_CPU, ENERGEST_TYPE_LPM);
    /*
     * After a soft reset on the nRF54L15, GRTC interrupts may not wake the CPU
     * from WFI. Spin instead until that is resolved.
     */
    for(volatile int i = 0; i < 1000; i++) {
      __NOP();
    }
    ENERGEST_SWITCH(ENERGEST_TYPE_LPM, ENERGEST_TYPE_CPU);
  }
  critical_exit(status);
  clock_arch_check_and_recover();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
