/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
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
 */

/**
 * \file
 *         Example posix implementation of CoAP timer driver.
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "coap-timer.h"
#include <sys/time.h>

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "posix-timer"
#define LOG_LEVEL  LOG_LEVEL_WARN

/* The maximal time allowed to move forward between updates */
#define MAX_TIME_CHANGE_MSEC 360000UL

static uint64_t uptime_msec = 0;
static uint64_t last_msec;
/*---------------------------------------------------------------------------*/
static uint64_t
uptime(void)
{
  struct timeval tv;
  uint64_t t;

  if(gettimeofday(&tv, NULL)) {
    LOG_WARN("*** failed to retrieve system time\n");
    return last_msec;
  }

  t = tv.tv_sec * (uint64_t)1000;
  t += tv.tv_usec / (uint64_t)1000;

  if(last_msec == 0) {
    /* No update first time */
  } else if(t < last_msec) {
    /* System time has moved backwards */
    LOG_WARN("*** system time has moved backwards %lu msec\n",
             (unsigned long)(last_msec - t));

  } else if(t - last_msec > MAX_TIME_CHANGE_MSEC) {
    /* Too large jump forward in system time */
    LOG_WARN("*** system time has moved forward %lu msec\n",
             (unsigned long)(t - last_msec));
    uptime_msec += 1000UL;
  } else {
    uptime_msec += t - last_msec;
  }

  last_msec = t;

  return uptime_msec;
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  uptime();
}
/*---------------------------------------------------------------------------*/
static void
update(void)
{
}
/*---------------------------------------------------------------------------*/
const coap_timer_driver_t coap_timer_native_driver = {
  .init = init,
  .uptime = uptime,
  .update = update,
};
/*---------------------------------------------------------------------------*/
