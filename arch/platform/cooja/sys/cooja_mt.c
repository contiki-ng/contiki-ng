/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
/*
 * This file is ripped from mt.c of the Contiki Multi-threading library.
 * Fredrik Osterlind <fros@sics.se>
 */

#ifdef __APPLE__
#define _DARWIN_C_SOURCE /* For pthread_get_stackaddr_np() */
#else
#define _GNU_SOURCE /* For pthread_getattr_np(). */
#endif
#include <pthread.h>

#include "sys/cooja_mt.h"

#include <signal.h>
#include <stdio.h>

#define MT_STATE_READY   1
#define MT_STATE_RUNNING 2
#define MT_STATE_EXITED  5

static struct cooja_mt_thread *current;
static struct cooja_mt_thread *cooja_running_thread;

#ifdef __APPLE__
/* Avoid deprecated warnings for ucontext.h functions on OS X. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

/*--------------------------------------------------------------------------*/
int
cooja_mt_init(struct cooja_mt_thread *t)
{
#ifdef __APPLE__
  /* Apple's makecontext() fails if size is less than MINSIGSTKSZ. */
  if(COOJA_MTARCH_STACKSIZE < MINSIGSTKSZ) {
    return -1;
  }
#endif
  void *stack_addr;
  size_t stack_size;
  pthread_t self = pthread_self();
#ifdef __APPLE__
  stack_addr = pthread_get_stackaddr_np(self);
  stack_size = pthread_get_stacksize_np(self);
#else
  pthread_attr_t attrs;
  if(pthread_getattr_np(self, &attrs) != 0) {
    return -2;
  }
  if(pthread_attr_getstack(&attrs, &stack_addr, &stack_size) != 0) {
    return -3;
  }
#endif
  if(getcontext(&t->ctxt) == -1) {
    perror("getcontext failed");
    return -4;
  }
  t->ctxt.uc_stack.ss_sp = stack_addr;
  t->ctxt.uc_stack.ss_flags = 0;
  t->ctxt.uc_stack.ss_size = stack_size;
  return 0;
}
/*--------------------------------------------------------------------------*/
void
cooja_mt_start(struct cooja_mt_thread *caller,
               struct cooja_mt_thread *t, void (*function)(void))
{
  if(getcontext(&t->ctxt) == -1) {
    perror("getcontext failed");
    return;
  }
  t->ctxt.uc_stack.ss_sp = t->stack;
  t->ctxt.uc_stack.ss_flags = 0;
  t->ctxt.uc_stack.ss_size = sizeof(t->stack);
  t->ctxt.uc_link = &caller->ctxt;
  makecontext(&t->ctxt, function, 0);
  t->state = MT_STATE_READY;
}
/*--------------------------------------------------------------------------*/
void
cooja_mt_exec(struct cooja_mt_thread *caller, struct cooja_mt_thread *thread)
{
  if(thread->state == MT_STATE_READY) {
    thread->state = MT_STATE_RUNNING;
    current = thread;
    /* Switch context to the thread. The function call will not return
       until the the thread has yielded, or is preempted. */
    cooja_running_thread = thread;
    if(swapcontext(&caller->ctxt, &thread->ctxt) == -1) {
      perror("swapcontext");
    }
    cooja_running_thread = NULL;
  }
}
/*--------------------------------------------------------------------------*/
void
cooja_mt_yield(void)
{
  current->state = MT_STATE_READY;
  current = NULL;
  /* This function is called from the running thread, and we call the
     switch function in order to switch the thread to the main Contiki
     program instead. For us, the switch function will not return
     until the next time we are scheduled to run. */
  if(cooja_running_thread->ctxt.uc_link == NULL) {
    return;
  }
  if(swapcontext(&cooja_running_thread->ctxt,
                 cooja_running_thread->ctxt.uc_link) == -1) {
    perror("swapcontext");
  }
}

#ifdef __APPLE__
#pragma GCC diagnostic pop
#endif
