/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
 *         Contiki configuration for MSP-EXP430FR5969 LaunchPad
 */

#ifndef CONTIKI_CONF_H_
#define CONTIKI_CONF_H_

/*---------------------------------------------------------------------------*/
/* Include project-specific configuration if provided */
/*---------------------------------------------------------------------------*/
#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif

/*---------------------------------------------------------------------------*/
/* Include platform hardware definitions */
/*---------------------------------------------------------------------------*/
#include "msp-exp430fr5969-def.h"

/*---------------------------------------------------------------------------*/
/* Include MSP430 CPU definitions */
/*---------------------------------------------------------------------------*/
#include "msp430-def.h"

/*---------------------------------------------------------------------------*/
/* RAM saving configuration for 2KB SRAM */
/*---------------------------------------------------------------------------*/
/* Disable stack check - no linker support for _stack_origin on MSP430 */
#ifndef STACK_CHECK_CONF_ENABLED
#define STACK_CHECK_CONF_ENABLED 0
#endif

/* Reduce process limit */
#ifndef PROCESS_CONF_NUMEVENTS
#define PROCESS_CONF_NUMEVENTS 8
#endif

/* Disable queuebuf since there's no radio */
#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 0
#endif

/* Disable neighbor tables since there's no network */
#ifndef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 0
#endif

/*---------------------------------------------------------------------------*/
/* Network stack configuration - No radio on this platform */
/*---------------------------------------------------------------------------*/
#ifndef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO   nullradio_driver
#endif

/* Disable networking by default since there is no radio */
#ifndef NETSTACK_CONF_WITH_IPV6
#define NETSTACK_CONF_WITH_IPV6 0
#endif

#ifndef NETSTACK_CONF_WITH_IPV4
#define NETSTACK_CONF_WITH_IPV4 0
#endif

/*
 * This board has no radio and only 2 KB of SRAM, so the IPv6/IPv4 network
 * stacks cannot fit and would never have a link anyway. Fail early with a
 * clear message if an example or project-conf.h turns them on, instead of
 * the confusing errors this otherwise produces (undeclared IPv6 helpers at
 * compile time, or a RAM overflow at link time).
 */
#if NETSTACK_CONF_WITH_IPV6 || NETSTACK_CONF_WITH_IPV4
#error "MSP-EXP430FR5969 has no radio and only 2 KB SRAM; IP networking is not \
supported. Do not enable NETSTACK_CONF_WITH_IPV6 or NETSTACK_CONF_WITH_IPV4."
#endif

/*---------------------------------------------------------------------------*/
/* uIP configuration */
/*---------------------------------------------------------------------------*/
#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    128
#endif

/*---------------------------------------------------------------------------*/
/* Include MSP430 CPU configuration */
/*---------------------------------------------------------------------------*/
#include "msp430-conf.h"

#endif /* CONTIKI_CONF_H_ */
