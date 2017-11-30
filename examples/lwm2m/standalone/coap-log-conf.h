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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *         Standalone CoAP log configuration
 * \author
 *         Niclas Finne <niclas.finne@ri.se>
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

#ifndef COAP_LOG_CONF_H_
#define COAP_LOG_CONF_H_

#include "contiki.h"
#include <stdio.h>

/* The different log levels available */
#define LOG_LEVEL_NONE         0 /* No log */
#define LOG_LEVEL_ERR          1 /* Errors */
#define LOG_LEVEL_WARN         2 /* Warnings */
#define LOG_LEVEL_INFO         3 /* Basic info */
#define LOG_LEVEL_DBG          4 /* Detailed debug */

#ifndef LOG_LEVEL_COAP
#define LOG_LEVEL_COAP         LOG_LEVEL_DBG
#endif /* LOG_LEVEL_COAP */

#ifndef LOG_WITH_LOC
#define LOG_WITH_LOC           0
#endif /* LOG_WITH_LOC */

#ifndef LOG_WITH_MODULE_PREFIX
#define LOG_WITH_MODULE_PREFIX 1
#endif /* LOG_WITH_MODULE_PREFIX */

/* Custom output function -- default is printf */
#ifdef LOG_CONF_OUTPUT
#define LOG_OUTPUT(...) LOG_CONF_OUTPUT(__VA_ARGS__)
#else /* LOG_CONF_OUTPUT */
#define LOG_OUTPUT(...) printf(__VA_ARGS__)
#endif /* LOG_CONF_OUTPUT */

/* Custom line prefix output function -- default is LOG_OUTPUT */
#ifdef LOG_CONF_OUTPUT_PREFIX
#define LOG_OUTPUT_PREFIX(level, levelstr, module) LOG_CONF_OUTPUT_PREFIX(level, levelstr, module)
#else /* LOG_CONF_OUTPUT_PREFIX */
#define LOG_OUTPUT_PREFIX(level, levelstr, module) LOG_OUTPUT("[%-4s: %-10s] ", levelstr, module)
#endif /* LOG_CONF_OUTPUT_PREFIX */

/* Main log function */

#define LOG(newline, level, levelstr, ...) do {                         \
    if(level <= (LOG_LEVEL)) {                                          \
      if(newline) {                                                     \
        if(LOG_WITH_MODULE_PREFIX) {                                    \
          LOG_OUTPUT_PREFIX(level, levelstr, LOG_MODULE);               \
        }                                                               \
        if(LOG_WITH_LOC) {                                              \
          LOG_OUTPUT("[%s: %d] ", __FILE__, __LINE__);                  \
        }                                                               \
      }                                                                 \
      LOG_OUTPUT(__VA_ARGS__);                                          \
    }                                                                   \
  } while (0)

/* More compact versions of LOG macros */
#define LOG_ERR(...)           LOG(1, LOG_LEVEL_ERR, "ERR", __VA_ARGS__)
#define LOG_WARN(...)          LOG(1, LOG_LEVEL_WARN, "WARN", __VA_ARGS__)
#define LOG_INFO(...)          LOG(1, LOG_LEVEL_INFO, "INFO", __VA_ARGS__)
#define LOG_DBG(...)           LOG(1, LOG_LEVEL_DBG, "DBG", __VA_ARGS__)

#define LOG_ERR_(...)          LOG(0, LOG_LEVEL_ERR, "ERR", __VA_ARGS__)
#define LOG_WARN_(...)         LOG(0, LOG_LEVEL_WARN, "WARN", __VA_ARGS__)
#define LOG_INFO_(...)         LOG(0, LOG_LEVEL_INFO, "INFO", __VA_ARGS__)
#define LOG_DBG_(...)          LOG(0, LOG_LEVEL_DBG, "DBG", __VA_ARGS__)

/* For testing log level */
#define LOG_ERR_ENABLED        ((LOG_LEVEL) >= (LOG_LEVEL_ERR))
#define LOG_WARN_ENABLED       ((LOG_LEVEL) >= (LOG_LEVEL_WARN))
#define LOG_INFO_ENABLED       ((LOG_LEVEL) >= (LOG_LEVEL_INFO))
#define LOG_DBG_ENABLED        ((LOG_LEVEL) >= (LOG_LEVEL_DBG))

#endif /* COAP_LOG_CONF_H_ */
