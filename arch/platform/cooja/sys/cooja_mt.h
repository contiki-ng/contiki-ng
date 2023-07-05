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
 * This file is ripped from mt.h of the Contiki Multi-threading library.
 * Fredrik Osterlind <fros@sics.se>
 */
#ifndef COOJA_MT_H_
#define COOJA_MT_H_

#ifndef _XOPEN_SOURCE
/* Enable POSIX.1-2001, required on OS X. */
#define _XOPEN_SOURCE 600
#endif
#include <ucontext.h>

#ifndef COOJA_MTARCH_STACKSIZE
#ifdef __APPLE__
#define COOJA_MTARCH_STACKSIZE 32768
#else
#define COOJA_MTARCH_STACKSIZE 8192
#endif
#endif /* COOJA_MTARCH_STACKSIZE */

struct cooja_mt_thread {
  char stack[COOJA_MTARCH_STACKSIZE];
  ucontext_t ctxt;
  int state;
};

/**
 * Inintializes the main thread structure.
 *
 * \param thread Pointer to the (implicit) main thread.
 */
int cooja_mt_init(struct cooja_mt_thread *thread);

/**
 * Starts a multithreading thread.
 *
 * \param caller Pointer to a struct for the calling thread.
 *
 * \param thread Pointer to an mt_thread struct that must have been
 * previously allocated by the caller.
 *
 * \param function A pointer to the entry function of the thread that is
 * to be set up.
 */
void cooja_mt_start(struct cooja_mt_thread *caller,
                    struct cooja_mt_thread *thread, void (*function)(void));

/**
 * Execute parts of a thread.
 *
 * This function is called by a Contiki process and runs a
 * thread. The function does not return until the thread has yielded,
 * or is preempted.
 *
 * \note The thread must first be initialized with the cooja_mt_start() function.
 *
 * \param caller Pointer to a struct for the calling thread.
 *
 * \param thread A pointer to a struct mt_thread block that must be
 * allocated by the caller.
 *
 */
void cooja_mt_exec(struct cooja_mt_thread *caller,
                   struct cooja_mt_thread *thread);

/**
 * Voluntarily give up the processor.
 *
 * This function is called by a running thread in order to give up
 * control of the CPU.
 *
 */
void cooja_mt_yield(void);

#endif /* MT_H_ */
