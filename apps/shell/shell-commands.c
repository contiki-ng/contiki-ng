/*
 * Copyright (c) 2017, Inria.
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
 * \file
 *         The Contiki shell commands
 * \author
 *         Simon Duquennoy <simon.duquennoy@inria.fr>
 */

/**
 * \addtogroup shell
 * @{
 */

#include "contiki.h"
#include "shell.h"
#include "shell-commands.h"
#include "sys/log.h"
#include "net/ip/uip.h"
#include "net/ip/uiplib.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/uip-ds6.h"
#include "net/mac/tsch/tsch-log.h"
#if UIP_CONF_IPV6_RPL_LITE == 1
#include "net/rpl-lite/rpl.h"
#else /* UIP_CONF_IPV6_RPL_LITE == 1 */
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#endif /* UIP_CONF_IPV6_RPL_LITE == 1 */

#include <stdlib.h>

#define PING_TIMEOUT (5 * CLOCK_SECOND)

static struct uip_icmp6_echo_reply_notification echo_reply_notification;
static shell_output_func *curr_ping_output_func = NULL;
static struct process *curr_ping_process;

/*---------------------------------------------------------------------------*/
static void
echo_reply_handler(uip_ipaddr_t *source, uint8_t ttl, uint8_t *data, uint16_t datalen)
{
  if(curr_ping_output_func != NULL) {
    SHELL_OUTPUT(curr_ping_output_func, "Received ping reply from ");
    shell_output_6addr(curr_ping_output_func, source);
    SHELL_OUTPUT(curr_ping_output_func, ", ttl %u, len %u\n", ttl, datalen);
    curr_ping_output_func = NULL;
    process_poll(curr_ping_process);
  }
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(cmd_ping(struct pt *pt, shell_output_func output, char *args))
{
  static uip_ipaddr_t remote_addr;
  static struct etimer timeout_timer;
  char *next_args;

  PT_BEGIN(pt);

  SHELL_ARGS_INIT(args, next_args);

  /* Get argument (remote IPv6) */
  SHELL_ARGS_NEXT(args, next_args);
  if(uiplib_ipaddrconv(args, &remote_addr) == 0) {
    SHELL_OUTPUT(output, "Invalid IPv6: %s\n", args);
    PT_EXIT(pt);
  }

  SHELL_OUTPUT(output, "Pinging ");
  shell_output_6addr(output, &remote_addr);
  SHELL_OUTPUT(output, "\n");

  /* Send ping request */
  curr_ping_process = PROCESS_CURRENT();
  curr_ping_output_func = output;
  etimer_set(&timeout_timer, PING_TIMEOUT);
  uip_icmp6_send(&remote_addr, ICMP6_ECHO_REQUEST, 0, 4);
  PT_WAIT_UNTIL(pt, curr_ping_output_func == NULL || etimer_expired(&timeout_timer));

  if(curr_ping_output_func != NULL) {
    SHELL_OUTPUT(output, "Timeout\n");
    curr_ping_output_func = NULL;
  }

  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(cmd_rpl_set_root(struct pt *pt, shell_output_func output, char *args))
{
  static int is_on;
  static uip_ipaddr_t prefix;
  char *next_args;

  PT_BEGIN(pt);

  SHELL_ARGS_INIT(args, next_args);

  /* Get first arg (0/1) */
  SHELL_ARGS_NEXT(args, next_args);

  if(!strcmp(args, "1")) {
    is_on = 1;
  } else if(!strcmp(args, "0")) {
    is_on = 0;
  } else {
    SHELL_OUTPUT(output, "Invalid argument: %s\n", args);
    PT_EXIT(pt);
  }

  /* Get first second arg (prefix) */
  SHELL_ARGS_NEXT(args, next_args);
  if(args != NULL) {
    if(uiplib_ipaddrconv(args, &prefix) == 0) {
      SHELL_OUTPUT(output, "Invalid Prefix: %s\n", args);
      PT_EXIT(pt);
    }
  } else {
    uip_ip6addr(&prefix, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  }

  if(is_on) {
    if(!rpl_dag_root_is_root()) {
      SHELL_OUTPUT(output, "Setting as DAG root with prefix ");
      shell_output_6addr(output, &prefix);
      SHELL_OUTPUT(output, "\n");
      rpl_dag_root_init(&prefix, NULL);
      rpl_dag_root_init_dag_immediately();
    } else {
      SHELL_OUTPUT(output, "Node is already a DAG root\n");
    }
  } else {
    if(rpl_dag_root_is_root()) {
      SHELL_OUTPUT(output, "Setting as non-root node: leaving DAG\n");
      rpl_dag_leave();
    } else {
      SHELL_OUTPUT(output, "Node is not a DAG root\n");
    }
  }

  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(cmd_log(struct pt *pt, shell_output_func output, char *args))
{
  static int prev_level;
  static int level;
  char *next_args;
  char *ptr;

  PT_BEGIN(pt);

  SHELL_ARGS_INIT(args, next_args);

  /* Get and parse argument */
  SHELL_ARGS_NEXT(args, next_args);
  level = (int)strtol(args, &ptr, 10);
  if((level == 0 && args == ptr)
    || level < LOG_LEVEL_NONE || level > LOG_LEVEL_DBG) {
    SHELL_OUTPUT(output, "Invalid argument: %s\n", args);
    PT_EXIT(pt);
  }

  /* Set log level */
  prev_level = log_get_level();
  if(level != prev_level) {
    log_set_level(level);
    if(level >= LOG_LEVEL_DBG) {
      tsch_log_init();
      SHELL_OUTPUT(output, "TSCH logging started\n");
    } else {
      tsch_log_stop();
      SHELL_OUTPUT(output, "TSCH logging stopped\n");
    }
  }
  SHELL_OUTPUT(output, "Log level set to %u (%s)\n", level, log_level_to_str(level));

  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(cmd_help(struct pt *pt, shell_output_func output, char *args))
{
  struct shell_command_t *cmd_ptr;

  PT_BEGIN(pt);

  SHELL_OUTPUT(output, "Available commands:\n");
  cmd_ptr = shell_commands;
  while(cmd_ptr->name != NULL) {
    SHELL_OUTPUT(output, "%s\n", cmd_ptr->help);
    cmd_ptr++;
  }

  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
void
shell_commands_init(void)
{
  /* Set up Ping Reply callback */
  uip_icmp6_echo_reply_callback_add(&echo_reply_notification,
                                    echo_reply_handler);
}
/*---------------------------------------------------------------------------*/
struct shell_command_t shell_commands[] = {
  { "help",          cmd_help,            "'> help': Shows this help" },
  { "ping",          cmd_ping,            "'> ping addr': Pings the IPv6 address 'addr'" },
  { "rpl-set-root",  cmd_rpl_set_root,    "'> rpl-set-root 0/1 [prefix]': Sets node as root (on) or not (off). A /64 prefix can be optionally specified." },
  { "log",           cmd_log,             "'> log level': Sets log level (0--4). Level 4 also enables TSCH per-slot logging." },
  { NULL, NULL, NULL },
};

/** @} */
