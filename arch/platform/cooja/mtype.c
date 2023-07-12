/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * \file
 *         COOJA Contiki mote type file.
 * \author
 *         Fredrik Osterlind <fros@sics.se>
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "sys/cc.h"
#include "sys/cooja_mt.h"

/* The main function, implemented in contiki-main.c */
int main(void);

/*
 * referenceVar is used for comparing absolute and process relative memory.
 * (this must not be static due to memory locations)
 */
intptr_t referenceVar;

/*
 * Contiki and rtimer threads.
 */
static struct cooja_mt_thread rtimer_thread;
static struct cooja_mt_thread process_run_thread;
/*---------------------------------------------------------------------------*/
static void
rtimer_thread_loop(void *data)
{
  while(1) {
    rtimer_arch_check();

    /* Return to COOJA */
    cooja_mt_yield();
  }
}
/*---------------------------------------------------------------------------*/
static void
process_run_thread_loop(void *data)
{
  /* Yield once during bootup */
  simProcessRunValue = 1;
  cooja_mt_yield();

  /* Then call common Contiki-NG main function */
  main();
}
/*---------------------------------------------------------------------------*/
void
cooja_init(void)
{
  /* Create rtimers and Contiki threads */
  cooja_mt_start(&rtimer_thread, &rtimer_thread_loop, NULL);
  cooja_mt_start(&process_run_thread, &process_run_thread_loop, NULL);
}
/*---------------------------------------------------------------------------*/
void
cooja_tick(void)
{
  simProcessRunValue = 0;

  /* Let all simulation interfaces act first */
  doActionsBeforeTick();

  /* Poll etimer process */
  if(etimer_pending()) {
    etimer_request_poll();
  }

  /* Let rtimers run.
   * Sets simProcessRunValue */
  cooja_mt_exec(&rtimer_thread);

  if(simProcessRunValue == 0) {
    /* Rtimers done: Let Contiki handle a few events.
     * Sets simProcessRunValue */
    cooja_mt_exec(&process_run_thread);
  }

  /* Let all simulation interfaces act before returning to java */
  doActionsAfterTick();

  /* Do we have any pending timers */
  simEtimerPending = etimer_pending();

  /* Save nearest expiration time */
  simEtimerNextExpirationTime = etimer_next_expiration_time();
}
