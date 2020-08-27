/*
 * Copyright (c) 2016, University of Bristol - http://www.bris.ac.uk/
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
#include "dev/aux-ctrl.h"
#include "lib/list.h"

#include "ti-lib.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/*---------------------------------------------------------------------------*/
LIST(consumers_list);
/*---------------------------------------------------------------------------*/
#ifdef THIS_DRIVERLIB_BUILD
#if THIS_DRIVERLIB_BUILD == DRIVERLIB_BUILD_CC13X2_CC26X2
#define TARGET_CC13X2_CC26X2    1
#endif
#endif

#ifdef TARGET_CC13X2_CC26X2


#define clock_ena(x)
static inline
void aux_wake() {
    ti_lib_aux_sysif_opmode_change(AUX_SYSIF_OPMODEREQ_REQ_A);
}

static inline
void aux_off() {
    /* Turn off AUX */
    ti_lib_aux_sysif_opmode_change(AUX_SYSIF_OPMODEREQ_REQ_PDA);
}

#else

// cc13/26x0 targets

static inline
void clock_ena(uint32_t clocks){
    ti_lib_aux_wuc_clock_enable(clocks);
    while(ti_lib_aux_wuc_clock_status(clocks) != AUX_WUC_CLOCK_READY);
}

static inline
void aux_wake() {
    ti_lib_aon_wuc_aux_wakeup_event(AONWUC_AUX_WAKEUP);
    while(!(ti_lib_aon_wuc_power_status_get() & AONWUC_AUX_POWER_ON));
}

static inline
void aux_off() {
    /* Turn off AUX */
    ti_lib_aon_wuc_aux_wakeup_event(AONWUC_AUX_ALLOW_SLEEP);
    ti_lib_aux_wuc_power_ctrl(AUX_WUC_POWER_OFF);
    while(ti_lib_aon_wuc_power_status_get() & AONWUC_AUX_POWER_ON);
}

#endif


void
aux_ctrl_register_consumer(aux_consumer_module_t *consumer)
{
  bool interrupts_disabled = ti_lib_int_master_disable();

  list_add(consumers_list, consumer);

  aux_ctrl_power_up();

  clock_ena(consumer->clocks);

  if(!interrupts_disabled) {
    ti_lib_int_master_enable();
  }
}
/*---------------------------------------------------------------------------*/
void
aux_ctrl_unregister_consumer(aux_consumer_module_t *consumer)
{
  bool interrupts_disabled = ti_lib_int_master_disable();

  list_remove(consumers_list, consumer);

  aux_ctrl_power_down(false);

  if(!interrupts_disabled) {
    ti_lib_int_master_enable();
  }
}
/*---------------------------------------------------------------------------*/
void
aux_ctrl_power_up()
{
  /* Don't if we have no consumers */
  if(list_head(consumers_list) == NULL) {
    return;
  }

  aux_wake();
}
/*---------------------------------------------------------------------------*/
void
aux_ctrl_power_down(bool force)
{
  aux_consumer_module_t *consumer;
  uint32_t clocks_in_use = 0;

  if(!force) {
    /* Visit all modules and release clocks */
    for(consumer = list_head(consumers_list); consumer != NULL;
        consumer = consumer->next) {
      clocks_in_use |= consumer->clocks;
    }

    /* If any clocks are still in use, AUX needs to stay powered and clocked */
    if(clocks_in_use) {
      ti_lib_aon_wuc_aux_power_down_config(AONWUC_CLOCK_SRC_LF);
      return;
    }
  }

  /* No clock for AUX in power down */
  ti_lib_aon_wuc_aux_power_down_config(AONWUC_NO_CLOCK);

  /* Disable retention */
  ti_lib_aon_wuc_aux_sram_config(false);

  aux_off();
}
/*---------------------------------------------------------------------------*/
