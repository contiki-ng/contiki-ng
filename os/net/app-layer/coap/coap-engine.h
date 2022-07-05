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
 *      CoAP engine implementation.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

/**
 * \addtogroup coap
 * @{
 */

#ifndef COAP_ENGINE_H_
#define COAP_ENGINE_H_

typedef struct coap_resource_s coap_resource_t;
typedef struct coap_periodic_resource_s coap_periodic_resource_t;

#include "coap.h"
#include "coap-timer.h"

typedef enum {
  COAP_HANDLER_STATUS_CONTINUE,
  COAP_HANDLER_STATUS_PROCESSED
} coap_handler_status_t;

typedef coap_handler_status_t
(* coap_handler_callback_t)(coap_message_t *request,
                            coap_message_t *response,
                            uint8_t *buffer, uint16_t buffer_size,
                            int32_t *offset);

typedef struct coap_handler coap_handler_t;

struct coap_handler {
  coap_handler_t *next;
  coap_handler_callback_t handler;
};

#define COAP_HANDLER(name, handler) \
  coap_handler_t name = { NULL, handler }

void coap_add_handler(coap_handler_t *handler);
void coap_remove_handler(coap_handler_t *handler);

void coap_engine_init(void);

int coap_receive(const coap_endpoint_t *src,
                 uint8_t *payload, uint16_t payload_length);

coap_handler_status_t coap_call_handlers(coap_message_t *request,
                                         coap_message_t *response,
                                         uint8_t *buffer,
                                         uint16_t buffer_size,
                                         int32_t *offset);
/*---------------------------------------------------------------------------*/
/* signatures of handler functions */
typedef void (* coap_resource_handler_t)(coap_message_t *request,
                                         coap_message_t *response,
                                         uint8_t *buffer,
                                         uint16_t preferred_size,
                                         int32_t *offset);
typedef void (* coap_resource_periodic_handler_t)(void);
typedef void (* coap_resource_response_handler_t)(void *data,
                                                  coap_message_t *response);
typedef void (* coap_resource_trigger_handler_t)(void);

/* data structure representing a resource in CoAP */
struct coap_resource_s {
  coap_resource_t *next;            /* for LIST, points to next resource defined */
  const char *url;                  /*handled URL */
  coap_resource_flags_t flags;      /* handled CoAP methods */
  const char *attributes;           /* link-format attributes */
  coap_resource_handler_t get_handler;    /* handler function */
  coap_resource_handler_t post_handler;   /* handler function */
  coap_resource_handler_t put_handler;    /* handler function */
  coap_resource_handler_t delete_handler; /* handler function */
  union {
    coap_periodic_resource_t *periodic;  /* special data depending on flags */
    coap_resource_trigger_handler_t trigger;
    coap_resource_trigger_handler_t resume;
  };
};

struct coap_periodic_resource_s {
  uint32_t period;
  coap_timer_t periodic_timer;
  const coap_resource_periodic_handler_t periodic_handler;
};

/*
 * Macro to define a CoAP resource.
 * Resources are statically defined for the sake of efficiency and better memory management.
 */
#define RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler) \
  coap_resource_t name = { NULL, NULL, NO_FLAGS, attributes, get_handler, post_handler, put_handler, delete_handler, { NULL } }

#define PARENT_RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler) \
  coap_resource_t name = { NULL, NULL, HAS_SUB_RESOURCES, attributes, get_handler, post_handler, put_handler, delete_handler, { NULL } }

#define SEPARATE_RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler, resume_handler) \
  coap_resource_t name = { NULL, NULL, IS_SEPARATE, attributes, get_handler, post_handler, put_handler, delete_handler, { .resume = resume_handler } }

#define EVENT_RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler, event_handler) \
  coap_resource_t name = { NULL, NULL, IS_OBSERVABLE, attributes, get_handler, post_handler, put_handler, delete_handler, { .trigger = event_handler } }

/*
 * Macro to define a periodic resource.
 * The corresponding [name]_periodic_handler() function will be called every period.
 * For instance polling a sensor and publishing a changed value to subscribed clients would be done there.
 */
#define PERIODIC_RESOURCE(name, attributes, get_handler, post_handler, put_handler, delete_handler, period, periodic_handler) \
  static coap_periodic_resource_t periodic_##name = { period, { 0 }, periodic_handler }; \
  coap_resource_t name = { NULL, NULL, IS_OBSERVABLE | IS_PERIODIC, attributes, get_handler, post_handler, put_handler, delete_handler, { .periodic = &periodic_##name } }

/*---------------------------------------------------------------------------*/
/**
 *
 * \brief      Resources wanted to be accessible should be activated with the following code.
 * \param resource
 *             A CoAP resource defined through the RESOURCE macros.
 * \param path
 *             The local URI path where to provide the resource.
 */
void coap_activate_resource(coap_resource_t *resource, const char *path);
/*---------------------------------------------------------------------------*/
/**
 * \brief      Returns the first of the registered CoAP resources.
 * \return     The first registered CoAP resource or NULL if none exists.
 */
coap_resource_t *coap_get_first_resource(void);
/*---------------------------------------------------------------------------*/
/**
 * \brief      Returns the next registered CoAP resource.
 * \return     The next resource or NULL if no more exists.
 */
coap_resource_t *coap_get_next_resource(coap_resource_t *resource);
/*---------------------------------------------------------------------------*/

#include "coap-transactions.h"
#include "coap-observe.h"
#include "coap-separate.h"
#include "coap-observe-client.h"
#include "coap-transport.h"

#endif /* COAP_ENGINE_H_ */
/** @} */
