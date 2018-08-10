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
 */

/**
 * \file
 *         Implementation of the Contiki OMA LWM2M engine
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */

#include "lwm2m-engine.h"
#include "lwm2m-object.h"
#include "lwm2m-device.h"
#include "lwm2m-plain-text.h"
#include "lwm2m-json.h"
#include "coap-constants.h"
#include "coap-engine.h"
#include "lwm2m-tlv.h"
#include "lwm2m-tlv-reader.h"
#include "lwm2m-tlv-writer.h"
#include "lib/list.h"
#include "sys/cc.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#ifndef LWM2M_ENGINE_CLIENT_ENDPOINT_NAME
#include "net/ipv6/uip-ds6.h"
#endif /* LWM2M_ENGINE_CLIENT_ENDPOINT_NAME */

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "lwm2m-eng"
#define LOG_LEVEL  LOG_LEVEL_LWM2M

#ifndef LWM2M_ENGINE_CLIENT_ENDPOINT_PREFIX
#ifdef LWM2M_DEVICE_MODEL_NUMBER
#define LWM2M_ENGINE_CLIENT_ENDPOINT_PREFIX "Contiki-NG-"LWM2M_DEVICE_MODEL_NUMBER
#else /* LWM2M_DEVICE_MODEL_NUMBER */
#define LWM2M_ENGINE_CLIENT_ENDPOINT_PREFIX "Contiki-NG"
#endif /* LWM2M_DEVICE_MODEL_NUMBER */
#endif /* LWM2M_ENGINE_CLIENT_ENDPOINT_PREFIX */

#ifdef LWM2M_ENGINE_CONF_USE_RD_CLIENT
#define USE_RD_CLIENT LWM2M_ENGINE_CONF_USE_RD_CLIENT
#else
#define USE_RD_CLIENT 1
#endif /* LWM2M_ENGINE_CONF_USE_RD_CLIENT */


#if LWM2M_QUEUE_MODE_ENABLED
 /* Queue Mode is handled using the RD Client and the Q-Mode object */
#define USE_RD_CLIENT 1
#endif

#if USE_RD_CLIENT
#include "lwm2m-rd-client.h"
#endif

#if LWM2M_QUEUE_MODE_ENABLED
#include "lwm2m-queue-mode.h"
#include "lwm2m-notification-queue.h"
#if LWM2M_QUEUE_MODE_OBJECT_ENABLED
#include "lwm2m-queue-mode-object.h"
#endif /* LWM2M_QUEUE_MODE_OBJECT_ENABLED */
#endif /* LWM2M_QUEUE_MODE_ENABLED */

/* MACRO for getting out resource ID from resource array ID + flags */
#define RSC_ID(x)       ((uint16_t)(x & 0xffff))
#define RSC_READABLE(x) ((x & LWM2M_RESOURCE_READ) > 0)
#define RSC_WRITABLE(x) ((x & LWM2M_RESOURCE_WRITE) > 0)
#define RSC_UNSPECIFIED(x) ((x & LWM2M_RESOURCE_OP_MASK) == 0)

/* invalid instance ID - ffff object ID */
#define NO_INSTANCE 0xffffffff

/* This is a double-buffer for generating BLOCKs in CoAP - the idea
   is that typical LWM2M resources will fit 1 block unless they themselves
   handle BLOCK transfer - having a double sized buffer makes it possible
   to allow writing more than one block before sending the full block.
   The RFC seems to indicate that all blocks execept the last one should
   be full.
*/
static uint8_t d_buf[COAP_MAX_BLOCK_SIZE * 2];
static lwm2m_buffer_t lwm2m_buf = {
  .len = 0, .size =  COAP_MAX_BLOCK_SIZE * 2, .buffer = d_buf
};
static lwm2m_object_instance_t instance_buffer;

/* obj-id / ... */
static uint16_t lwm2m_buf_lock[4];
static uint64_t lwm2m_buf_lock_timeout = 0;

static lwm2m_write_opaque_callback current_opaque_callback;
static int current_opaque_offset = 0;

static coap_handler_status_t lwm2m_handler_callback(coap_message_t *request,
                                                    coap_message_t *response,
                                                    uint8_t *buffer,
                                                    uint16_t buffer_size,
                                                    int32_t *offset);
static lwm2m_object_instance_t *
next_object_instance(const lwm2m_context_t *context, lwm2m_object_t *object, lwm2m_object_instance_t *last);

static struct {
  uint16_t object_id;
  uint16_t instance_id;
  uint16_t token_len;
  uint8_t token[COAP_TOKEN_LEN];
  /* in the future also a timeout */
} created;

COAP_HANDLER(lwm2m_handler, lwm2m_handler_callback);
LIST(object_list);
LIST(generic_object_list);

/*---------------------------------------------------------------------------*/
static lwm2m_object_t *
get_object(uint16_t object_id)
{
  lwm2m_object_t *object;
  for(object = list_head(generic_object_list);
      object != NULL;
      object = object->next) {
    if(object->impl && object->impl->object_id == object_id) {
      return object;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static int
has_non_generic_object(uint16_t object_id)
{
  lwm2m_object_instance_t *instance;
  for(instance = list_head(object_list);
      instance != NULL;
      instance = instance->next) {
    if(instance->object_id == object_id) {
      return 1;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t *
get_instance(uint16_t object_id, uint16_t instance_id, lwm2m_object_t **o)
{
  lwm2m_object_instance_t *instance;
  lwm2m_object_t *object;

  if(o) {
    *o = NULL;
  }

  for(instance = list_head(object_list);
      instance != NULL;
      instance = instance->next) {
    if(instance->object_id == object_id) {
      if(instance->instance_id == instance_id ||
         instance_id == LWM2M_OBJECT_INSTANCE_NONE) {
        return instance;
      }
    }
  }

  object = get_object(object_id);
  if(object != NULL) {
    if(o) {
      *o = object;
    }
    if(instance_id == LWM2M_OBJECT_INSTANCE_NONE) {
      return object->impl->get_first(NULL);
    }
    return object->impl->get_by_id(instance_id, NULL);
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t *
get_instance_by_context(const lwm2m_context_t *context, lwm2m_object_t **o)
{
  if(context->level < 2) {
    return get_instance(context->object_id, LWM2M_OBJECT_INSTANCE_NONE, o);
  }
  return get_instance(context->object_id, context->object_instance_id, o);
}
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
call_instance(lwm2m_object_instance_t *instance, lwm2m_context_t *context)
{
  if(context->level < 3) {
    return LWM2M_STATUS_OPERATION_NOT_ALLOWED;
  }

  if(instance == NULL) {
    /* No instance */
    return LWM2M_STATUS_NOT_FOUND;
  }

  if(instance->callback == NULL) {
    return LWM2M_STATUS_ERROR;
  }

  return instance->callback(instance, context);
}
/*---------------------------------------------------------------------------*/
/* This is intended to switch out a block2 transfer buffer
 * It assumes that ctx containts the double buffer and that the outbuf is to
 * be the new buffer in ctx.
 */
static int
double_buffer_flush(lwm2m_buffer_t *ctxbuf, lwm2m_buffer_t *outbuf, int size)
{
  /* Copy the data from the double buffer in ctx to the outbuf and move data */
  /* If the buffer is less than size - we will output all and get remaining down
     to zero */
  if(ctxbuf->len < size) {
    size = ctxbuf->len;
  }
  if(ctxbuf->len >= size && outbuf->size >= size) {
    LOG_DBG("Double buffer - copying out %d bytes remaining: %d\n",
            size, ctxbuf->len - size);
    memcpy(outbuf->buffer, ctxbuf->buffer, size);
    memcpy(ctxbuf->buffer, &ctxbuf->buffer[size],
           ctxbuf->len - size);
    ctxbuf->len -= size;
    outbuf->len = size;
    return outbuf->len;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static inline const char *
get_method_as_string(coap_resource_flags_t method)
{
  if(method == METHOD_GET) {
    return "GET";
  } else if(method == METHOD_POST) {
    return "POST";
  } else if(method == METHOD_PUT) {
    return "PUT";
  } else if(method == METHOD_DELETE) {
    return "DELETE";
  } else {
    return "UNKNOWN";
  }
}
/*--------------------------------------------------------------------------*/
static const char *
get_status_as_string(lwm2m_status_t status)
{
  static char buffer[8];
  switch(status) {
  case LWM2M_STATUS_OK:
    return "OK";
  case LWM2M_STATUS_ERROR:
    return "ERROR";
  case LWM2M_STATUS_WRITE_ERROR:
    return "WRITE ERROR";
  case LWM2M_STATUS_READ_ERROR:
    return "READ ERROR";
  case LWM2M_STATUS_BAD_REQUEST:
    return "BAD REQUEST";
  case LWM2M_STATUS_UNAUTHORIZED:
    return "UNAUTHORIZED";
  case LWM2M_STATUS_FORBIDDEN:
    return "FORBIDDEN";
  case LWM2M_STATUS_NOT_FOUND:
    return "NOT FOUND";
  case LWM2M_STATUS_OPERATION_NOT_ALLOWED:
    return "OPERATION NOT ALLOWED";
  case LWM2M_STATUS_NOT_ACCEPTABLE:
    return "NOT ACCEPTABLE";
  case LWM2M_STATUS_UNSUPPORTED_CONTENT_FORMAT:
    return "UNSUPPORTED CONTENT FORMAT";
  case LWM2M_STATUS_NOT_IMPLEMENTED:
    return "NOT IMPLEMENTED";
  case LWM2M_STATUS_SERVICE_UNAVAILABLE:
    return "SERVICE UNAVAILABLE";
  default:
    snprintf(buffer, sizeof(buffer) - 1, "<%u>", status);
    return buffer;
  }
}
/*--------------------------------------------------------------------------*/
static int
parse_path(const char *path, int path_len,
           uint16_t *oid, uint16_t *iid, uint16_t *rid)
{
  int ret;
  int pos;
  uint16_t val;
  char c = 0;

  /* get object id */
  LOG_DBG("parse PATH: \"");
  LOG_DBG_COAP_STRING(path, path_len);
  LOG_DBG_("\"\n");

  ret = 0;
  pos = 0;
  do {
    val = 0;
    /* we should get a value first - consume all numbers */
    while(pos < path_len && (c = path[pos]) >= '0' && c <= '9') {
      val = val * 10 + (c - '0');
      pos++;
    }
    /* Slash will mote thing forward - and the end will be when pos == pl */
    if(c == '/' || pos == path_len) {
      /* PRINTF("Setting %u = %u\n", ret, val); */
      if(ret == 0) *oid = val;
      if(ret == 1) *iid = val;
      if(ret == 2) *rid = val;
      ret++;
      pos++;
    } else {
      /* PRINTF("Error: illegal char '%c' at pos:%d\n", c, pos); */
      return -1;
    }
  } while(pos < path_len);
  return ret;
}
/*--------------------------------------------------------------------------*/
static int
lwm2m_engine_parse_context(const char *path, int path_len,
                           coap_message_t *request, coap_message_t *response,
                           uint8_t *outbuf, size_t outsize,
                           lwm2m_context_t *context)
{
  int ret;
  if(context == NULL || path == NULL) {
    return 0;
  }

  ret = parse_path(path, path_len, &context->object_id,
                   &context->object_instance_id, &context->resource_id);

  if(ret > 0) {
    context->level = ret;
  }

  return ret;
}

/*---------------------------------------------------------------------------*/
void lwm2m_engine_set_opaque_callback(lwm2m_context_t *ctx, lwm2m_write_opaque_callback cb)
{
  /* Here we should set the callback for the opaque that we are currently generating... */
  /* And we should in the future associate the callback with the CoAP message info - MID */
  LOG_DBG("Setting opaque handler - offset: %"PRIu32",%d\n",
          ctx->offset, ctx->outbuf->len);

  current_opaque_offset = 0;
  current_opaque_callback = cb;
}
/*---------------------------------------------------------------------------*/
int
lwm2m_engine_set_rd_data(lwm2m_buffer_t *outbuf, int block)
{
  /* remember things here - need to lock lwm2m buffer also!!! */
  static lwm2m_object_t *object;
  static lwm2m_object_instance_t *instance;
  int len;
  /* pick size from outbuf */
  int maxsize = outbuf->size;

  if(lwm2m_buf_lock[0] != 0 && (lwm2m_buf_lock_timeout > coap_timer_uptime()) &&
     ((lwm2m_buf_lock[1] != 0xffff) ||
      (lwm2m_buf_lock[2] != 0xffff))) {
    LOG_DBG("Set-RD: already exporting resource: %d/%d/%d\n",
            lwm2m_buf_lock[1], lwm2m_buf_lock[2], lwm2m_buf_lock[3]);
    /* fail - what should we return here? */
    return 0;
  }

  if(block == 0) {
    LOG_DBG("Starting RD generation\n");
    /* start with simple object instances */
    instance = list_head(object_list);
    object = NULL;

    if(instance == NULL) {
      /* No simple object instances available */
      object = list_head(generic_object_list);
      if(object == NULL) {
        /* No objects of any kind available */
        return 0;
      }
      if(object->impl != NULL) {
        instance = object->impl->get_first(NULL);
      }
    }

    lwm2m_buf_lock[0] = 1; /* lock "flag" */
    lwm2m_buf_lock[1] = 0xffff;
    lwm2m_buf_lock[2] = 0xffff;
    lwm2m_buf_lock[3] = 0xffff;
  } else {
    /* object and instance was static... */
  }

  lwm2m_buf_lock_timeout = coap_timer_uptime() + 1000;

  LOG_DBG("Generating RD list:");
  while(instance != NULL || object != NULL) {
    int pos = lwm2m_buf.len;
    if(instance != NULL) {
      len = snprintf((char *) &lwm2m_buf.buffer[pos],
                     lwm2m_buf.size - pos, (pos > 0 || block > 0) ? ",</%d/%d>" : "</%d/%d>",
                     instance->object_id, instance->instance_id);
      LOG_DBG_((pos > 0 || block > 0) ? ",</%d/%d>" : "</%d/%d>",
               instance->object_id, instance->instance_id);
    } else if(object->impl != NULL) {
      len = snprintf((char *) &lwm2m_buf.buffer[pos],
                     lwm2m_buf.size - pos,
                     (pos > 0 || block > 0) ? ",</%d>" : "</%d>",
                     object->impl->object_id);
      LOG_DBG_((pos > 0 || block > 0) ? ",</%d>" : "</%d>",
               object->impl->object_id);
    } else {
      len = 0;
    }
    lwm2m_buf.len += len;
    if(instance != NULL) {
      instance = next_object_instance(NULL, object, instance);
    }

    if(instance == NULL) {
      if(object == NULL) {
        /*
         * No object and no instance - we are done with simple object instances.
         */
        object = list_head(generic_object_list);
      } else {
        /*
         * Object exists but not an instance - instances for this object are
         * done - go to next.
         */
        object = object->next;
      }

      if(object != NULL && object->impl != NULL) {
        instance = object->impl->get_first(NULL);
      }

      if(instance == NULL && object == NULL && lwm2m_buf.len <= maxsize) {
        /* Data generation is done. No more messages are needed after this. */
        break;
      }
    }

    if(lwm2m_buf.len >= maxsize) {
      LOG_DBG_("\n");
      LOG_DBG("**** CoAP MAX BLOCK Reached!!! **** SEND\n");
      /* If the produced data is larger than a CoAP block we need to send
         this now */
      double_buffer_flush(&lwm2m_buf, outbuf, maxsize);
      /* there will be more - keep lock! */
      return 1;
    }
  }
  LOG_DBG_("\n");
  double_buffer_flush(&lwm2m_buf, outbuf, maxsize);
  /* unlock the buffer */
  lwm2m_buf_lock[0] = 0;
  return 0;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_engine_init(void)
{
  list_init(object_list);
  list_init(generic_object_list);

#ifdef LWM2M_ENGINE_CLIENT_ENDPOINT_NAME
  const char *endpoint = LWM2M_ENGINE_CLIENT_ENDPOINT_NAME;

#else /* LWM2M_ENGINE_CLIENT_ENDPOINT_NAME */
  static char endpoint[32];
  int len, i;
  uint8_t state;
  uip_ipaddr_t *ipaddr;

  len = strlen(LWM2M_ENGINE_CLIENT_ENDPOINT_PREFIX);
  /* ensure that this fits with the hex-nums */
  if(len > sizeof(endpoint) - 13) {
    len = sizeof(endpoint) - 13;
  }

  for(i = 0; i < len; i++) {
    if(LWM2M_ENGINE_CLIENT_ENDPOINT_PREFIX[i] == ' ') {
      endpoint[i] = '-';
    } else {
      endpoint[i] = LWM2M_ENGINE_CLIENT_ENDPOINT_PREFIX[i];
    }
  }
  /* pick an IP address that is PREFERRED or TENTATIVE */
  ipaddr = NULL;
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      ipaddr = &(uip_ds6_if.addr_list[i]).ipaddr;
      break;
    }
  }

  if(ipaddr != NULL) {
    for(i = 0; i < 6; i++) {
      /* assume IPv6 for now */
      uint8_t b = ipaddr->u8[10 + i];
      endpoint[len++] = (b >> 4) > 9 ? 'A' - 10 + (b >> 4) : '0' + (b >> 4);
      endpoint[len++] = (b & 0xf) > 9 ? 'A' - 10 + (b & 0xf) : '0' + (b & 0xf);
    }
  }

  /* a zero at end of string */
  endpoint[len] = 0;

#endif /* LWM2M_ENGINE_CLIENT_ENDPOINT_NAME */

  /* Initialize CoAP engine. Contiki-NG already does that from the main,
   * but for standalone use of lwm2m, this is required here. coap_engine_init()
   * checks for double-initialization and can be called twice safely. */
  coap_engine_init();

  /* Register the CoAP handler for lightweight object handling */
  coap_add_handler(&lwm2m_handler);

#if USE_RD_CLIENT
  lwm2m_rd_client_init(endpoint);
#endif

#if LWM2M_QUEUE_MODE_ENABLED && LWM2M_QUEUE_MODE_OBJECT_ENABLED
  lwm2m_queue_mode_object_init();
#endif
}
/*---------------------------------------------------------------------------*/
/*
 * Set the writer pointer to the proper writer based on the Accept: header
 *
 * param[in] context  LWM2M context to operate on
 * param[in] accept   Accept type number from CoAP headers
 *
 * return The content type of the response if the selected writer is used
 */
static unsigned int
lwm2m_engine_select_writer(lwm2m_context_t *context, unsigned int accept)
{
  switch(accept) {
    case LWM2M_TLV:
    case LWM2M_OLD_TLV:
      context->writer = &lwm2m_tlv_writer;
      break;
    case LWM2M_TEXT_PLAIN:
    case TEXT_PLAIN:
      context->writer = &lwm2m_plain_text_writer;
      break;
    case LWM2M_JSON:
    case LWM2M_OLD_JSON:
    case APPLICATION_JSON:
      context->writer = &lwm2m_json_writer;
      break;
    default:
      LOG_WARN("Unknown Accept type %u, using LWM2M plain text\n", accept);
      context->writer = &lwm2m_plain_text_writer;
      /* Set the response type to plain text */
      accept = LWM2M_TEXT_PLAIN;
      break;
  }
  context->content_type = accept;
  return accept;
}
/*---------------------------------------------------------------------------*/
/*
 * Set the reader pointer to the proper reader based on the Content-format: header
 *
 * param[in] context        LWM2M context to operate on
 * param[in] content_format Content-type type number from CoAP headers
 */
static void
lwm2m_engine_select_reader(lwm2m_context_t *context, unsigned int content_format)
{
  switch(content_format) {
    case LWM2M_TLV:
    case LWM2M_OLD_TLV:
      context->reader = &lwm2m_tlv_reader;
      break;
    case LWM2M_JSON:
    case LWM2M_OLD_JSON:
      context->reader = &lwm2m_plain_text_reader;
      break;
    case LWM2M_TEXT_PLAIN:
    case TEXT_PLAIN:
      context->reader = &lwm2m_plain_text_reader;
      break;
    default:
      LOG_WARN("Unknown content type %u, using LWM2M plain text\n",
               content_format);
      context->reader = &lwm2m_plain_text_reader;
      break;
  }
}

/*---------------------------------------------------------------------------*/
/* Lightweight object instances */
/*---------------------------------------------------------------------------*/
static uint32_t last_instance_id = NO_INSTANCE;
static int last_rsc_pos;

/* Multi read will handle read of JSON / TLV or Discovery (Link Format) */
static lwm2m_status_t
perform_multi_resource_read_op(lwm2m_object_t *object,
                               lwm2m_object_instance_t *instance,
                               lwm2m_context_t *ctx)
{
  int size = ctx->outbuf->size;
  int len = 0;
  uint8_t initialized = 0; /* used for commas, etc */
  uint8_t num_read = 0;
  lwm2m_buffer_t *outbuf;

  if(instance == NULL) {
    /* No existing instance */
    return LWM2M_STATUS_NOT_FOUND;
  }

  if(ctx->level < 3 &&
     (ctx->content_type == LWM2M_TEXT_PLAIN ||
      ctx->content_type == TEXT_PLAIN ||
      ctx->content_type == LWM2M_OLD_OPAQUE)) {
    return LWM2M_STATUS_OPERATION_NOT_ALLOWED;
  }

  /* copy out the out-buffer as read will use its own - will be same for disoc when
     read is fixed */
  outbuf = ctx->outbuf;

  /* Currently we only handle one incoming read request at a time - so we return
     BUZY or service unavailable */
  if(lwm2m_buf_lock[0] != 0 && (lwm2m_buf_lock_timeout > coap_timer_uptime()) &&
     ((lwm2m_buf_lock[1] != ctx->object_id) ||
      (lwm2m_buf_lock[2] != ctx->object_instance_id) ||
      (lwm2m_buf_lock[3] != ctx->resource_id))) {
    LOG_DBG("Multi-read: already exporting resource: %d/%d/%d\n",
            lwm2m_buf_lock[1], lwm2m_buf_lock[2], lwm2m_buf_lock[3]);
    return LWM2M_STATUS_SERVICE_UNAVAILABLE;
  }

  LOG_DBG("MultiRead: %d/%d/%d lv:%d offset:%"PRIu32"\n",
          ctx->object_id, ctx->object_instance_id, ctx->resource_id,
          ctx->level, ctx->offset);

  /* Make use of the double buffer */
  ctx->outbuf = &lwm2m_buf;

  if(ctx->offset == 0) {
    /* First GET request - need to setup all buffers and reset things here */
    last_instance_id =
      ((uint32_t)instance->object_id << 16) | instance->instance_id;
    last_rsc_pos = 0;
    /* reset any callback */
    current_opaque_callback = NULL;
    /* reset lwm2m_buf_len - so that we can use the double-size buffer */
    lwm2m_buf_lock[0] = 1; /* lock "flag" */
    lwm2m_buf_lock[1] = ctx->object_id;
    lwm2m_buf_lock[2] = ctx->object_instance_id;
    lwm2m_buf_lock[3] = ctx->resource_id;
    lwm2m_buf.len = 0;
    /* Here we should print top node */
  } else {
    /* offset > 0 - assume that we are already in a disco or multi get*/
    instance = get_instance(last_instance_id >> 16, last_instance_id & 0xffff,
                            &object);

    /* we assume that this was initialized */
    initialized = 1;
    ctx->writer_flags |= WRITER_OUTPUT_VALUE;
    if(instance == NULL) {
      ctx->offset = -1;
      ctx->outbuf->buffer[0] = ' ';
    }
  }
  lwm2m_buf_lock_timeout = coap_timer_uptime() + 1000;

  while(instance != NULL) {
    /* Do the discovery or read */
    if(instance->resource_ids != NULL && instance->resource_count > 0) {
      /* show all the available resources (or read all) */
      while(last_rsc_pos < instance->resource_count) {
        LOG_DBG("READ: 0x%"PRIx32" 0x%x 0x%x lv:%d\n",
                instance->resource_ids[last_rsc_pos],
                RSC_ID(instance->resource_ids[last_rsc_pos]),
                ctx->resource_id, ctx->level);

        /* Check if this is a object read or if it is the correct resource */
        if(ctx->level < 3 || ctx->resource_id == RSC_ID(instance->resource_ids[last_rsc_pos])) {
          /* ---------- Discovery operation ------------- */
          /* If this is a discovery all the object, instance, and resource triples should be
             generted */
          if(ctx->operation == LWM2M_OP_DISCOVER) {
            int dim = 0;
            len = snprintf((char *) &ctx->outbuf->buffer[ctx->outbuf->len],
                           ctx->outbuf->size - ctx->outbuf->len,
                           (ctx->outbuf->len == 0 && ctx->offset == 0) ? "</%d/%d/%d>":",</%d/%d/%d>",
                           instance->object_id, instance->instance_id,
                           RSC_ID(instance->resource_ids[last_rsc_pos]));
            if(instance->resource_dim_callback != NULL &&
               (dim = instance->resource_dim_callback(instance,
                                                      RSC_ID(instance->resource_ids[last_rsc_pos]))) > 0) {
              len += snprintf((char *) &ctx->outbuf->buffer[ctx->outbuf->len + len],
                              ctx->outbuf->size - ctx->outbuf->len - len,  ";dim=%d", dim);
            }
            /* here we have "read" out something */
            num_read++;
            ctx->outbuf->len += len;
            if(len < 0 || ctx->outbuf->len >= size) {
              double_buffer_flush(ctx->outbuf, outbuf, size);

              LOG_DBG("Copied lwm2m buf - remaining: %d\n", lwm2m_buf.len);
              /* switch buffer */
              ctx->outbuf = outbuf;
              ctx->writer_flags |= WRITER_HAS_MORE;
              ctx->offset += size;
              return LWM2M_STATUS_OK;
            }
            /* ---------- Read operation ------------- */
          } else if(ctx->operation == LWM2M_OP_READ) {
            lwm2m_status_t success = 0;
            uint8_t lv;

            lv = ctx->level;

            /* Do not allow a read on a non-readable */
            if(lv == 3 && !RSC_READABLE(instance->resource_ids[last_rsc_pos])) {
              lwm2m_buf_lock[0] = 0;
              return LWM2M_STATUS_OPERATION_NOT_ALLOWED;
            }
            /* Set the resource ID is ctx->level < 3 */
            if(lv < 3) {
              ctx->resource_id = RSC_ID(instance->resource_ids[last_rsc_pos]);
            }
            if(lv < 2) {
              ctx->object_instance_id = instance->instance_id;
            }

            if(RSC_READABLE(instance->resource_ids[last_rsc_pos])) {
              ctx->level = 3;
              if(!initialized) {
                /* Now we need to initialize the object writing for this new object */
                len = ctx->writer->init_write(ctx);
                ctx->outbuf->len += len;
                LOG_DBG("INIT WRITE len:%d size:%"PRIu16"\n", len, ctx->outbuf->size);
                initialized = 1;
              }

              if(current_opaque_callback == NULL) {
                LOG_DBG("Doing the callback to the resource %d\n", ctx->outbuf->len);
                /* No special opaque callback to handle - use regular callback */
                success = instance->callback(instance, ctx);
                LOG_DBG("After the callback to the resource %d: %s\n",
                        ctx->outbuf->len, get_status_as_string(success));

                if(success != LWM2M_STATUS_OK) {
                  /* What to do here? */
                  LOG_DBG("Callback failed: %s\n", get_status_as_string(success));
                  if(lv < 3) {
                    if(success == LWM2M_STATUS_NOT_FOUND) {
                      /* ok with a not found during a multi read - what more
                         is ok? */
                    } else {
                      lwm2m_buf_lock[0] = 0;
                      return success;
                    }
                  } else {
                    lwm2m_buf_lock[0] = 0;
                    return success;
                  }
                }
              }
              if(current_opaque_callback != NULL) {
                uint32_t old_offset = ctx->offset;
                int num_write = COAP_MAX_BLOCK_SIZE - ctx->outbuf->len;
                /* Check if the callback did set a opaque callback function - then
                   we should produce data via that callback until the opaque has fully
                   been handled */
                ctx->offset = current_opaque_offset;
                /* LOG_DBG("Calling the opaque handler %x\n", ctx->writer_flags); */
                success = current_opaque_callback(instance, ctx, num_write);
                if((ctx->writer_flags & WRITER_HAS_MORE) == 0) {
                  /* This opaque stream is now done! */
                  /* LOG_DBG("Setting opaque callback to null - it is done!\n"); */
                  current_opaque_callback = NULL;
                } else if(ctx->outbuf->len < COAP_MAX_BLOCK_SIZE) {
                  lwm2m_buf_lock[0] = 0;
                  return LWM2M_STATUS_ERROR;
                }
                current_opaque_offset += num_write;
                ctx->offset = old_offset;
                /* LOG_DBG("Setting back offset to: %d\n", ctx->offset); */
              }

              /* here we have "read" out something */
              num_read++;
              /* We will need to handle no-success and other things */
              LOG_DBG("Called %u/%u/%u outlen:%u %s\n",
                      ctx->object_id, ctx->object_instance_id, ctx->resource_id,
                      ctx->outbuf->len, get_status_as_string(success));

              /* we need to handle full buffer, etc here also! */
              ctx->level = lv;
            } else {
              LOG_DBG("Resource %u not readable\n",
                      RSC_ID(instance->resource_ids[last_rsc_pos]));
            }
          }
        }
        if(current_opaque_callback == NULL) {
          /* This resource is now done - (only when the opaque is also done) */
          last_rsc_pos++;
        } else {
          LOG_DBG("Opaque is set - continue with that.\n");
        }

        if(ctx->outbuf->len >= COAP_MAX_BLOCK_SIZE) {
          LOG_DBG("**** CoAP MAX BLOCK Reached!!! **** SEND\n");
          /* If the produced data is larger than a CoAP block we need to send
             this now */
          if(ctx->outbuf->len < 2 * COAP_MAX_BLOCK_SIZE) {
            /* We assume that size is equal to COAP_MAX_BLOCK_SIZE here */
            double_buffer_flush(ctx->outbuf, outbuf, size);

            LOG_DBG("Copied lwm2m buf - remaining: %d\n", lwm2m_buf.len);
            /* switch buffer */
            ctx->outbuf = outbuf;
            ctx->writer_flags |= WRITER_HAS_MORE;
            ctx->offset += size;
            /* OK - everything went well... but we have more. - keep the lock here! */
            return LWM2M_STATUS_OK;
          } else {
            LOG_WARN("*** ERROR Overflow?\n");
            return LWM2M_STATUS_ERROR;
          }
        }
      }
    }
    instance = next_object_instance(ctx, object, instance);
    if(instance != NULL) {
      last_instance_id =
        ((uint32_t)instance->object_id << 16) | instance->instance_id;
    } else {
      last_instance_id = NO_INSTANCE;
    }
    if(ctx->operation == LWM2M_OP_READ) {
      LOG_DBG("END Writer %d ->", ctx->outbuf->len);
      len = ctx->writer->end_write(ctx);
      ctx->outbuf->len += len;
      LOG_DBG("%d\n", ctx->outbuf->len);
    }

    initialized = 0;
    last_rsc_pos = 0;
  }

  /* did not read anything even if we should have - on single item */
  if(num_read == 0 && ctx->level == 3) {
    lwm2m_buf_lock[0] = 0;
    return LWM2M_STATUS_NOT_FOUND;
  }

  /* seems like we are done! - flush buffer */
  len = double_buffer_flush(ctx->outbuf, outbuf, size);
  ctx->outbuf = outbuf;
  ctx->offset += len;

  /* If there is still data in the double-buffer - indicate that so that we get another
     callback */
  if(lwm2m_buf.len > 0) {
    ctx->writer_flags |= WRITER_HAS_MORE;
  } else {
    /* OK - everything went well we are done, unlock and return */
    lwm2m_buf_lock[0] = 0;
  }

  LOG_DBG("At END: Copied lwm2m buf %d\n", len);

  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t *
create_instance(lwm2m_context_t *context, lwm2m_object_t *object)
{
  lwm2m_object_instance_t *instance;
  if(object == NULL || object->impl == NULL ||
     object->impl->create_instance == NULL) {
    return NULL;
  }

  /* NOTE: context->object_instance_id needs to be set before calling */
  instance = object->impl->create_instance(context->object_instance_id, NULL);
  if(instance != NULL) {
    LOG_DBG("Created instance: %u/%u\n", context->object_id, context->object_instance_id);
    coap_set_status_code(context->response, CREATED_2_01);
#if USE_RD_CLIENT
    lwm2m_rd_client_set_update_rd();
#endif
  }
  return instance;
}
/*---------------------------------------------------------------------------*/
#define MODE_NONE      0
#define MODE_INSTANCE  1
#define MODE_VALUE     2
#define MODE_READY     3

static lwm2m_object_instance_t *
get_or_create_instance(lwm2m_context_t *ctx, lwm2m_object_t *object,
                       uint16_t *c)
{
  lwm2m_object_instance_t *instance;

  instance = get_instance_by_context(ctx, NULL);
  LOG_DBG("Instance: %u/%u/%u = %p\n", ctx->object_id,
          ctx->object_instance_id, ctx->resource_id, instance);
  /* by default we assume that the instance is not created... so we set flag to zero */
  if(c != NULL) {
    *c = LWM2M_OBJECT_INSTANCE_NONE;
  }
  if(instance == NULL) {
    instance = create_instance(ctx, object);
    if(instance != NULL) {
      if(c != NULL) {
        *c = instance->instance_id;
      }
      created.instance_id = instance->instance_id;
      created.object_id = instance->object_id;
      created.token_len = MIN(COAP_TOKEN_LEN, ctx->request->token_len);
      memcpy(&created.token, ctx->request->token, created.token_len);
    }
  }
  return instance;
}
/*---------------------------------------------------------------------------*/
static int
check_write(lwm2m_context_t *ctx, lwm2m_object_instance_t *instance, int rid)
{
  int i;
  if(instance->resource_ids != NULL && instance->resource_count > 0) {
    int count = instance->resource_count;
    for(i = 0; i < count; i++) {
      if(RSC_ID(instance->resource_ids[i]) == rid) {
        if(RSC_WRITABLE(instance->resource_ids[i])) {
          /* yes - writable */
          return 1;
        }
        if(RSC_UNSPECIFIED(instance->resource_ids[i]) &&
           created.instance_id == instance->instance_id &&
           created.object_id == instance->object_id &&
           created.token_len == ctx->request->token_len &&
           memcmp(&created.token, ctx->request->token,
                  created.token_len) == 0) {
          /* yes - writeable at create - never otherwise - sec / srv */
          return 1;
        }
        break;
      }
    }
  }
  /* Resource did not exist... - Ignore to avoid problems. */
  if(created.instance_id == instance->instance_id &&
     created.object_id == instance->object_id &&
     created.token_len == ctx->request->token_len &&
     memcmp(&created.token, ctx->request->token,
            created.token_len) == 0) {
    LOG_DBG("Ignoring resource %u/%u/%d in newly created instance\n",
            created.object_id, created.instance_id, rid);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
process_tlv_write(lwm2m_context_t *ctx, lwm2m_object_t *object,
                  int rid, uint8_t *data, int len)
{
  lwm2m_object_instance_t *instance;
  uint16_t created = LWM2M_OBJECT_INSTANCE_NONE;
  ctx->inbuf->buffer = data;
  ctx->inbuf->pos = 0;
  ctx->inbuf->size = len;
  ctx->level = 3;
  ctx->resource_id = rid;
  LOG_DBG("  Doing callback to %u/%u/%u\n", ctx->object_id,
          ctx->object_instance_id, ctx->resource_id);
  instance = get_or_create_instance(ctx, object, &created);
  if(instance != NULL && instance->callback != NULL) {
    if(check_write(ctx, instance, rid)) {
      return instance->callback(instance, ctx);
    } else {
      return LWM2M_STATUS_OPERATION_NOT_ALLOWED;
    }
  }
  return LWM2M_STATUS_ERROR;
}
/*---------------------------------------------------------------------------*/
static int last_tlv_id = 0;

static lwm2m_status_t
perform_multi_resource_write_op(lwm2m_object_t *object,
                                lwm2m_object_instance_t *instance,
                                lwm2m_context_t *ctx, int format)
{
  /* Only for JSON and TLV formats */
  uint16_t oid = 0, iid = 0, rid = 0;
  uint8_t olv = 0;
  uint8_t mode = 0;
  uint8_t *inbuf;
  int inpos;
  size_t insize;
  int i;
  uint16_t created = LWM2M_OBJECT_INSTANCE_NONE;

  olv = ctx->level;
  inbuf = ctx->inbuf->buffer;
  inpos = ctx->inbuf->pos;
  insize = ctx->inbuf->size;

  if(format == LWM2M_JSON || format == LWM2M_OLD_JSON) {
    struct json_data json;

    while(lwm2m_json_next_token(ctx, &json)) {
      LOG_DBG("JSON: '");
      for(i = 0; i < json.name_len; i++) {
        LOG_DBG_("%c", json.name[i]);
      }
      LOG_DBG_("':'");
      for(i = 0; i < json.value_len; i++) {
        LOG_DBG_("%c", json.value[i]);
      }
      LOG_DBG_("'\n");

      if(json.name[0] == 'n') {
        i = parse_path((const char *) json.value, json.value_len, &oid, &iid, &rid);
        if(i > 0) {
          if(ctx->level == 1) {
            ctx->level = 3;
            ctx->object_instance_id = oid;
            ctx->resource_id = iid;

            instance = get_or_create_instance(ctx, object, &created);
          }
          if(instance != NULL && instance->callback != NULL) {
            mode |= MODE_INSTANCE;
          } else {
            /* Failure... */
            return LWM2M_STATUS_ERROR;
          }
        }
      } else {
        /* HACK - assume value node - can it be anything else? */
        mode |= MODE_VALUE;
        /* update values */
        inbuf = ctx->inbuf->buffer;
        inpos = ctx->inbuf->pos;

        ctx->inbuf->buffer = json.value;
        ctx->inbuf->pos = 0;
        ctx->inbuf->size = json.value_len;
      }

      if(mode == MODE_READY) {
        /* allow write if just created - otherwise not */
        if(!check_write(ctx, instance, ctx->resource_id)) {
          return LWM2M_STATUS_OPERATION_NOT_ALLOWED;
        }
        if(instance->callback(instance, ctx) != LWM2M_STATUS_OK) {
          /* TODO what to do here */
        }
        mode = MODE_NONE;
        ctx->inbuf->buffer = inbuf;
        ctx->inbuf->pos = inpos;
        ctx->inbuf->size = insize;
        ctx->level = olv;
      }
    }
  } else if(format == LWM2M_TLV || format == LWM2M_OLD_TLV) {
    size_t len;
    lwm2m_tlv_t tlv;
    int tlvpos = 0;
    lwm2m_status_t status;

    /* For handling blockwise (BLOCK1) write */
    uint32_t num;
    uint8_t more;
    uint16_t size;
    uint32_t offset;

    /* NOTE: this assumes that a BLOCK1 non-first block is not a part of a
       small TLV but rather a large opaque - this needs to be fixed in the
       future */

    if(coap_get_header_block1(ctx->request, &num, &more, &size, &offset)) {
      LOG_DBG("CoAP BLOCK1: %"PRIu32"/%d/%d offset:%"PRIu32
              "  LWM2M CTX->offset=%"PRIu32"\n",
              num, more, size, offset, ctx->offset);
      LOG_DBG("Last TLV ID:%d final:%d\n", last_tlv_id,
              lwm2m_object_is_final_incoming(ctx));
      if(offset > 0) {
        status = process_tlv_write(ctx, object, last_tlv_id,
                                   inbuf, size);
        return status;
      }
    }

    while(tlvpos < insize) {
      len = lwm2m_tlv_read(&tlv, &inbuf[tlvpos], insize - tlvpos);
      LOG_DBG("Got TLV format First is: type:%d id:%d len:%d (p:%d len:%d/%d)\n",
             tlv.type, tlv.id, (int) tlv.length,
             (int) tlvpos, (int) len, (int) insize);
      if(tlv.type == LWM2M_TLV_TYPE_OBJECT_INSTANCE) {
        lwm2m_tlv_t tlv2;
        int len2;
        int pos = 0;
        ctx->object_instance_id = tlv.id;
        if(tlv.length == 0) {
          /* Create only - no data */
          if((instance = create_instance(ctx, object)) == NULL) {
            return LWM2M_STATUS_ERROR;
          }
        }
        while(pos < tlv.length && (len2 = lwm2m_tlv_read(&tlv2, &tlv.value[pos],
                                                       tlv.length - pos))) {
          LOG_DBG("   TLV type:%d id:%d len:%d (len:%d/%d)\n",
                  tlv2.type, tlv2.id, (int)tlv2.length,
                  (int)len2, (int)insize);
          if(tlv2.type == LWM2M_TLV_TYPE_RESOURCE) {
            last_tlv_id = tlv2.id;
            status = process_tlv_write(ctx, object, tlv2.id,
                                       (uint8_t *)&tlv.value[pos], len2);
            if(status != LWM2M_STATUS_OK) {
              return status;
            }
          }
          pos += len2;
        }
      } else if(tlv.type == LWM2M_TLV_TYPE_RESOURCE) {
        status = process_tlv_write(ctx, object, tlv.id, &inbuf[tlvpos], len);
        if(status != LWM2M_STATUS_OK) {
          return status;
        }
        coap_set_status_code(ctx->response, CHANGED_2_04);
      }
      tlvpos += len;
    }
  } else if(format == LWM2M_TEXT_PLAIN ||
            format == TEXT_PLAIN ||
            format == LWM2M_OLD_OPAQUE) {
    return call_instance(instance, ctx);

  } else {
    /* Unsupported format */
    return LWM2M_STATUS_UNSUPPORTED_CONTENT_FORMAT;
  }

  /* Here we have a success! */
  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
lwm2m_object_instance_t *
lwm2m_engine_get_instance_buffer(void)
{
  return &instance_buffer;
}
/*---------------------------------------------------------------------------*/
int
lwm2m_engine_has_instance(uint16_t object_id, uint16_t instance_id)
{
  return get_instance(object_id, instance_id, NULL) != NULL;
}
/*---------------------------------------------------------------------------*/
int
lwm2m_engine_add_object(lwm2m_object_instance_t *object)
{
  lwm2m_object_instance_t *instance;
  uint16_t min_id = 0xffff;
  uint16_t max_id = 0;
  int found = 0;

  if(object == NULL || object->callback == NULL) {
    /* Insufficient object configuration */
    LOG_DBG("failed to register NULL object\n");
    return 0;
  }
  if(get_object(object->object_id) != NULL) {
    /* A generic object with this id has already been registered */
    LOG_DBG("object with id %u already registered\n", object->object_id);
    return 0;
  }

  for(instance = list_head(object_list);
      instance != NULL;
      instance = instance->next) {
    if(object->object_id == instance->object_id) {
      if(object->instance_id == instance->instance_id) {
        LOG_DBG("object with id %u/%u already registered\n",
               instance->object_id, instance->instance_id);
        return 0;
      }

      found++;
      if(instance->instance_id > max_id) {
        max_id = instance->instance_id;
      }
      if(instance->instance_id < min_id) {
        min_id = instance->instance_id;
      }
    }
  }

  if(object->instance_id == LWM2M_OBJECT_INSTANCE_NONE) {
    /* No instance id has been assigned yet */
    if(found == 0) {
      /* First object with this id */
      object->instance_id = 0;
    } else if(min_id > 0) {
      object->instance_id = min_id - 1;
    } else {
      object->instance_id = max_id + 1;
    }
  }
  list_add(object_list, object);
#if USE_RD_CLIENT
  lwm2m_rd_client_set_update_rd();
#endif
  return 1;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_engine_remove_object(lwm2m_object_instance_t *object)
{
  list_remove(object_list, object);
#if USE_RD_CLIENT
  lwm2m_rd_client_set_update_rd();
#endif
}
/*---------------------------------------------------------------------------*/
int
lwm2m_engine_add_generic_object(lwm2m_object_t *object)
{
  if(object == NULL || object->impl == NULL
     || object->impl->get_first == NULL
     || object->impl->get_next == NULL
     || object->impl->get_by_id == NULL) {
    LOG_WARN("failed to register NULL object\n");
    return 0;
  }
  if(get_object(object->impl->object_id) != NULL) {
    /* A generic object with this id has already been registered */
    LOG_WARN("object with id %u already registered\n",
             object->impl->object_id);
    return 0;
  }
  if(has_non_generic_object(object->impl->object_id)) {
    /* An object with this id has already been registered */
    LOG_WARN("object with id %u already registered\n",
             object->impl->object_id);
    return 0;
  }
  list_add(generic_object_list, object);

#if USE_RD_CLIENT
  lwm2m_rd_client_set_update_rd();
#endif

  return 1;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_engine_remove_generic_object(lwm2m_object_t *object)
{
  list_remove(generic_object_list, object);
#if USE_RD_CLIENT
  lwm2m_rd_client_set_update_rd();
#endif
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t *
next_object_instance(const lwm2m_context_t *context, lwm2m_object_t *object,
                     lwm2m_object_instance_t *last)
{
  if(context != NULL && context->level >= 2) {
    /* Only single instance */
    return NULL;
  }

  /* There has to be a last to get a next */
  if(last == NULL) {
    return NULL;
  }

  if(object == NULL) {
    for(last = last->next; last != NULL; last = last->next) {
      /* if no context is given - this will just give the next object */
      if(context == NULL || last->object_id == context->object_id) {
        return last;
      }
    }
    return NULL;
  }
  return object->impl->get_next(last, NULL);
}
/*---------------------------------------------------------------------------*/
static coap_handler_status_t
lwm2m_handler_callback(coap_message_t *request, coap_message_t *response,
                       uint8_t *buffer, uint16_t buffer_size, int32_t *offset)
{
  const char *url;
  int url_len;
  unsigned int format;
  unsigned int accept;
  int depth;
  lwm2m_context_t context;
  lwm2m_object_t *object;
  lwm2m_object_instance_t *instance;
  uint32_t bnum;
  uint8_t bmore;
  uint16_t bsize;
  uint32_t boffset;
  lwm2m_status_t success;
  lwm2m_buffer_t inbuf;
  lwm2m_buffer_t outbuf;

  /* Initialize the context */
  memset(&context, 0, sizeof(context));
  memset(&outbuf, 0, sizeof(outbuf));
  memset(&inbuf, 0, sizeof(inbuf));

  context.outbuf = &outbuf;
  context.inbuf = &inbuf;

  /* Set CoAP request/response for now */
  context.request = request;
  context.response = response;

  /* Set out buffer */
  context.outbuf->buffer = buffer;
  context.outbuf->size = buffer_size;

  /* Set input buffer */
  if(offset != NULL) {
    context.offset = *offset;
  }
  context.inbuf->size = coap_get_payload(request, (const uint8_t **)&context.inbuf->buffer);
  context.inbuf->pos = 0;

#if LWM2M_QUEUE_MODE_ENABLED 
lwm2m_queue_mode_request_received();
#endif /* LWM2M_QUEUE_MODE_ENABLED */

  /* Maybe this should be part of CoAP itself - this seems not to be working
     with the leshan server */
#define LWM2M_CONF_ENTITY_TOO_LARGE_BLOCK1 0
#if LWM2M_CONF_ENTITY_TOO_LARGE_BLOCK1
  if(coap_is_option(request, COAP_OPTION_BLOCK1)) {
    uint16_t bsize;
    coap_get_header_block1(request, NULL, NULL, &bsize, NULL);

    LOG_DBG("Block1 size:%d\n", bsize);
    if(bsize > COAP_MAX_BLOCK_SIZE) {
      LOG_WARN("Entity too large: %u...\n", bsize);
      coap_set_status_code(response, REQUEST_ENTITY_TOO_LARGE_4_13);
      coap_set_header_size1(response, COAP_MAX_BLOCK_SIZE);
      return COAP_HANDLER_STATUS_PROCESSED;
    }
  }
#endif

  /* Set default reader/writer */
  context.reader = &lwm2m_plain_text_reader;
  context.writer = &lwm2m_tlv_writer;


  url_len = coap_get_header_uri_path(request, &url);

  if(url_len == 2 && strncmp("bs", url, 2) == 0) {
    LOG_INFO("BOOTSTRAPPED!!!\n");
    coap_set_status_code(response, CHANGED_2_04);
    return COAP_HANDLER_STATUS_PROCESSED;
  }

  depth = lwm2m_engine_parse_context(url, url_len, request, response,
                                     buffer, buffer_size, &context);
  if(depth < 0) {
    /* Not a LWM2M context */
    return COAP_HANDLER_STATUS_CONTINUE;
  }

  LOG_DBG("%s URL:'", get_method_as_string(coap_get_method_type(request)));
  LOG_DBG_COAP_STRING(url, url_len);
  LOG_DBG_("' CTX:%u/%u/%u dp:%u bs:%d\n", context.object_id, context.object_instance_id,
	 context.resource_id, depth, buffer_size);
  /* Get format and accept */
  if(!coap_get_header_content_format(request, &format)) {
    LOG_DBG("No format given. Assume text plain...\n");
    format = TEXT_PLAIN;
  } else if(format == LWM2M_TEXT_PLAIN) {
    /* CoAP content format text plain - assume LWM2M text plain */
    format = TEXT_PLAIN;
  }
  if(!coap_get_header_accept(request, &accept)) {
    if(format == TEXT_PLAIN && depth < 3) {
      LOG_DBG("No Accept header, assume JSON\n");
      accept = LWM2M_JSON;
    } else {
      LOG_DBG("No Accept header, using same as content-format: %d\n", format);
      accept = format;
    }
  }

  /*
   * 1 => Object only
   * 2 => Object and Instance
   * 3 => Object and Instance and Resource
   */
  if(depth < 1) {
    /* No possible object id found in URL - ignore request unless delete all */
    if(coap_get_method_type(request) == METHOD_DELETE) {
      LOG_DBG("This is a delete all - for bootstrap...\n");
      context.operation = LWM2M_OP_DELETE;
      coap_set_status_code(response, DELETED_2_02);

      /* Delete all dynamic objects that can be deleted */
      for(object = list_head(generic_object_list);
          object != NULL;
          object = object->next) {
        if(object->impl != NULL && object->impl->delete_instance != NULL) {
          object->impl->delete_instance(LWM2M_OBJECT_INSTANCE_NONE, NULL);
        }
      }
#if USE_RD_CLIENT
      lwm2m_rd_client_set_update_rd();
#endif
      return COAP_HANDLER_STATUS_PROCESSED;
    }
    return COAP_HANDLER_STATUS_CONTINUE;
  }

  instance = get_instance_by_context(&context, &object);

  /*
   * Check if we found either instance or object. Instance means we found an
   * existing instance and generic objects means we might create an instance.
   */
  if(instance == NULL && object == NULL) {
    /* No matching object/instance found - ignore request */
    return COAP_HANDLER_STATUS_CONTINUE;
  }

  LOG_INFO("Context: %u/%u/%u  found: %d\n",
           context.object_id, context.object_instance_id,
           context.resource_id, depth);

  /*
   * Select reader and writer based on provided Content type and
   * Accept headers.
   */
  lwm2m_engine_select_reader(&context, format);
  lwm2m_engine_select_writer(&context, accept);

  switch(coap_get_method_type(request)) {
  case METHOD_PUT:
    /* can also be write atts */
    context.operation = LWM2M_OP_WRITE;
    coap_set_status_code(response, CHANGED_2_04);
    break;
  case METHOD_POST:
    if(context.level < 2) {
      /* write to a instance */
      context.operation = LWM2M_OP_WRITE;
      coap_set_status_code(response, CHANGED_2_04);
    } else if(context.level == 3) {
      context.operation = LWM2M_OP_EXECUTE;
      coap_set_status_code(response, CHANGED_2_04);
    }
    break;
  case METHOD_GET:
    if(accept == APPLICATION_LINK_FORMAT) {
      context.operation = LWM2M_OP_DISCOVER;
    } else {
      context.operation = LWM2M_OP_READ;
    }
    coap_set_status_code(response, CONTENT_2_05);
    break;
  case METHOD_DELETE:
    context.operation = LWM2M_OP_DELETE;
    coap_set_status_code(response, DELETED_2_02);
    break;
  default:
    break;
  }

  if(LOG_DBG_ENABLED) {
    /* for debugging */
    LOG_DBG("[");
    LOG_DBG_COAP_STRING(url, url_len);
    LOG_DBG_("] %s Format:%d ID:%d bsize:%u offset:%"PRId32"\n",
             get_method_as_string(coap_get_method_type(request)),
             format, context.object_id, buffer_size,
             offset != NULL ? *offset : 0);
    if(format == TEXT_PLAIN) {
      /* a string */
      const uint8_t *data;
      int plen = coap_get_payload(request, &data);
      if(plen > 0) {
        LOG_DBG("Data: '");
        LOG_DBG_COAP_STRING((const char *)data, plen);
        LOG_DBG_("'\n");
      }
    }
  }

  /* PUT/POST - e.g. write will not send in offset here - Maybe in the future? */
  if((offset != NULL && *offset == 0) &&
     coap_is_option(request, COAP_OPTION_BLOCK1)) {
    coap_get_header_block1(request, &bnum, &bmore, &bsize, &boffset);
    context.offset = boffset;
  }

  /* This is a discovery operation */
  switch(context.operation) {
  case LWM2M_OP_DISCOVER:
    /* Assume only one disco at a time... */
    success = perform_multi_resource_read_op(object, instance, &context);
    break;
  case LWM2M_OP_READ:
    success = perform_multi_resource_read_op(object, instance, &context);
    break;
  case LWM2M_OP_WRITE:
    success = perform_multi_resource_write_op(object, instance, &context, format);
    break;
  case LWM2M_OP_EXECUTE:
    success = call_instance(instance, &context);
    break;
  case LWM2M_OP_DELETE:
    if(object != NULL && object->impl != NULL &&
       object->impl->delete_instance != NULL) {
      object->impl->delete_instance(context.object_instance_id, &success);
#if USE_RD_CLIENT
      lwm2m_rd_client_set_update_rd();
#endif
    } else {
      success = LWM2M_STATUS_OPERATION_NOT_ALLOWED;
    }
    break;
  default:
    success = LWM2M_STATUS_OPERATION_NOT_ALLOWED;
    break;
  }

  if(success == LWM2M_STATUS_OK) {
    /* Handle blockwise 1 */
    if(coap_is_option(request, COAP_OPTION_BLOCK1)) {
      LOG_DBG("Setting BLOCK 1 num:%"PRIu32" o2:%"PRIu32" o:%"PRId32"\n", bnum, boffset,
              (offset != NULL ? *offset : 0));
      coap_set_header_block1(response, bnum, 0, bsize);
    }

    if(context.outbuf->len > 0) {
      LOG_DBG("[");
      LOG_DBG_COAP_STRING(url, url_len);
      LOG_DBG_("] replying with %u bytes\n", context.outbuf->len);
      coap_set_payload(response, context.outbuf->buffer, context.outbuf->len);
      coap_set_header_content_format(response, context.content_type);

      if(offset != NULL) {
        LOG_DBG("Setting new offset: oo %"PRIu32
                ", no: %"PRIu32"\n", *offset, context.offset);
        if(context.writer_flags & WRITER_HAS_MORE) {
          *offset = context.offset;
        } else {
          /* this signals to CoAP that there is no more CoAP messages to expect */
          *offset = -1;
        }
      }
    } else {
      LOG_DBG("[");
      LOG_DBG_COAP_STRING(url, url_len);
      LOG_DBG_("] no data in reply\n");
    }
  } else {
    switch(success) {
    case LWM2M_STATUS_FORBIDDEN:
      coap_set_status_code(response, FORBIDDEN_4_03);
      break;
    case LWM2M_STATUS_NOT_FOUND:
      coap_set_status_code(response, NOT_FOUND_4_04);
      break;
    case LWM2M_STATUS_OPERATION_NOT_ALLOWED:
      coap_set_status_code(response, METHOD_NOT_ALLOWED_4_05);
      break;
    case LWM2M_STATUS_NOT_ACCEPTABLE:
      coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
      break;
    case LWM2M_STATUS_UNSUPPORTED_CONTENT_FORMAT:
      coap_set_status_code(response, UNSUPPORTED_MEDIA_TYPE_4_15);
      break;
    default:
      /* Failed to handle the request */
      coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
      break;
    }
    LOG_WARN("[");
    LOG_WARN_COAP_STRING(url, url_len);
    LOG_WARN("] resource failed: %s\n", get_status_as_string(success));
  }
  return COAP_HANDLER_STATUS_PROCESSED;
}
/*---------------------------------------------------------------------------*/
static void
lwm2m_send_notification(char* path)
{
#if LWM2M_QUEUE_MODE_ENABLED && LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
    if(lwm2m_queue_mode_get_dynamic_adaptation_flag()) {
      lwm2m_queue_mode_set_handler_from_notification();
    } 
#endif
  coap_notify_observers_sub(NULL, path);
}
/*---------------------------------------------------------------------------*/
void 
lwm2m_notify_object_observers(lwm2m_object_instance_t *obj,
                                   uint16_t resource)
{
  char path[20]; /* 60000/60000/60000 */
  if(obj != NULL) {
    snprintf(path, 20, "%d/%d/%d", obj->object_id, obj->instance_id, resource);
  }

#if LWM2M_QUEUE_MODE_ENABLED
  
  if(coap_has_observers(path)) {
    /* Client is sleeping -> add the notification to the list */
    if(!lwm2m_rd_client_is_client_awake()) {
      lwm2m_notification_queue_add_notification_path(obj->object_id, obj->instance_id, resource);

      /* if it is the first notification -> wake up and send update */
      if(!lwm2m_queue_mode_is_waked_up_by_notification()) {
        lwm2m_queue_mode_set_waked_up_by_notification();
        lwm2m_rd_client_fsm_execute_queue_mode_update();
      }
    /* Client is awake -> send the notification */  
    } else {
      lwm2m_send_notification(path);
    }
  }
#else 
  lwm2m_send_notification(path);
#endif
}
/*---------------------------------------------------------------------------*/
/** @} */
