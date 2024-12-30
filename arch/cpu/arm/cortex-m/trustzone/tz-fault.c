/*
 * Copyright (c) 2023, RISE Research Institutes of Sweden
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
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

/*
 * \file
 *      ARMv8-M fault handling.
 * \author
 *      Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 *      Niclas Finne <niclas.finne@ri.se>
 */

#include "contiki.h"
#include "dev/watchdog.h"

#include <arm_cmse.h>

/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "SecureFault"
#define LOG_LEVEL LOG_LEVEL_INFO
/*---------------------------------------------------------------------------*/
/* Magic value to check for initialization */
#define FAULT_MAGIC 0x12345678
struct fault_info {
  uint32_t magic;
  uint32_t sfsr;
  uint32_t sfar;
};
__attribute__((section(".noinit"))) volatile struct fault_info fault_info;
/*---------------------------------------------------------------------------*/
static void
print_sfsr(uint32_t sfsr)
{
  if(sfsr & SAU_SFSR_LSERR_Msk) {
    LOG_WARN_(" LSERR");
  }
  if(sfsr & SAU_SFSR_SFARVALID_Msk) {
    LOG_WARN_(" SFARVALID");
  }
  if(sfsr & SAU_SFSR_LSPERR_Msk) {
    LOG_WARN_(" LSPERR");
  }
  if(sfsr & SAU_SFSR_INVTRAN_Msk) {
    LOG_WARN_(" INVTRAN");
  }
  if(sfsr & SAU_SFSR_AUVIOL_Msk) {
    LOG_WARN_(" AUVIOL");
  }
  if(sfsr & SAU_SFSR_INVER_Msk) {
    LOG_WARN_(" INVER");
  }
  if(sfsr & SAU_SFSR_INVIS_Msk) {
    LOG_WARN_(" INVIS");
  }
  if(sfsr & SAU_SFSR_INVEP_Msk) {
    LOG_WARN_(" INVEP");
  }
}
/*---------------------------------------------------------------------------*/
void
SecureFault_Handler(void)
{
  fault_info.magic = FAULT_MAGIC;
  fault_info.sfar = SAU->SFAR;
  fault_info.sfsr = SAU->SFSR;
  NVIC_SystemReset();
}
/*---------------------------------------------------------------------------*/
void
tz_fault_init(void)
{
  if(fault_info.magic == FAULT_MAGIC) {
    fault_info.magic = 0;

    LOG_WARN("Reboot caused by Secure Fault! Address 0x%"PRIx32
             ", SFSR 0x%"PRIx32"\n",
             fault_info.sfar, fault_info.sfsr);
    LOG_WARN("Secure Fault status:");
    print_sfsr(fault_info.sfsr);
    LOG_WARN_("\n");
  }
}
/*---------------------------------------------------------------------------*/
