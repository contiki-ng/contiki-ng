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
#include "lib/list.h"
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
 * Interface handlers.
 */
LIST(pre_tick_actions);
LIST(post_tick_actions);
/*---------------------------------------------------------------------------*/
void
cooja_add_pre_tick_action(struct cooja_tick_action *handler)
{
  /* Constructor order is per module on macOS, so init list here instead. */
  static bool initialized = false;
  if(!initialized) {
    list_init(pre_tick_actions);
    initialized = true;
  }
  list_add(pre_tick_actions, handler);
}
/*---------------------------------------------------------------------------*/
void
cooja_add_post_tick_action(struct cooja_tick_action *handler)
{
  /* Constructor order is per module on macOS, so init list here instead. */
  static bool initialized = false;
  if(!initialized) {
    list_init(post_tick_actions);
    initialized = true;
  }
  list_add(post_tick_actions, handler);
}
/*---------------------------------------------------------------------------*/
/*
 * Contiki and rtimer threads.
 */
static struct cooja_mt_thread cooja_thread;
static struct cooja_mt_thread rtimer_thread;
static struct cooja_mt_thread process_run_thread;
/*---------------------------------------------------------------------------*/
#ifdef __APPLE__
extern int macos_data_start __asm("section$start$__DATA$__data");
extern int macos_data_end __asm("section$end$__DATA$__data");
extern int macos_bss_start __asm("section$start$__DATA$__bss");
extern int macos_bss_end __asm("section$end$__DATA$__bss");
extern int macos_common_start __asm("section$start$__DATA$__common");
extern int macos_common_end __asm("section$end$__DATA$__common");

uintptr_t
cooja_data_start(void)
{
  return (uintptr_t)&macos_data_start;
}

int
cooja_data_size(void)
{
  return (int)((uintptr_t)&macos_data_end - (uintptr_t)&macos_data_start);
}

uintptr_t
cooja_bss_start(void)
{
  return (uintptr_t)&macos_bss_start;
}

int
cooja_bss_size(void)
{
  return (int)((uintptr_t)&macos_bss_end - (uintptr_t)&macos_bss_start);
}

uintptr_t
cooja_common_start(void)
{
  return (uintptr_t)&macos_common_start;
}

int
cooja_common_size(void)
{
  return (int)((uintptr_t)&macos_common_end - (uintptr_t)&macos_common_start);
}
#endif /* __APPLE__ */
/*---------------------------------------------------------------------------*/
static void
rtimer_thread_loop(void)
{
  while(1) {
    rtimer_arch_check();

    /* Return to COOJA */
    cooja_mt_yield();
  }
}
/*---------------------------------------------------------------------------*/
static void
process_run_thread_loop(void)
{
  /* Yield once during bootup */
  simProcessRunValue = 1;
  cooja_mt_yield();
  /* Then call common Contiki-NG main function */
  main();
}
/*---------------------------------------------------------------------------*/
int
cooja_init(void)
{
  int rv;
  /* Create rtimers and Contiki threads */
  if((rv = cooja_mt_init(&cooja_thread))) {
    return rv;
  }
  cooja_mt_start(&cooja_thread, &rtimer_thread, rtimer_thread_loop);
  cooja_mt_start(&cooja_thread, &process_run_thread, process_run_thread_loop);
  return 0;
}
/*---------------------------------------------------------------------------*/
void
cooja_tick(void)
{
  simProcessRunValue = 0;

  /* Let all simulation interfaces act first */
  for(struct cooja_tick_action *r = list_head(pre_tick_actions);
      r != NULL; r = r->next) {
    r->action();
  }

  /* Poll etimer process */
  if(etimer_pending()) {
    etimer_request_poll();
  }

  /* Let rtimers run.
   * Sets simProcessRunValue */
  cooja_mt_exec(&cooja_thread, &rtimer_thread);

  if(simProcessRunValue == 0) {
    /* Rtimers done: Let Contiki handle a few events.
     * Sets simProcessRunValue */
    cooja_mt_exec(&cooja_thread, &process_run_thread);
  }

  /* Let all simulation interfaces act before returning to java */
  for(struct cooja_tick_action *r = list_head(post_tick_actions);
      r != NULL; r = r->next) {
    r->action();
  }

  /* Do we have any pending timers */
  simEtimerPending = etimer_pending();

  /* Save nearest expiration time */
  simEtimerNextExpirationTime = etimer_next_expiration_time();
}
