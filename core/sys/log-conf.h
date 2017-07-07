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
 *         Default log levels for a number of modules
 * \author
 *         Simon Duquennoy <simon.duquennoy@inria.fr>
 */

/** \addtogroup sys
 * @{ */

/** \addtogroup log
* @{ */

#ifndef __LOG_CONF_H__
#define __LOG_CONF_H__

/* Log only the last 16 bytes of link-layer and IPv6 addresses */
#ifdef LOG_CONF_WITH_COMPACT_ADDR
#define LOG_WITH_COMPACT_ADDR LOG_CONF_WITH_COMPACT_ADDR
#else /* LOG_CONF_WITH_COMPACT_ADDR */
#define LOG_WITH_COMPACT_ADDR 0
#endif /* LOG_CONF_WITH_COMPACT_ADDR */

/* Prefix all logs with file name and line-of-code */
#ifdef LOG_CONF_WITH_LOC
#define LOG_WITH_LOC LOG_CONF_WITH_LOC
#else /* LOG_CONF_WITH_LOC */
#define LOG_WITH_LOC 0
#endif /* LOG_CONF_WITH_LOC */

/* Cooja annotations */
#ifdef LOG_CONF_WITH_ANNOTATE
#define LOG_WITH_ANNOTATE LOG_CONF_WITH_ANNOTATE
#else /* LOG_CONF_WITH_ANNOTATE */
#define LOG_WITH_ANNOTATE 0
#endif /* LOG_CONF_WITH_ANNOTATE */

/* Custom output function -- default is printf */
#ifdef LOG_CONF_OUTPUT
#define LOG_OUTPUT(...) LOG_CONF_OUTPUT(__VA_ARGS__)
#else /* LOG_CONF_OUTPUT */
#define LOG_OUTPUT(...) printf(__VA_ARGS__)
#endif /* LOG_CONF_OUTPUT */

/******************************************************************************/
/********************* A list of currently supported modules ******************/
/******************************************************************************/

#ifndef RPL_LOG_LEVEL
#define RPL_LOG_LEVEL                         LOG_LEVEL_NONE /* Only for rpl-lite */
#endif /* RPL_LOG_LEVEL */

#ifndef TCPIP_LOG_LEVEL
#define TCPIP_LOG_LEVEL                       LOG_LEVEL_NONE
#endif /* TCPIP_LOG_LEVEL */

#ifndef IPV6_LOG_LEVEL
#define IPV6_LOG_LEVEL                        LOG_LEVEL_NONE
#endif /* IPV6_LOG_LEVEL */

#ifndef SICSLOWPAN_LOG_LEVEL
#define SICSLOWPAN_LOG_LEVEL                  LOG_LEVEL_NONE
#endif /* SICSLOWPAN_LOG_LEVEL */

#ifndef MAC_LOG_LEVEL
#define MAC_LOG_LEVEL                         LOG_LEVEL_NONE
#endif /* MAC_LOG_LEVELL */

#ifndef FRAMER_LOG_LEVEL
#define FRAMER_LOG_LEVEL                      LOG_LEVEL_NONE
#endif /* FRAMER_LOG_LEVEL */

#endif /* __LOG_CONF_H__ */

/** @} */
/** @} */
