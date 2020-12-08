/*
 * Copyright (c) 2015, Nordic Semiconductor
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * \addtogroup nrf52840-dev Device drivers
 * @{
 *
 * \addtogroup nrf52840-lpm Low power mode functions
 * @{
 *
 * \file
 *         A header file for low power mode functions.
 * \author
 *         Wojciech Bober <wojciech.bober@nordicsemi.no>
 */
#ifndef LPM_H
#define LPM_H

#include "sys/int-master.h"
#include "sys/critical.h"
#include "sys/process.h"
#include "sys/energest.h"

/**
 * \brief Stop and wait for an event
 *
 */
static inline void
lpm_drop(void)
{
  int_master_status_t status;
  int abort;
  status = critical_enter();
  abort = process_nevents();
  if(!abort) {
    ENERGEST_SWITCH(ENERGEST_TYPE_CPU, ENERGEST_TYPE_LPM);
    __WFI();
    ENERGEST_SWITCH(ENERGEST_TYPE_LPM, ENERGEST_TYPE_CPU);
  }
  critical_exit(status);
}
#endif /* LPM_H_ */
/**
 * @}
 * @}
 */
