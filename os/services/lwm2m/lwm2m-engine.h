/*
 * Copyright (c) 2015, Yanzi Networks AB.
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
 * \addtogroup lwm2m
 * @{
 */

/**
 * \file
 *         Header file for the Contiki OMA LWM2M engine
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */

#ifndef LWM2M_ENGINE_H
#define LWM2M_ENGINE_H

#include "lwm2m-object.h"
#include "lwm2m-queue-mode-conf.h"

#define LWM2M_FLOAT32_BITS  10
#define LWM2M_FLOAT32_FRAC (1L << LWM2M_FLOAT32_BITS)

/* LWM2M / CoAP Content-Formats */
typedef enum {
  LWM2M_TEXT_PLAIN = 1541,
  LWM2M_TLV        = 11542,
  LWM2M_JSON       = 11543,
  LWM2M_OLD_TLV    = 1542,
  LWM2M_OLD_JSON   = 1543,
  LWM2M_OLD_OPAQUE  = 1544
} lwm2m_content_format_t;

void lwm2m_engine_init(void);

int lwm2m_engine_set_rd_data(lwm2m_buffer_t *outbuf, int block);

typedef lwm2m_status_t
(* lwm2m_object_instance_callback_t)(lwm2m_object_instance_t *object,
                                     lwm2m_context_t *ctx);
typedef int
(* lwm2m_resource_dim_callback_t)(lwm2m_object_instance_t *object,
                                  uint16_t resource_id);

#define LWM2M_OBJECT_INSTANCE_NONE 0xffff

struct lwm2m_object_instance {
  lwm2m_object_instance_t *next;
  uint16_t object_id;
  uint16_t instance_id;
  /* an array of resource IDs for discovery, etc */
  const lwm2m_resource_id_t *resource_ids;
  uint16_t resource_count;
  /* the callback for requests */
  lwm2m_object_instance_callback_t callback;
  lwm2m_resource_dim_callback_t resource_dim_callback;
};

typedef struct {
  uint16_t object_id;
  lwm2m_object_instance_t *(* create_instance)(uint16_t instance_id,
                                               lwm2m_status_t *status);
  int (* delete_instance)(uint16_t instance_id, lwm2m_status_t *status);
  lwm2m_object_instance_t *(* get_first)(lwm2m_status_t *status);
  lwm2m_object_instance_t *(* get_next)(lwm2m_object_instance_t *instance,
                                        lwm2m_status_t *status);
  lwm2m_object_instance_t *(* get_by_id)(uint16_t instance_id,
                                         lwm2m_status_t *status);
} lwm2m_object_impl_t;

typedef struct lwm2m_object lwm2m_object_t;
struct lwm2m_object {
  lwm2m_object_t *next;
  const lwm2m_object_impl_t *impl;
};

lwm2m_object_instance_t *lwm2m_engine_get_instance_buffer(void);

int  lwm2m_engine_has_instance(uint16_t object_id, uint16_t instance_id);
int  lwm2m_engine_add_object(lwm2m_object_instance_t *object);
void lwm2m_engine_remove_object(lwm2m_object_instance_t *object);
int  lwm2m_engine_add_generic_object(lwm2m_object_t *object);
void lwm2m_engine_remove_generic_object(lwm2m_object_t *object);
void lwm2m_notify_object_observers(lwm2m_object_instance_t *obj,
                                   uint16_t resource);

void lwm2m_engine_set_opaque_callback(lwm2m_context_t *ctx, lwm2m_write_opaque_callback cb);

#endif /* LWM2M_ENGINE_H */
/** @} */
