/*
 * Copyright (c) 2023 RISE Research Institutes of Sweden
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "contiki.h"
#include "trustzone/tz-api.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "SecureProcess"
#define LOG_LEVEL LOG_LEVEL_DBG
/*---------------------------------------------------------------------------*/
PROCESS(secure_process, "Secure process");
AUTOSTART_PROCESSES(&secure_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(secure_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  PROCESS_WAIT_EVENT_UNTIL(ev == trustzone_init_event);

  LOG_INFO("Ready to run\n");

  etimer_set(&et, 60 * CLOCK_SECOND);
  for(;;) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    LOG_INFO("Got a timer event in the secure world\n");
    etimer_reset(&et);
  }

  PROCESS_END();
}
