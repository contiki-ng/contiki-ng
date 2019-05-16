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
 *         Main header file for the Contiki shell
 * \author
 *         Simon Duquennoy <simon.duquennoy@inria.fr>
 */

/** \addtogroup shell
 * @{ */

#ifndef _SHELL_COMMANDS_H_
#define _SHELL_COMMANDS_H_

/* Command handling function type */
typedef char (shell_commands_func)(struct pt *pt, shell_output_func output, char *args);

/* Command structure */
struct shell_command_t {
  const char *name;
  shell_commands_func *func;
  const char *help;
};

struct shell_command_set_t {
  struct shell_command_set_t *next;
  const struct shell_command_t *const commands;
};

void shell_command_set_register(struct shell_command_set_t *);
int shell_command_set_deregister(struct shell_command_set_t *);
const struct shell_command_t *shell_command_lookup(const char *);

/**
 * Initializes Shell-commands module
 */
void shell_commands_init(void);

#include "net/mac/tsch/tsch.h"
#if TSCH_WITH_SIXTOP
typedef void (*shell_command_6top_sub_cmd_t)(shell_output_func output,
                                             char *args);
void shell_commands_set_6top_sub_cmd(shell_command_6top_sub_cmd_t sub_cmd);
#endif /* TSCH_WITH_SIXTOP */

#endif /* _SHELL_COMMANDS_H_ */

/** @} */
