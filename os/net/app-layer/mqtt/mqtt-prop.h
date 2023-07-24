/*
 * Copyright (c) 2020, Alexandru-Ioan Pop - https://alexandruioan.me
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
#ifndef MQTT_PROP_H_
#define MQTT_PROP_H_
/*---------------------------------------------------------------------------*/
#include "mqtt.h"

#include <stdarg.h>
/*---------------------------------------------------------------------------*/
/* If not using memb, you must provide a pointer to
 * statically-allocated memory to register_prop()
 */
#ifdef MQTT_PROP_CONF_PROP_USE_MEMB
#define MQTT_PROP_USE_MEMB MQTT_PROP_CONF_PROP_USE_MEMB
#else
#define MQTT_PROP_USE_MEMB 1
#endif
/*---------------------------------------------------------------------------*/
/* Number of output property lists */
#define MQTT_PROP_MAX_OUT_PROP_LISTS 1

/* Number of output properties that will be declared, regardless of
 * message type
 */
#define MQTT_PROP_MAX_OUT_PROPS 2

/* Max length of 1 property in bytes */
#define MQTT_PROP_MAX_PROP_LENGTH     32
/* Max number of bytes in Variable Byte Integer representation of
 * total property length
 */
#define MQTT_PROP_MAX_PROP_LEN_BYTES   2
/* Max number of topic aliases (when receiving) */
#define MQTT_PROP_MAX_NUM_TOPIC_ALIASES 1

#define MQTT_PROP_LIST_NONE NULL
/*----------------------------------------------------------------------------*/
struct mqtt_prop_list {
  /* Total length of properties */
  uint32_t properties_len;
  uint8_t properties_len_enc[MQTT_PROP_MAX_PROP_LEN_BYTES];
  uint8_t properties_len_enc_bytes;
  LIST_STRUCT(props);
};

/* This struct represents output packet Properties (MQTTv5.0). */
struct mqtt_prop_out_property {
  /* Used by the list interface, must be first in the struct. */
  struct mqtt_prop_out_property *next;

  /* Property identifier (as an MQTT Variable Byte Integer)
   * The maximum ID is currently 0x2A so 1 byte is sufficient
   * (the range of 1 VBI byte is 0x00 - 0x7F)
   */
  mqtt_vhdr_prop_t id;
  /* Property length */
  uint32_t property_len;
  /* Property value */
  uint8_t val[MQTT_PROP_MAX_PROP_LENGTH];
};

struct mqtt_prop_bin_data {
  uint16_t len;
  uint8_t data[MQTT_PROP_MAX_PROP_LENGTH];
};

struct mqtt_prop_auth_event {
  struct mqtt_string auth_method;
  struct mqtt_prop_bin_data auth_data;
};
/*----------------------------------------------------------------------------*/
void mqtt_prop_print_input_props(struct mqtt_connection *conn);

uint32_t mqtt_prop_encode(struct mqtt_prop_out_property **prop_out, mqtt_vhdr_prop_t prop_id,
                          va_list args);

void mqtt_prop_parse_connack_props(struct mqtt_connection *conn);

void mqtt_prop_parse_auth_props(struct mqtt_connection *conn, struct mqtt_prop_auth_event *event);

void mqtt_prop_decode_input_props(struct mqtt_connection *conn);

/* Switch argument order to avoid undefined behavior from having a type
   that undergoes argument promotion immediately before ", ...". */
#if MQTT_PROP_USE_MEMB
#define mqtt_prop_register(l, out, msg, id, ...) \
  mqtt_prop_register_internal(l, msg, id, out, __VA_ARGS__)
#else
#define mqtt_prop_register(l, prop, out, msg, id, ...)           \
  mqtt_prop_register_internal(l, prop, msg, id, out, __VA_ARGS__)
#endif /* MQTT_PROP_USE_MEMB */

uint8_t mqtt_prop_register_internal(struct mqtt_prop_list **prop_list,
#if !MQTT_PROP_USE_MEMB
                                    struct mqtt_prop_out_property *prop,
#endif
                                    mqtt_msg_type_t msg,
                                    mqtt_vhdr_prop_t prop_id,
                                    struct mqtt_prop_out_property **prop_out, ...);

void mqtt_prop_create_list(struct mqtt_prop_list **prop_list_out);

void mqtt_prop_print_list(struct mqtt_prop_list *prop_list, mqtt_vhdr_prop_t prop_id);

void mqtt_prop_clear_list(struct mqtt_prop_list **prop_list);

void mqtt_props_init();
/*---------------------------------------------------------------------------*/
#endif /* MQTT_PROP_H_ */
/*---------------------------------------------------------------------------*/
