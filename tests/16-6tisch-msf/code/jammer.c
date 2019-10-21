/*
 * Copyright (c) 2019, Inria.
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

#include <stdlib.h>
#include <stdint.h>

#include <contiki.h>
#include <lib/assert.h>

#include <dev/serial-line.h>
#include <net/mac/tsch/tsch.h>

#define SLOTFRAME_HANDLE 0
#define SLOTFRAME_DURATION (                                            \
    TSCH_SCHEDULE_DEFAULT_LENGTH *                                      \
    TSCH_DEFAULT_TIMESLOT_TIMING[tsch_ts_timeslot_length] *             \
    CLOCK_SECOND / 1000000)

PROCESS(jammer_process, "Jammer");
AUTOSTART_PROCESSES(&jammer_process);

static int32_t
parse_uint16(char *str, char **next_str_p)
{
  int i;
  char uint16[6]; /* max value is "65535" with a null char */

  memset(uint16, 0, sizeof(uint16));
  for(i = 0; i < sizeof(uint16); i++) {
    if(str[i] >= '0' && str[i] <= '9') {
      uint16[i] = str[i];
    } else {
      /* treat the other characters as a terminator */
      uint16[i] = '\0';
      break;
    }
  }

  if(i == sizeof(uint16)) {
    /* we cannot parse the string */
    *next_str_p = str + i;
    return -1;
  } else {
    *next_str_p = str + i + 1;
    return atoi(uint16);
  }
}

static int
schedule_tx_cell(uint16_t slot_offset, uint16_t channel_offset,
                 uint8_t *num_tx_cells)
{
  /* allocate a cell and increase traffic */
  struct tsch_slotframe *slotframe;
  struct tsch_neighbor *timesource;
  int ret;

  printf("jammer: scheduling a cell at slot offset: %d, channel offset: %d\n",
         slot_offset, channel_offset);
  if((slotframe =
      tsch_schedule_get_slotframe_by_handle(SLOTFRAME_HANDLE)) != NULL &&
     (timesource = tsch_queue_get_time_source()) != NULL &&
     tsch_schedule_add_link(slotframe, LINK_OPTION_TX, LINK_TYPE_NORMAL,
                            &timesource->addr,
                            slot_offset, channel_offset) != NULL) {
    printf("jammer: succeeded to schedule a TX cell\n");
    *num_tx_cells += 1;
    ret = 0;
  } else {
    printf("jammer: failed to schedule a TX cell; try it later\n");
    ret = -1;
  }
  return ret;
}

PROCESS_THREAD(jammer_process, ev, data)
{
  char *str_p;
  static int32_t slot_offset, channel_offset;
  static struct etimer et;
  static uint8_t num_tx_cells = 0;
  static bool do_scheduling = false;

  PROCESS_BEGIN();
  etimer_set(&et, SLOTFRAME_DURATION);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message ||
                             etimer_expired(&et));
    /* we expect comma-separated two values, slot offset and channel offset */
    if(ev == serial_line_event_message) {
      if(data == NULL) {
        /* shouldn't happen; but catch it here; do nothing */
      } else {
        str_p = (char *)data;
        printf("jammer: recv msg - %s\n", data);
        if(str_p[0] == 's') { /* schedule */
          str_p++;
          slot_offset = parse_uint16(str_p, &str_p);
          channel_offset = parse_uint16(str_p, &str_p);
          if(slot_offset > 0 &&
             channel_offset > 0) {
            do_scheduling = true;
          } else {
            /* invalid values; ignore them */
            do_scheduling = false;
          }
        } else if(str_p[0] == 'q') { /* quit */
          printf("jammer: quit the main loop\n");
          break;
        }
      }
    }

    if(etimer_expired(&et) || do_scheduling == true) {
      if(do_scheduling == true) {
        if(schedule_tx_cell(slot_offset, channel_offset,
                            &num_tx_cells) == 0) {
          assert(num_tx_cells > 0);
          etimer_set(&et, SLOTFRAME_DURATION / num_tx_cells);
          do_scheduling = false;
        } else {
          /* retry later */
          do_scheduling = true;
        }
      }
      if(num_tx_cells > 0) {
        printf("jammer: send KA\n");
        tsch_schedule_keepalive_immediately();
      }
      if(etimer_expired(&et)) {
        etimer_reset(&et);
      }
    }
  }

  PROCESS_END();
}
