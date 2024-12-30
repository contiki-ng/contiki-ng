/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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

#ifndef SIMENVCHANGE_H_
#define SIMENVCHANGE_H_

#include "contiki.h"

#define COOJA_VIB_INIT_PRIO 150
#define COOJA_MOTEID_INIT_PRIO 160
#define COOJA_RS232_INIT_PRIO 170
#define COOJA_RADIO_INIT_PRIO 180
#define COOJA_BUTTON_INIT_PRIO 190
#define COOJA_PIR_INIT_PRIO 200
#define COOJA_IP_INIT_PRIO 210

struct cooja_tick_action {
  struct cooja_tick_action *next;
  void (*action)(void);
};

#define COOJA_POST_TICK_ACTION(prio, handler) \
  COOJA_TICK_ACTION(post, (prio), (handler))

#define COOJA_PRE_TICK_ACTION(prio, handler) \
  COOJA_TICK_ACTION(pre, (prio), (handler))

#define COOJA_TICK_ACTION(kind, prio, handler) \
  static struct cooja_tick_action kind##_tick_action = { NULL, (handler) }; \
  CC_CONSTRUCTOR((prio)) static void \
  add_##kind##_tick_action(void) \
  { \
    void cooja_add_##kind##_tick_action(struct cooja_tick_action *handler); \
    cooja_add_##kind##_tick_action(&kind##_tick_action); \
  }

// Variable for keeping the last process_run() return value
extern int simProcessRunValue;
extern int simEtimerPending;
extern clock_time_t simEtimerNextExpirationTime;
extern clock_time_t simCurrentTime;

// Variable that when set to != 0, stops the mote from falling asleep next tick
extern char simDontFallAsleep;

#endif /* SIMENVCHANGE_H_ */
