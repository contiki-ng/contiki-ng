/*
 * Copyright (c) 2023, RISE Research Institutes of Sweden
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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

/*
 * \file
 *   TrustZone API setup for normal world.
 * \author
 *   Niclas Finne <niclas.finne@ri.se>
 *   Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"
#include "dev/serial-line.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "sys/platform.h"
#include "trustzone/tz-api.h"
#include "trustzone/normal/tz-normal.h"

/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "TZNormalWorld"
#if defined(TZ_DEBUG_KEEP_SECURE_UART)
#define LOG_LEVEL LOG_LEVEL_NONE
#else
#define LOG_LEVEL LOG_LEVEL_INFO
#endif
/*---------------------------------------------------------------------------*/
static volatile bool is_poll_requested;
static uint8_t packet_data[PACKETBUF_SIZE];

PROCESS(tz_normal_process, "TZ normal process");
/*---------------------------------------------------------------------------*/
bool
tz_normal_request_poll(void)
{
  is_poll_requested = true;
  process_poll(&tz_normal_process);
  return true;
}
/*---------------------------------------------------------------------------*/
__attribute__((weak)) void
tz_arch_init_ns_signal(void)
{
}
/*---------------------------------------------------------------------------*/
static bool
process_packet_data(size_t packet_size)
{
  if(packet_size == 0 || packet_size > sizeof(packet_data)) {
    return false;
  }

  packetbuf_clear();
  packetbuf_copyfrom(packet_data, packet_size);
  NETSTACK_MAC.input();

  return true;
}
/*---------------------------------------------------------------------------*/
static void
init_tz_api(void)
{
  struct tz_api tz_api = {0};

  tz_api.request_poll = tz_normal_request_poll;
  tz_api.process_packet_data = process_packet_data;
  tz_api.packet_data = packet_data;
  tz_api.packet_data_size = sizeof(packet_data);
  tz_api.serial_input = serial_line_input_byte;
  bool result = tz_api_init(&tz_api);
  LOG_INFO("Initialize TrustZone API: %s\n",
           result ? "SUCCESS" : "FAILURE");

  tz_arch_init_ns_signal();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tz_normal_process, ev, data)
{
  PROCESS_BEGIN();

  while(true) {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
    if(is_poll_requested) {
      is_poll_requested = false;
      LOG_DBG("> Poll secure world\n");
      if(tz_api_poll()) {
        tz_normal_request_poll();
      }
      LOG_DBG("< Poll secure world %s!\n",
              is_poll_requested ? "waiting" : "done");
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
platform_main_loop(void)
{
  /*
   * Start tz_normal_process before init_tz_api so the EGU doorbell
   * armed by tz_arch_init_ns_signal() can validly poll a process in
   * PROCESS_STATE_RUNNING if the IRQ fires immediately on enable.
   * Then issue one explicit poll to drain trustzone_init_event and
   * any other secure-side events queued during init -- process_post
   * does not fire PROCESS_POLL_REQUESTED, so without this kick the
   * init events would sit unread until something else woke NS.
   */
  process_start(&tz_normal_process, NULL);
  init_tz_api();
  tz_normal_request_poll();

  while(1) {
    process_num_events_t r;
    do {
      r = process_run();
      watchdog_periodic();
    } while(r > 0);

    platform_idle();
  }
}
/*---------------------------------------------------------------------------*/
