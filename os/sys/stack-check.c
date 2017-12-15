/*
 * Copyright (c) 2017, University of Bristol - http://www.bris.ac.uk/
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
 * \addtogroup stack
 * @{
 */

/**
 * \file
 *     Implementation of the stack checker library.
 * \author
 *     Atis Elsts <atis.elsts@bristol.ac.uk>
 */

#include "contiki.h"
#include "sys/stack-check.h"
#include "dev/watchdog.h"
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "Stack"
#define LOG_LEVEL LOG_LEVEL_MAIN

/*---------------------------------------------------------------------------*/
/* linker will provide a symbol for the end of the .bss segment */
extern uint8_t _stack;

#if STACK_CHECK_PERIODIC_CHECKS
PROCESS(stack_check_process, "Stack check");
#endif
/*---------------------------------------------------------------------------*/
/* The symbol with which the stack memory is initially filled */
#define STACK_FILL 0xcd
/*---------------------------------------------------------------------------*/
#ifdef STACK_ORIGIN
/* use the #defined value */
#define GET_STACK_ORIGIN() STACK_ORIGIN
#else
/* use the value provided by the linker script */
extern int _stack_origin;
#define GET_STACK_ORIGIN() (&_stack_origin)
#endif
/*---------------------------------------------------------------------------*/
void
stack_check_init(void)
{
  uint8_t *p;

  /* Make this static to avoid destroying it in the while loop */
  static void *stack_top;
  /* Use address of this local variable as a boundary */
  stack_top = &p;

  /* Note: this is expected to be called before the WDT is started! */
  p = &_stack;
  while(p < (uint8_t *)stack_top) {
    *p++ = STACK_FILL;
  }

#if STACK_CHECK_PERIODIC_CHECKS
  /* Start the periodic checker process */
  process_start(&stack_check_process, NULL);
#endif
}
/*---------------------------------------------------------------------------*/
uint16_t
stack_check_get_usage(void)
{
  uint8_t *p = &_stack;

  /* Make sure WDT is not triggered */
  watchdog_periodic();

  /* Skip the bytes used after heap; it's 1 byte by default for _stack,
   * more than that means dynamic memory allocation is used somewhere.
   */
  while(*p != STACK_FILL && p < (uint8_t *)GET_STACK_ORIGIN()) {
    p++;
  }

  /* Skip the region of the memory reserved for the stack not used yet by the program */
  while(*p == STACK_FILL && p < (uint8_t *)GET_STACK_ORIGIN()) {
    p++;
  }

  /* Make sure WDT is not triggered */
  watchdog_periodic();

  if(p >= (uint8_t*)GET_STACK_ORIGIN()) {
    /* This means the stack is screwed. */
    return 0xffff;
  }

  return (uint8_t *)GET_STACK_ORIGIN() - p;
}
/*---------------------------------------------------------------------------*/
uint16_t
stack_check_get_reserved_size(void)
{
  return (uint8_t *)GET_STACK_ORIGIN() - &_stack;
}
/*---------------------------------------------------------------------------*/
#if STACK_CHECK_PERIODIC_CHECKS
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(stack_check_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  etimer_set(&et, STACK_CHECK_PERIOD);

  while(1) {
    uint16_t actual, allowed;

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    actual = stack_check_get_usage();
    allowed = stack_check_get_reserved_size();
    if(actual > allowed) {
      LOG_ERR("Check failed: %u vs. %u\n", actual, allowed);
    } else {
      LOG_DBG("Check ok: %u vs. %u\n", actual, allowed);
    }

    etimer_reset(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
#endif /* STACK_CHECK_PERIODIC_CHECKS */
/*---------------------------------------------------------------------------*/
/** @} */
