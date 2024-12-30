/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 */

/**
 * \addtogroup process
 * @{
 */

/**
 * \file
 *         Implementation of the Contiki process kernel.
 * \author
 *         Adam Dunkels <adam@sics.se>
 *
 */

#include <stdio.h>

#include "contiki.h"
#include "sys/process.h"

#include "sys/log.h"
#define LOG_MODULE "Process"
#define LOG_LEVEL LOG_LEVEL_SYS

/*
 * The process_num_events_t type is an uint8_t. It must be able to store
 * the value of PROCESS_CONF_NUMEVENTS + 1, for the additional
 * boolean value indicating whether a poll has been requested.
 */
static_assert(PROCESS_CONF_NUMEVENTS > 0 && PROCESS_CONF_NUMEVENTS <= 128,
  "PROCESS_CONF_NUMEVENTS must be a positive value of at most 128.");

/* Require that PROCESS_CONF_NUMEVENTS is a power of 2 to allow
   optimization of modulo operations. */
static_assert(!(PROCESS_CONF_NUMEVENTS & (PROCESS_CONF_NUMEVENTS - 1)),
  "PROCESS_CONF_NUMEVENTS must be a power of 2.");

/*
 * A configurable function called after a process poll been requested.
 */
#ifdef PROCESS_CONF_POLL_REQUESTED
#define PROCESS_POLL_REQUESTED PROCESS_CONF_POLL_REQUESTED
void PROCESS_POLL_REQUESTED(void);
#else
#define PROCESS_POLL_REQUESTED()
#endif

/*
 * Pointer to the currently running process structure.
 */
struct process *process_list;
struct process *process_current;

static process_event_t lastevent;

/*
 * Structure used for keeping the queue of active events.
 */
struct event_data {
  process_data_t data;
  struct process *p;
  process_event_t ev;
};

static process_num_events_t nevents, fevent;
static struct event_data events[PROCESS_CONF_NUMEVENTS];

#if PROCESS_CONF_STATS
process_num_events_t process_maxevents;
#endif

static volatile bool poll_requested;

#define PROCESS_STATE_NONE        0
#define PROCESS_STATE_RUNNING     1
#define PROCESS_STATE_CALLED      2

static void call_process(struct process *p, process_event_t ev, process_data_t data);
/*---------------------------------------------------------------------------*/
process_event_t
process_alloc_event(void)
{
  if(lastevent == (process_event_t)~0U) {
    LOG_WARN("Cannot allocate another event number\n");
    return PROCESS_EVENT_NONE;
  }
  return lastevent++;
}
/*---------------------------------------------------------------------------*/
void
process_start(struct process *p, process_data_t data)
{
  struct process *q;

  /* First make sure that we don't try to start a process that is
     already running. */
  for(q = process_list; q != p && q != NULL; q = q->next);

  /* If we found the process on the process list, we bail out. */
  if(q == p) {
    return;
  }
  /* Put on the procs list.*/
  p->next = process_list;
  process_list = p;
  p->state = PROCESS_STATE_RUNNING;
  PT_INIT(&p->pt);

  LOG_DBG("starting '%s'\n", PROCESS_NAME_STRING(p));

  /* Post a synchronous initialization event to the process. */
  process_post_synch(p, PROCESS_EVENT_INIT, data);
}
/*---------------------------------------------------------------------------*/
static void
exit_process(struct process *p, const struct process *fromprocess)
{
  register struct process *q;
  struct process *old_current = process_current;

  LOG_DBG("exit_process '%s'\n", PROCESS_NAME_STRING(p));

  /* Make sure the process is in the process list before we try to
     exit it. */
  for(q = process_list; q != p && q != NULL; q = q->next);
  if(q == NULL) {
    return;
  }

  if(process_is_running(p)) {
    /* Process was running */

    if(p->thread != NULL && p != fromprocess) {
      /* Post the exit event to the process that is about to exit. */
      process_current = p;
      p->thread(&p->pt, PROCESS_EVENT_EXIT, NULL);
    }
  }

  if(p == process_list) {
    process_list = process_list->next;
  } else {
    for(q = process_list; q != NULL; q = q->next) {
      if(q->next == p) {
        q->next = p->next;
        break;
      }
    }
  }

  if(process_is_running(p)) {
    /* Process was running */
    p->state = PROCESS_STATE_NONE;

    /*
     * Post a synchronous event to all processes to inform them that
     * this process is about to exit. This will allow services to
     * deallocate state associated with this process.
     */
    for(q = process_list; q != NULL; q = q->next) {
        call_process(q, PROCESS_EVENT_EXITED, (process_data_t)p);
    }
  }

  process_current = old_current;
}
/*---------------------------------------------------------------------------*/
static void
call_process(struct process *p, process_event_t ev, process_data_t data)
{
  if(p->state == PROCESS_STATE_CALLED) {
    LOG_DBG("process '%s' called again with event %d\n",
            PROCESS_NAME_STRING(p), ev);
  }

  if((p->state & PROCESS_STATE_RUNNING) &&
     p->thread != NULL) {
    LOG_DBG("calling process '%s' with event %d\n",
            PROCESS_NAME_STRING(p), ev);
    process_current = p;
    p->state = PROCESS_STATE_CALLED;
    int ret = p->thread(&p->pt, ev, data);
    if(ret == PT_EXITED || ret == PT_ENDED || ev == PROCESS_EVENT_EXIT) {
      exit_process(p, p);
    } else {
      p->state = PROCESS_STATE_RUNNING;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
process_exit(struct process *p)
{
  exit_process(p, PROCESS_CURRENT());
}
/*---------------------------------------------------------------------------*/
void
process_init(void)
{
  lastevent = PROCESS_EVENT_MAX;
}
/*---------------------------------------------------------------------------*/
/*
 * Call each process' poll handler.
 */
/*---------------------------------------------------------------------------*/
static void
do_poll(void)
{
  poll_requested = false;
  /* Call the processes that needs to be polled. */
  for(struct process *p = process_list; p != NULL; p = p->next) {
    if(p->needspoll) {
      p->state = PROCESS_STATE_RUNNING;
      p->needspoll = false;
      call_process(p, PROCESS_EVENT_POLL, NULL);
    }
  }
}
/*---------------------------------------------------------------------------*/
/*
 * Process the next event in the event queue and deliver it to
 * listening processes.
 */
/*---------------------------------------------------------------------------*/
static void
do_event(void)
{
  /*
   * If there are any events in the queue, take the first one and walk
   * through the list of processes to see if the event should be
   * delivered to any of them. If so, we call the event handler
   * function for the process. We only process one event at a time and
   * call the poll handlers inbetween.
   */
  if(nevents > 0) {

    /* There are events that we should deliver. */
    process_event_t ev = events[fevent].ev;
    process_data_t data = events[fevent].data;
    struct process *receiver = events[fevent].p;

    /* Since we have seen the new event, we move pointer upwards
       and decrease the number of events. */
    fevent = (fevent + 1) % PROCESS_CONF_NUMEVENTS;
    --nevents;

    /* If this is a broadcast event, we deliver it to all events, in
       order of their priority. */
    if(receiver == PROCESS_BROADCAST) {
      for(struct process *p = process_list; p != NULL; p = p->next) {
        /* If we have been requested to poll a process, we do this in
           between processing the broadcast event. */
        if(poll_requested) {
          do_poll();
        }
        call_process(p, ev, data);
      }
    } else {
      /* This is not a broadcast event, so we deliver it to the
         specified process. */
      /* If the event was an INIT event, we should also update the
         state of the process. */
      if(ev == PROCESS_EVENT_INIT) {
        receiver->state = PROCESS_STATE_RUNNING;
      }

      /* Make sure that the process actually is running. */
      call_process(receiver, ev, data);
    }
  }
}
/*---------------------------------------------------------------------------*/
process_num_events_t
process_run(void)
{
  /* Process poll events. */
  if(poll_requested) {
    do_poll();
  }

  /* Process one event from the queue */
  do_event();

  return nevents + poll_requested;
}
/*---------------------------------------------------------------------------*/
process_num_events_t
process_nevents(void)
{
  return nevents + poll_requested;
}
/*---------------------------------------------------------------------------*/
int
process_post(struct process *p, process_event_t ev, process_data_t data)
{
  if(nevents == PROCESS_CONF_NUMEVENTS) {
    LOG_WARN("Cannot post event %d to %s from %s because the queue is full\n",
             ev,
             p == PROCESS_BROADCAST ? "<broadcast>" : PROCESS_NAME_STRING(p),
             PROCESS_NAME_STRING(process_current));
    return PROCESS_ERR_FULL;
  }

  LOG_DBG("Process '%s' posts event %d to process '%s', nevents %d\n",
          PROCESS_NAME_STRING(PROCESS_CURRENT()),
          ev, p == PROCESS_BROADCAST ? "<broadcast>" : PROCESS_NAME_STRING(p),
          nevents);

  process_num_events_t snum =
    (process_num_events_t)(fevent + nevents) % PROCESS_CONF_NUMEVENTS;
  events[snum].ev = ev;
  events[snum].data = data;
  events[snum].p = p;
  ++nevents;

#if PROCESS_CONF_STATS
  if(nevents > process_maxevents) {
    process_maxevents = nevents;
  }
#endif /* PROCESS_CONF_STATS */

  return PROCESS_ERR_OK;
}
/*---------------------------------------------------------------------------*/
void
process_post_synch(struct process *p, process_event_t ev, process_data_t data)
{
  struct process *caller = process_current;

  call_process(p, ev, data);
  process_current = caller;
}
/*---------------------------------------------------------------------------*/
void
process_poll(struct process *p)
{
  if(p != NULL &&
     (p->state == PROCESS_STATE_RUNNING || p->state == PROCESS_STATE_CALLED)) {
    p->needspoll = true;
    poll_requested = true;
    PROCESS_POLL_REQUESTED();
  }
}
/*---------------------------------------------------------------------------*/
bool
process_is_running(struct process *p)
{
  return p->state != PROCESS_STATE_NONE;
}
/*---------------------------------------------------------------------------*/
/** @} */
