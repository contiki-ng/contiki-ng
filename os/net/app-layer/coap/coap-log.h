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
 *         Log support for CoAP
 * \author
 *         Niclas Finne <niclas.finne@ri.se>
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

/**
 * \addtogroup coap
 * @{
 */

#ifndef COAP_LOG_H_
#define COAP_LOG_H_

#include "contiki.h"

#ifdef COAP_LOG_CONF_PATH
#include COAP_LOG_CONF_PATH
#else /* COAP_LOG_CONF_PATH */
#include "sys/log.h"
#endif /* COAP_LOG_CONF_PATH */

#include "coap-endpoint.h"

/* CoAP endpoint */
#define LOG_COAP_EP(level, endpoint) do {                   \
    if(level <= (LOG_LEVEL)) {                              \
      coap_endpoint_log(endpoint);                          \
    }                                                       \
  } while (0)

#define LOG_ERR_COAP_EP(endpoint)  LOG_COAP_EP(LOG_LEVEL_ERR, endpoint)
#define LOG_WARN_COAP_EP(endpoint) LOG_COAP_EP(LOG_LEVEL_WARN, endpoint)
#define LOG_INFO_COAP_EP(endpoint) LOG_COAP_EP(LOG_LEVEL_INFO, endpoint)
#define LOG_DBG_COAP_EP(endpoint)  LOG_COAP_EP(LOG_LEVEL_DBG, endpoint)

/* CoAP strings */
#define LOG_COAP_STRING(level, text, len) do {              \
    if(level <= (LOG_LEVEL)) {                              \
      coap_log_string(text, len);                           \
    }                                                       \
  } while (0)

#define LOG_ERR_COAP_STRING(text, len)  LOG_COAP_STRING(LOG_LEVEL_ERR, text, len)
#define LOG_WARN_COAP_STRING(text, len) LOG_COAP_STRING(LOG_LEVEL_WARN, text, len)
#define LOG_INFO_COAP_STRING(text, len) LOG_COAP_STRING(LOG_LEVEL_INFO, text, len)
#define LOG_DBG_COAP_STRING(text, len)  LOG_COAP_STRING(LOG_LEVEL_DBG, text, len)

/**
 * \brief Logs a CoAP string that has a length specified, but might
 * not be zero-terminated.
 *
 * \param text The CoAP string.
 * \param len  The number of characters in the CoAP string.
 */
void coap_log_string(const char *text, size_t len);

#endif /* COAP_LOG_H_ */
/** @} */
