/*
 * Copyright (c) 2018, Inria.
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
 *         The shell application
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
#include "net/ipv6/uip.h"
#include "net/ipv6/ip64-addr.h"
#include "net/ipv6/uiplib.h"

#if NETSTACK_CONF_WITH_IPV6
/*---------------------------------------------------------------------------*/
void
shell_output_6addr(shell_output_func output, const uip_ipaddr_t *ipaddr)
{
  char buf[UIPLIB_IPV6_MAX_STR_LEN];
  uiplib_ipaddr_snprint(buf, sizeof(buf), ipaddr);
  SHELL_OUTPUT(output, "%s", buf);
}
#endif /* NETSTACK_CONF_WITH_IPV6 */
/*---------------------------------------------------------------------------*/
void
shell_output_lladdr(shell_output_func output, const linkaddr_t *lladdr)
{
  if(lladdr == NULL) {
    SHELL_OUTPUT(output, "(NULL LL addr)");
    return;
  } else {
    unsigned int i;
    for(i = 0; i < LINKADDR_SIZE; i++) {
      if(i > 0 && i % 2 == 0) {
        SHELL_OUTPUT(output, ".");
      }
      SHELL_OUTPUT(output, "%02x", lladdr->u8[i]);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
output_prompt(shell_output_func output)
{
  SHELL_OUTPUT(output, "#");
  shell_output_lladdr(output, &linkaddr_node_addr);
  SHELL_OUTPUT(output, "> ");
}
/*---------------------------------------------------------------------------*/
PT_THREAD(shell_input(struct pt *pt, shell_output_func output, const char *cmd))
{
  static char *args;
  static const struct shell_command_t *cmd_descr = NULL;

  PT_BEGIN(pt);

  /* Shave off any leading spaces. */
  while(*cmd == ' ') {
    cmd++;
  }

  /* Skip empty lines */
  if(*cmd != '\0') {
    /* Look for arguments */
    args = strchr(cmd, ' ');
    if(args != NULL) {
      *args = '\0';
      args++;
    }

    cmd_descr = shell_command_lookup(cmd);
    if(cmd_descr != NULL) {
      static struct pt cmd_pt;
      PT_SPAWN(pt, &cmd_pt, cmd_descr->func(&cmd_pt, output, args));
    } else {
      SHELL_OUTPUT(output, "Command not found. Type 'help' for a list of commands\n");
    }
  }

  output_prompt(output);
  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
void
shell_init(void)
{
  shell_commands_init();
}
/** @} */
