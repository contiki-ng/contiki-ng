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

/** \addtogroup apps
 * @{ */

#ifndef SHELL_H_
#define SHELL_H_

#include "net/ipv6/uip.h"
#include "net/linkaddr.h"
#include "sys/process.h"
#include <stdio.h>

/* Helper macros to parse arguments */
#define SHELL_ARGS_INIT(args, next_args) (next_args) = (args);

#define SHELL_ARGS_NEXT(args, next_args) do {   \
    (args) = (next_args);                       \
    if((args) != NULL) {                        \
      if(*(args) == '\0') {                     \
        (args) = NULL;                          \
      } else {                                  \
        (next_args) = strchr((args), ' ');      \
        if((next_args) != NULL) {               \
          *(next_args) = '\0';                  \
          (next_args)++;                        \
        }                                       \
      }                                         \
    } else {                                    \
      (next_args) = NULL;                       \
    }                                           \
  } while(0)

/* Printf-formatted output via a given output function */
#define SHELL_OUTPUT(output_func, format, ...) do {             \
    char buffer[192];                                           \
    snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__);    \
    (output_func)(buffer);                                      \
  } while(0);

typedef void (shell_output_func)(const char *str);

/**
 * Initializes Shell module
 */
void shell_init(void);

/**
 * \brief A protothread that is spawned by a Shell driver when receiving a new line.
 */
PT_THREAD(shell_input(struct pt *pt, shell_output_func output, const char *cmd));

/**
 * Prints an IPv6 address
 *
 * \param output The output function
 * \param ipaddr The IPv6 to printed
 */
void shell_output_6addr(shell_output_func output, const uip_ipaddr_t *ipaddr);

/**
 * Prints a link-layer address
 *
 * \param output The output function
 * \param lladdr The link-layer to be printed
 */
void shell_output_lladdr(shell_output_func output, const linkaddr_t *lladdr);

#endif /* SHELL_H_ */

/** @} */
