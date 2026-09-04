/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
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
 */

/**
 * \file
 *      /.well-known/core resource implementation.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

/**
 * \addtogroup coap
 * @{
 */

#include "coap-engine.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "coap"
#define LOG_LEVEL  LOG_LEVEL_COAP

#define ADD_CHAR_IF_POSSIBLE(char) \
  if(strpos >= *offset && bufpos < preferred_size) { \
    buffer[bufpos++] = char; \
  } \
  ++strpos

#define ADD_STRING_IF_POSSIBLE(string, op) \
  tmplen = strlen(string); \
  if(strpos + tmplen > *offset) { \
    bufpos += snprintf((char *)buffer + bufpos, \
                       preferred_size - bufpos + 1, \
                       "%s", \
                       string \
                       + (*offset - (int32_t)strpos > 0 ? \
                          *offset - (int32_t)strpos : 0)); \
    if(bufpos op preferred_size) { \
      LOG_DBG("BREAK at %s (%p)\n", string, resource);      \
      break; \
    } \
  } \
  strpos += tmplen

/*---------------------------------------------------------------------------*/
#if COAP_LINK_FORMAT_FILTERING
/*
 * Find the first occurrence of the first needle_len bytes of needle in the
 * NUL-terminated haystack. The needle is part of the received message and is
 * not terminated itself, which is why strstr() cannot be used.
 */
static const char *
find_substring(const char *haystack, const char *needle, size_t needle_len)
{
  for(; *haystack != '\0'; haystack++) {
    if(strncmp(haystack, needle, needle_len) == 0) {
      return haystack;
    }
  }

  return NULL;
}
#endif /* COAP_LINK_FORMAT_FILTERING */
/*---------------------------------------------------------------------------*/
/*- Resource Handlers -------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static void
well_known_core_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size,
                            int32_t *offset)
{
  size_t strpos = 0;            /* position in overall string (which is larger than the buffer) */
  size_t bufpos = 0;            /* position within buffer (bytes written) */
  size_t tmplen = 0;
  coap_resource_t *resource = NULL;

#if COAP_LINK_FORMAT_FILTERING
  /* For filtering. */
  const char *filter = NULL;
  const char *attrib = NULL;
  const char *found = NULL;
  const char *end = NULL;
  const char *value = NULL;
  const char *separator = NULL;
  size_t name_len = 0;
  /* The value is matched without its last character, which is compared on
     its own so that a '*' there matches anything. */
  size_t prefix_len = 0;
  bool at_url_start = false;
  bool is_href = false;
  char lastchar = '\0';
  int len = coap_get_header_uri_query(request, &filter);

  if(len) {
    separator = memchr(filter, '=', len);
    if(separator == NULL || separator + 1 >= filter + len) {
      /* A filter is a name, an equals sign and a value. */
      LOG_WARN("Filter without a value\n");
      coap_set_status_code(response, BAD_REQUEST_4_00);
      return;
    }

    name_len = separator - filter;
    value = separator + 1;
    len -= name_len + 1;

    LOG_DBG("Filter ");
    LOG_DBG_COAP_STRING(filter, name_len);
    LOG_DBG_(" = ");
    LOG_DBG_COAP_STRING(value, len);
    LOG_DBG_("\n");

    is_href = name_len == 4 && memcmp(filter, "href", 4) == 0;
    if(is_href && value[0] == '/') {
      ++value;
      --len;
      at_url_start = true;
    }

    if(len == 0) {
      LOG_WARN("Filter with an empty value\n");
      coap_set_status_code(response, BAD_REQUEST_4_00);
      return;
    }

    lastchar = value[len - 1];
    prefix_len = len - 1;
  }
#endif /* COAP_LINK_FORMAT_FILTERING */

  for(resource = coap_get_first_resource(); resource;
      resource = coap_get_next_resource(resource)) {
#if COAP_LINK_FORMAT_FILTERING
    /* Filtering */
    if(len) {
      if(is_href) {
        attrib = find_substring(resource->url, value, prefix_len);
        if(attrib == NULL || (at_url_start && attrib != resource->url)) {
          continue;
        }
        end = attrib + strlen(attrib);
      } else if(resource->attributes != NULL) {
        attrib = find_substring(resource->attributes, filter, name_len);
        if(attrib == NULL
           || (attrib[name_len] != '=' && attrib[name_len] != '"')) {
          continue;
        }
        attrib += name_len + 2;
        end = strchr(attrib, '"');
      } else {
        continue;
      }

      LOG_DBG("Filter: res has attrib %s\n", attrib);
      found = attrib;
      while((found = find_substring(found, value, prefix_len)) != NULL) {
        if(found > end) {
          found = NULL;
          break;
        }
        if(lastchar == found[prefix_len] || lastchar == '*') {
          break;
        }
        ++found;
      }
      if(found == NULL) {
        continue;
      }
      LOG_DBG("Filter: res has prefix %s\n", found);
      if(lastchar != '*' && found[prefix_len + 1] != '"'
         && found[prefix_len + 1] != ' ' && found[prefix_len + 1] != '\0') {
        continue;
      }
      LOG_DBG("Filter: res has match\n");
    }
#endif

    LOG_DBG("/%s (%p)\npos: s%zu, o%ld, b%zu\n", resource->url, resource,
            strpos, (long)*offset, bufpos);

    if(strpos > 0) {
      ADD_CHAR_IF_POSSIBLE(',');
    }
    ADD_CHAR_IF_POSSIBLE('<');
    ADD_CHAR_IF_POSSIBLE('/');
    ADD_STRING_IF_POSSIBLE(resource->url, >=);
    ADD_CHAR_IF_POSSIBLE('>');

    if(resource->attributes != NULL && resource->attributes[0]) {
      ADD_CHAR_IF_POSSIBLE(';');
      ADD_STRING_IF_POSSIBLE(resource->attributes, >);
    }

    /* buffer full, but resource not completed yet; or: do not break if resource exactly fills buffer. */
    if(bufpos > preferred_size && strpos - bufpos > *offset) {
      LOG_DBG("BREAK at %s (%p)\n", resource->url, resource);
      break;
    }
  }

  if(bufpos > 0) {
    LOG_DBG("BUF %zu: ", bufpos);
    LOG_DBG_COAP_STRING((char *)buffer, bufpos);
    LOG_DBG_("\n");

    coap_set_payload(response, buffer, bufpos);
    coap_set_header_content_format(response, APPLICATION_LINK_FORMAT);
  } else if(strpos > 0) {
    LOG_DBG("well_known_core_handler(): bufpos<=0\n");

    coap_set_status_code(response, BAD_OPTION_4_02);
    coap_set_payload(response, "BlockOutOfScope", 15);
  }

  if(resource == NULL) {
    LOG_DBG("DONE\n");
    *offset = -1;
  } else {
    LOG_DBG("MORE at %s (%p)\n", resource->url, resource);
    *offset += preferred_size;
  }
}
/*---------------------------------------------------------------------------*/
RESOURCE(res_well_known_core, "ct=40", well_known_core_get_handler, NULL,
         NULL, NULL);
/*---------------------------------------------------------------------------*/
/** @} */
