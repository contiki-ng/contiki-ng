/*
 * Copyright (c) 2015-2018, Yanzi Networks AB.
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
 *
 */

/**
 * \file
 *         Implementation of the Contiki OMA LWM2M server
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include <stdint.h>
#include "lib/list.h"
#include "lwm2m-object.h"
#include "lwm2m-engine.h"
#include "lwm2m-server.h"
#include "lwm2m-rd-client.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "lwm2m-srv"
#define LOG_LEVEL  LOG_LEVEL_LWM2M

#define MAX_COUNT LWM2M_SERVER_MAX_COUNT

static lwm2m_status_t lwm2m_callback(lwm2m_object_instance_t *object,
                                     lwm2m_context_t *ctx);

static lwm2m_object_instance_t *create_instance(uint16_t instance_id,
                                                lwm2m_status_t *status);
static int delete_instance(uint16_t instance_id, lwm2m_status_t *status);
static lwm2m_object_instance_t *get_first(lwm2m_status_t *status);
static lwm2m_object_instance_t *get_next(lwm2m_object_instance_t *instance,
                                         lwm2m_status_t *status);
static lwm2m_object_instance_t *get_by_id(uint16_t instance_id,
                                          lwm2m_status_t *status);

static const lwm2m_resource_id_t resources[] = {
  RO(LWM2M_SERVER_SHORT_SERVER_ID),
  RW(LWM2M_SERVER_LIFETIME_ID),
  EX(LWM2M_SERVER_REG_UPDATE_TRIGGER_ID)
};

static const lwm2m_object_impl_t impl = {
  .object_id = LWM2M_OBJECT_SERVER_ID,
  .get_first = get_first,
  .get_next = get_next,
  .get_by_id = get_by_id,
  .create_instance = create_instance,
  .delete_instance = delete_instance,
};
static lwm2m_object_t server_object = {
  .impl = &impl,
};

LIST(server_list);
static lwm2m_server_t server_instances[MAX_COUNT];
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t *
create_instance(uint16_t instance_id, lwm2m_status_t *status)
{
  lwm2m_object_instance_t *instance;
  int i;

  instance = get_by_id(instance_id, NULL);
  if(instance != NULL) {
    /* An instance with this id is already registered */
    if(status) {
      *status = LWM2M_STATUS_OPERATION_NOT_ALLOWED;
    }
    return NULL;
  }

  for(i = 0; i < MAX_COUNT; i++) {
    if(server_instances[i].instance.instance_id == LWM2M_OBJECT_INSTANCE_NONE) {
      server_instances[i].instance.callback = lwm2m_callback;
      server_instances[i].instance.object_id = LWM2M_OBJECT_SERVER_ID;
      server_instances[i].instance.instance_id = instance_id;
      server_instances[i].instance.resource_ids = resources;
      server_instances[i].instance.resource_count =
        sizeof(resources) / sizeof(lwm2m_resource_id_t);
      server_instances[i].server_id = 0;
      server_instances[i].lifetime = 0;
      list_add(server_list, &server_instances[i].instance);

      if(status) {
        *status = LWM2M_STATUS_OK;
      }

      return &server_instances[i].instance;
    }
  }

  if(status) {
    *status = LWM2M_STATUS_SERVICE_UNAVAILABLE;
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
static int
delete_instance(uint16_t instance_id, lwm2m_status_t *status)
{
  lwm2m_object_instance_t *instance;

  if(status) {
    *status = LWM2M_STATUS_OK;
  }

  if(instance_id == LWM2M_OBJECT_INSTANCE_NONE) {
    /* Remove all instances */
    while((instance = list_pop(server_list)) != NULL) {
      instance->instance_id = LWM2M_OBJECT_INSTANCE_NONE;
    }
    return 1;
  }

  instance = get_by_id(instance_id, NULL);
  if(instance != NULL) {
    instance->instance_id = LWM2M_OBJECT_INSTANCE_NONE;
    list_remove(server_list, instance);
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t *
get_first(lwm2m_status_t *status)
{
  if(status) {
    *status = LWM2M_STATUS_OK;
  }
  return list_head(server_list);
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t *
get_next(lwm2m_object_instance_t *instance, lwm2m_status_t *status)
{
  if(status) {
    *status = LWM2M_STATUS_OK;
  }
  return instance == NULL ? NULL : instance->next;
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t *
get_by_id(uint16_t instance_id, lwm2m_status_t *status)
{
  lwm2m_object_instance_t *instance;
  if(status) {
    *status = LWM2M_STATUS_OK;
  }
  for(instance = list_head(server_list);
      instance != NULL;
      instance = instance->next) {
    if(instance->instance_id == instance_id) {
      return instance;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
lwm2m_callback(lwm2m_object_instance_t *object,
               lwm2m_context_t *ctx)
{
  int32_t value;
  lwm2m_server_t *server;
  server = (lwm2m_server_t *) object;

  if(ctx->operation == LWM2M_OP_WRITE) {
    LOG_DBG("Write to: %d\n", ctx->resource_id);
    switch(ctx->resource_id) {
    case LWM2M_SERVER_LIFETIME_ID:
      lwm2m_object_read_int(ctx, ctx->inbuf->buffer, ctx->inbuf->size, &value);
      server->lifetime = value;
      break;
    }
  } else if(ctx->operation == LWM2M_OP_READ) {
    switch(ctx->resource_id) {
    case LWM2M_SERVER_SHORT_SERVER_ID:
      lwm2m_object_write_int(ctx, server->server_id);
      break;
    case LWM2M_SERVER_LIFETIME_ID:
      lwm2m_object_write_int(ctx, server->lifetime);
      break;
    }
  } else if(ctx->operation == LWM2M_OP_EXECUTE) {
    switch(ctx->resource_id) {
    case LWM2M_SERVER_REG_UPDATE_TRIGGER_ID:
      lwm2m_rd_client_update_triggered(ctx->request->src_ep);
      break;
    }
  } else {
    return LWM2M_STATUS_NOT_IMPLEMENTED;
  }

  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
lwm2m_server_t *
lwm2m_server_add(uint16_t instance_id, uint16_t server_id, uint32_t lifetime)
{
  lwm2m_server_t *server;
  int i;

  for(server = list_head(server_list);
      server;
      server = (lwm2m_server_t *)server->instance.next) {
    if(server->server_id == server_id) {
      /* Found a matching server */
      if(server->instance.instance_id != instance_id) {
        /* Non-matching instance id */
        LOG_DBG("non-matching instance id for server %u\n", server_id);
        return NULL;
      }
      server->lifetime = lifetime;
      return server;
    } else if(server->instance.instance_id == instance_id) {
      /* Right instance but wrong server id */
      LOG_DBG("non-matching server id for instance %u\n", instance_id);
      return NULL;
    }
  }

  for(i = 0; i < MAX_COUNT; i++) {
    if(server_instances[i].instance.instance_id == LWM2M_OBJECT_INSTANCE_NONE) {
      server_instances[i].instance.callback = lwm2m_callback;
      server_instances[i].instance.object_id = LWM2M_OBJECT_SERVER_ID;
      server_instances[i].instance.instance_id = instance_id;
      server_instances[i].instance.resource_ids = resources;
      server_instances[i].instance.resource_count =
        sizeof(resources) / sizeof(lwm2m_resource_id_t);
      server_instances[i].server_id = server_id;
      server_instances[i].lifetime = lifetime;
      list_add(server_list, &server_instances[i].instance);

      return &server_instances[i];
    }
  }

  LOG_WARN("no space for more servers\n");

  return NULL;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_server_init(void)
{
  int i;

  LOG_INFO("init\n");

  list_init(server_list);

  for(i = 0; i < MAX_COUNT; i++) {
    server_instances[i].instance.instance_id = LWM2M_OBJECT_INSTANCE_NONE;
  }
  lwm2m_engine_add_generic_object(&server_object);
}
/*---------------------------------------------------------------------------*/
/** @} */
