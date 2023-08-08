/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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
 *         Sets up some commands for the border router
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "cmd.h"
#include "border-router.h"
#include "border-router-cmds.h"
#include "dev/serial-line.h"
#include "net/routing/routing.h"
#include "net/ipv6/uiplib.h"
#include <string.h>
#include "shell.h"
#include <stdio.h>

/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "BR"
#define LOG_LEVEL LOG_LEVEL_NONE

uint8_t command_context;

void packet_sent(uint8_t sessionid, uint8_t status, uint8_t tx);
void nbr_print_stat(void);

/*---------------------------------------------------------------------------*/
PROCESS(border_router_cmd_process, "Border router cmd process");
/*---------------------------------------------------------------------------*/
static int
hextoi(const uint8_t *buf, int len)
{
  int v = 0;
  for(; len > 0; len--, buf++) {
    if(*buf >= '0' && *buf <= '9') {
      v = (v << 4) + ((*buf - '0') & 0xf);
    } else if(*buf >= 'a' && *buf <= 'f') {
      v = (v << 4) + ((*buf - 'a' + 10) & 0xf);
    } else if(*buf >= 'A' && *buf <= 'F') {
      v = (v << 4) + ((*buf - 'A' + 10) & 0xf);
    } else {
      break;
    }
  }
  return v;
}
/*---------------------------------------------------------------------------*/
static int
dectoi(const uint8_t *buf, int len)
{
  int negative = 0;
  if(len <= 0) {
    return 0;
  }
  if(*buf == '$') {
    return hextoi(buf + 1, len - 1);
  }
  if(*buf == '0' && *(buf + 1) == 'x' && len > 2) {
    return hextoi(buf + 2, len - 2);
  }
  if(*buf == '-') {
    negative = 1;
    buf++;
  }
  int v = 0;
  for(; len > 0; len--, buf++) {
    if(*buf < '0' || *buf > '9') {
      break;
    }
    v = (v * 10) + ((*buf - '0') & 0xf);
  }
  return negative ? -v : v;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* TODO: the below code needs some way of identifying from where the command */
/* comes. In this case it can be from stdin or from SLIP.                    */
/*---------------------------------------------------------------------------*/
int
border_router_cmd_handler(const uint8_t *data, int len)
{
  /* handle global repair, etc here */
  if(data[0] == '!') {
    LOG_DBG("Got configuration message of type %c\n", data[1]);
    if(command_context == CMD_CONTEXT_STDIO) {
      switch(data[1]) {
      case 'G':
        /* This is supposed to be from stdin */
        printf("Performing Global Repair...\n");
        NETSTACK_ROUTING.global_repair("Command");
        return 1;
      case 'C': {
        /* send on a set-param thing! */
        uint8_t set_param[] = {'!', 'V', 0, RADIO_PARAM_CHANNEL, 0, 0 };
        int channel = dectoi(&data[2], len - 2);
        set_param[5] = channel & 0xff;
        write_to_slip(set_param, sizeof(set_param));
        return 1;
      }
      case 'P': {
        /* send on a set-param thing! */
        uint8_t set_param[] = {'!', 'V', 0, RADIO_PARAM_PAN_ID, 0, 0 };
        int pan_id = dectoi(&data[2], len - 2);
        set_param[4] = (pan_id >> 8) & 0xff;
        set_param[5] = pan_id & 0xff;
        write_to_slip(set_param, sizeof(set_param));
        return 1;
      }
      default:
        return 0;
      }
    } else if(command_context == CMD_CONTEXT_RADIO) {
      /* We need to know that this is from the slip-radio here. */
      switch(data[1]) {
      case 'M':
        LOG_DBG("Setting MAC address\n");
        border_router_set_mac(&data[2]);
        return 1;
      case 'V':
        if(data[3] == RADIO_PARAM_CHANNEL) {
          printf("Channel is %d\n", data[5]);
        }
        if(data[3] == RADIO_PARAM_PAN_ID) {
          printf("PAN_ID is 0x%04x\n", (data[4] << 8) + data[5]);
        }
        return 1;
      case 'R':
        LOG_DBG("Packet data report for sid:%d st:%d tx:%d\n",
               data[2], data[3], data[4]);
        packet_sent(data[2], data[3], data[4]);
        return 1;
      default:
      return 0;
      }
    }
  } else if(data[0] == '?') {
    LOG_DBG("Got request message of type %c\n", data[1]);
    if(data[1] == 'M' && command_context == CMD_CONTEXT_STDIO) {
      uint8_t buf[20];
      char *hexchar = "0123456789abcdef";
      int j;
      /* this is just a test so far... just to see if it works */
      buf[0] = '!';
      buf[1] = 'M';
      for(j = 0; j < UIP_LLADDR_LEN; j++) {
        buf[2 + j * 2] = hexchar[uip_lladdr.addr[j] >> 4];
        buf[3 + j * 2] = hexchar[uip_lladdr.addr[j] & 15];
      }
      cmd_send(buf, 18);
      return 1;
    } else if(data[1] == 'C' && command_context == CMD_CONTEXT_STDIO) {
      /* send on a set-param thing! */
      uint8_t set_param[] = {'?', 'V', 0, RADIO_PARAM_CHANNEL};
      write_to_slip(set_param, sizeof(set_param));
      return 1;
    } else if(data[1] == 'P' && command_context == CMD_CONTEXT_STDIO) {
      /* send on a set-param thing! */
      uint8_t set_param[] = {'?', 'V', 0, RADIO_PARAM_PAN_ID};
      write_to_slip(set_param, sizeof(set_param));
      return 1;
    } else if(data[1] == 'S') {
      border_router_print_stat();
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
border_router_cmd_output(const uint8_t *data, int data_len)
{
  int i;
  printf("CMD output: ");
  for(i = 0; i < data_len; i++) {
    printf("%c", data[i]);
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
static void
serial_shell_output(const char *str)
{
  printf("%s", str);
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(border_router_cmd_process, ev, data)
{
  static struct pt shell_input_pt;
  PROCESS_BEGIN();

  shell_init();

  while(1) {
    PROCESS_YIELD();
    if(ev == serial_line_event_message && data != NULL) {
      LOG_DBG("Got serial data!!! %s of len: %zd\n",
              (char *)data, strlen((char *)data));
      command_context = CMD_CONTEXT_STDIO;
      if(!cmd_input(data, strlen((char *)data))) {
        /* did not find command - run shell and see if ... */
        PROCESS_PT_SPAWN(&shell_input_pt,
                         shell_input(&shell_input_pt, serial_shell_output,
                                     data));
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
