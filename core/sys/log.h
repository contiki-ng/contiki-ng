/*
 * Copyright (c) 2017, RISE SICS.
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
 *         Header file for the logging system
 * \author
 *         Simon Duquennoy <simon.duquennoy@ri.se>
 */

/** \addtogroup sys
 * @{ */

/**
 * \defgroup log Per-module, per-level logging
 * @{
 *
 * The log module performs per-module, per-level logging
 *
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include "net/linkaddr.h"
#include "sys/log-conf.h"

#if NETSTACK_CONF_WITH_IPV6
#include "net/ip/uip.h"
#endif /* NETSTACK_CONF_WITH_IPV6 */

void net_debug_lladdr_print(const linkaddr_t *addr);
void uip_debug_ipaddr_print(const uip_ipaddr_t *addr);

#define LOG_LEVEL_NONE         0 /* No log */
#define LOG_LEVEL_ERR          1 /* Errors */
#define LOG_LEVEL_WARN         2 /* Warnings */
#define LOG_LEVEL_INFO         3 /* Basic info */
#define LOG_LEVEL_DBG          4 /* Detailled debug */

/* Prefix all logs with file name and line-of-code */
#ifdef LOG_CONF_WITH_LOC
#define LOG_WITH_LOC LOG_CONF_WITH_LOC
#else /* LOG_CONF_WITH_LOC */
#define LOG_WITH_LOC 0
#endif /* LOG_CONF_WITH_LOC */

/* Custom output function -- default is printf */
#ifdef LOG_CONF_OUTPUT
#define LOG_OUTPUT(...) LOG_CONF_OUTPUT(__VA_ARGS__)
#else /* LOG_CONF_OUTPUT */
#define LOG_OUTPUT(...) printf(__VA_ARGS__)
#endif /* LOG_CONF_OUTPUT */

/* Cooja annotations */
#ifdef LOG_CONF_WITH_ANNOTATE
#define LOG_WITH_ANNOTATE LOG_CONF_WITH_ANNOTATE
#else /* LOG_CONF_WITH_ANNOTATE */
#define LOG_WITH_ANNOTATE 0
#endif /* LOG_CONF_WITH_ANNOTATE */

/* Main log function */
#define LOG(level, ...) do {  \
                            if(level <= LOG_LEVEL) { \
                              if(LOG_WITH_LOC) { \
                                LOG_OUTPUT("%s:%d: ", __FILE__, __LINE__); \
                              } \
                              LOG_OUTPUT("%s: ", LOG_MODULE); \
                              LOG_OUTPUT(__VA_ARGS__); \
                            } \
                        } while (0)

/* For Cooja annotations */
#define LOG_ANNOTATE(...) do {  \
                            if(LOG_WITH_ANNOTATE) { \
                              LOG_OUTPUT(__VA_ARGS__); \
                            } \
                        } while (0)

/* Link-layer address */
#define LOG_LLADDR(level, lladdr) do {  \
                            if(level <= LOG_LEVEL) { \
                              net_debug_lladdr_print(lladdr); \
                            } \
                        } while (0)

/* IPv6 address */
#define LOG_6ADDR(level, lladdr) do {  \
                           if(level <= LOG_LEVEL) { \
                             uip_debug_ipaddr_print(lladdr); \
                           } \
                       } while (0)

/* More compact versions of LOG macros */
#define LOG_ERR(...)           LOG(LOG_LEVEL_ERR, __VA_ARGS__)
#define LOG_WARN(...)          LOG(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_INFO(...)          LOG(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_DBG(...)           LOG(LOG_LEVEL_DBG, __VA_ARGS__)

#define LOG_ERR_LLADDR(...)    LOG_LLADDR(LOG_LEVEL_ERR, __VA_ARGS__)
#define LOG_WARN_LLADDR(...)   LOG_LLADDR(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_INFO_LLADDR(...)   LOG_LLADDR(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_DBG_LLADDR(...)    LOG_LLADDR(LOG_LEVEL_DBG, __VA_ARGS__)

#define LOG_ERR_6ADDR(...)     LOG_6ADDR(LOG_LEVEL_ERR, __VA_ARGS__)
#define LOG_WARN_6ADDR(...)    LOG_6ADDR(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_INFO_6ADDR(...)    LOG_6ADDR(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_DBG_6ADDR(...)     LOG_6ADDR(LOG_LEVEL_DBG, __VA_ARGS__)

/* For testing log level */
#define LOG_ERR_ENABLED        (LOG_LEVEL >= LOG_LEVEL_ERR)
#define LOG_WARN_ENABLED       (LOG_LEVEL >= LOG_LEVEL_WARN)
#define LOG_INFO_ENABLED       (LOG_LEVEL >= LOG_LEVEL_INFO)
#define LOG_DBG_ENABLED        (LOG_LEVEL >= LOG_LEVEL_DBG)
#define LOG_ANNOTATE_ENABLED   (LOG_LEVEL >= LOG_LEVEL_ANNOTATE)

#endif /* __LOG_H__ */

/** @} */
/** @} */
