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
#include "contiki.h"
#include "contiki-lib.h"
#include "lib/memb.h"

#include "mqtt.h"
#include "mqtt-prop.h"

#include <stdlib.h>
/*---------------------------------------------------------------------------*/
#if MQTT_PROP_USE_MEMB
MEMB(prop_lists_mem, struct mqtt_prop_list, MQTT_PROP_MAX_OUT_PROP_LISTS);
MEMB(props_mem, struct mqtt_prop_out_property, MQTT_PROP_MAX_OUT_PROPS);
#endif
/*----------------------------------------------------------------------------*/
void
mqtt_props_init()
{
#if MQTT_PROP_USE_MEMB
  memb_init(&props_mem);
  memb_init(&prop_lists_mem);
#endif
}
/*----------------------------------------------------------------------------*/
static void
encode_prop_fixed_len_int(struct mqtt_prop_out_property **prop_out,
                          int val, uint8_t len)
{
  int8_t i;

  DBG("MQTT - Creating %d-byte int property %i\n", len, val);

  if(len > MQTT_PROP_MAX_PROP_LENGTH) {
    DBG("MQTT - Error, property too long (max %i bytes)", MQTT_PROP_MAX_PROP_LENGTH);
    return;
  }

  for(i = len - 1; i >= 0; i--) {
    (*prop_out)->val[i] = val & 0x00FF;
    val = val >> 8;
  }

  (*prop_out)->property_len = len;
}
/*---------------------------------------------------------------------------*/
static void
encode_prop_utf8(struct mqtt_prop_out_property **prop_out,
                 const char *str)
{
  int str_len;

  DBG("MQTT - Encoding UTF-8 Property %s\n", str);
  str_len = strlen(str);

  /* 2 bytes are needed for each string to encode its length */
  if((str_len + 2) > MQTT_PROP_MAX_PROP_LENGTH) {
    DBG("MQTT - Error, property too long (max %i bytes)", MQTT_PROP_MAX_PROP_LENGTH);
    return;
  }

  (*prop_out)->val[0] = str_len >> 8;
  (*prop_out)->val[1] = str_len & 0x00FF;
  memcpy((*prop_out)->val + 2, str, str_len);

  (*prop_out)->property_len = str_len + 2;
}
/*---------------------------------------------------------------------------*/
static void
encode_prop_binary(struct mqtt_prop_out_property **prop_out,
                   const char *data, int data_len)
{
  DBG("MQTT - Encoding Binary Data (%d bytes)\n", data_len);

  if((data_len + 2) > MQTT_PROP_MAX_PROP_LENGTH) {
    DBG("MQTT - Error, property too long (max %i bytes)", MQTT_PROP_MAX_PROP_LENGTH);
    return;
  }

  (*prop_out)->val[0] = data_len >> 8;
  (*prop_out)->val[1] = data_len & 0x00FF;
  memcpy((*prop_out)->val + 2, data, data_len);

  (*prop_out)->property_len = data_len + 2;
}
/*---------------------------------------------------------------------------*/
static void
encode_prop_var_byte_int(struct mqtt_prop_out_property **prop_out,
                         int val)
{
  uint8_t id_len;

  DBG("MQTT - Encoding Variable Byte Integer %d\n", val);

  mqtt_encode_var_byte_int(
    (*prop_out)->val,
    &id_len,
    val);

  (*prop_out)->property_len = id_len;
}
/*---------------------------------------------------------------------------*/
uint32_t
mqtt_prop_encode(struct mqtt_prop_out_property **prop_out, mqtt_vhdr_prop_t prop_id,
                 va_list args)
{
  DBG("MQTT - Creating property with ID %i\n", prop_id);

  if(!(*prop_out)) {
    DBG("MQTT - Error, property target NULL!\n");
    return 0;
  }

  (*prop_out)->property_len = 0;
  (*prop_out)->id = prop_id;

  /* Decode varargs and create encoded property value for selected type */
  switch(prop_id) {
  case MQTT_VHDR_PROP_PAYLOAD_FMT_IND:
  case MQTT_VHDR_PROP_REQ_PROBLEM_INFO:
  case MQTT_VHDR_PROP_REQ_RESP_INFO: {
    int val;

    val = va_arg(args, int);
    encode_prop_fixed_len_int(prop_out, val, 1);

    break;
  }
  case MQTT_VHDR_PROP_RECEIVE_MAX:
  case MQTT_VHDR_PROP_TOPIC_ALIAS_MAX:
  case MQTT_VHDR_PROP_TOPIC_ALIAS: {
    int val;

    val = va_arg(args, int);
    encode_prop_fixed_len_int(prop_out, val, 2);

    break;
  }
  case MQTT_VHDR_PROP_MSG_EXP_INT:
  case MQTT_VHDR_PROP_SESS_EXP_INT:
  case MQTT_VHDR_PROP_WILL_DELAY_INT:
  case MQTT_VHDR_PROP_MAX_PKT_SZ: {
    int val;

    val = va_arg(args, int);
    encode_prop_fixed_len_int(prop_out, val, 4);

    break;
  }
  case MQTT_VHDR_PROP_CONTENT_TYPE:
  case MQTT_VHDR_PROP_RESP_TOPIC:
  case MQTT_VHDR_PROP_AUTH_METHOD: {
    const char *str;

    str = va_arg(args, const char *);
    encode_prop_utf8(prop_out, str);

    break;
  }
  case MQTT_VHDR_PROP_CORRELATION_DATA:
  case MQTT_VHDR_PROP_AUTH_DATA: {
    const char *data;
    int data_len;

    data = va_arg(args, const char *);
    data_len = va_arg(args, int);

    encode_prop_binary(prop_out, data, data_len);

    break;
  }
  case MQTT_VHDR_PROP_SUB_ID: {
    int val;

    val = va_arg(args, int);

    encode_prop_var_byte_int(prop_out, val);

    break;
  }
  case MQTT_VHDR_PROP_USER_PROP: {
    const char *name;
    const char *value;
    uint16_t name_len;
    uint16_t val_len;

    name = va_arg(args, const char *);
    value = va_arg(args, const char *);

    name_len = strlen(name);
    val_len = strlen(value);

    DBG("MQTT - Encoding User Property '%s: %s'\n", name, value);

    /* 2 bytes are needed for each string to encode its length */
    if((name_len + val_len + 4) > MQTT_PROP_MAX_PROP_LENGTH) {
      DBG("MQTT - Error, property '%i' too long (max %i bytes)", prop_id, MQTT_PROP_MAX_PROP_LENGTH);
      return 0;
    }

    (*prop_out)->val[0] = name_len >> 8;
    (*prop_out)->val[1] = name_len & 0x00FF;
    memcpy((*prop_out)->val + 2, name, strlen(name));
    (*prop_out)->val[name_len + 2] = val_len >> 8;
    (*prop_out)->val[name_len + 3] = val_len & 0x00FF;
    memcpy((*prop_out)->val + name_len + 4, value, strlen(value));

    (*prop_out)->property_len = strlen(name) + strlen(value) + 4;
    break;
  }
  default:
    DBG("MQTT - Error, no such property '%i'\n", prop_id);
    *prop_out = NULL;
    return 0;
  }

  DBG("MQTT - Property encoded length %i\n", (*prop_out)->property_len);

  return (*prop_out)->property_len;
}
/*---------------------------------------------------------------------------*/
#if MQTT_5
void
mqtt_prop_decode_input_props(struct mqtt_connection *conn)
{
  uint8_t prop_len_bytes;

  DBG("MQTT - Parsing input properties\n");

  /* All but PINGREQ and PINGRESP may contain a set of properties in the VHDR */
  if(((conn->in_packet.fhdr & 0xF0) == MQTT_FHDR_MSG_TYPE_PINGREQ) ||
     ((conn->in_packet.fhdr & 0xF0) == MQTT_FHDR_MSG_TYPE_PINGRESP)) {
    return;
  }

  DBG("MQTT - Getting length\n");

  prop_len_bytes =
    mqtt_decode_var_byte_int(conn->in_packet.payload_start,
                             conn->in_packet.remaining_length - (conn->in_packet.payload_start - conn->in_packet.payload),
                             NULL, NULL, &conn->in_packet.properties_len);

  if(prop_len_bytes == 0) {
    DBG("MQTT - Error decoding input properties (out of bounds)\n");
    return;
  }

  DBG("MQTT - Received %i VBI property bytes\n", prop_len_bytes);
  DBG("MQTT - Input properties length %i\n", conn->in_packet.properties_len);

  /* Total property length = number of bytes to encode length + length of
   * properties themselves
   */
  conn->in_packet.properties_enc_len = prop_len_bytes;
  /* Actual properties start after property length VBI */
  conn->in_packet.props_start = conn->in_packet.payload_start + prop_len_bytes;
  conn->in_packet.payload_start += prop_len_bytes;
  conn->in_packet.curr_props_pos = conn->in_packet.props_start;

  DBG("MQTT - First byte of first prop %i\n", *conn->in_packet.curr_props_pos);

  conn->in_packet.has_props = 1;
}
#endif
/*---------------------------------------------------------------------------*/
static uint32_t
decode_prop_utf8(struct mqtt_connection *conn,
                 uint8_t *buf_in,
                 uint8_t *data)
{
  uint32_t len;

  len = (buf_in[0] << 8) + buf_in[1];

  DBG("MQTT - Decoding %d-char UTF8 string property\n", len);

  /* Include NULL terminator in destination */
  if((len + MQTT_STRING_LEN_SIZE + 1) > MQTT_PROP_MAX_PROP_LENGTH) {
    DBG("MQTT - Error, property too long (max %i bytes)", MQTT_PROP_MAX_PROP_LENGTH);
    return 0;
  }

  memcpy(data, buf_in, len + MQTT_STRING_LEN_SIZE);
  data[len + MQTT_STRING_LEN_SIZE] = '\0';

  /* Length of string + 2 bytes for length */
  return len + MQTT_STRING_LEN_SIZE;
}
/*---------------------------------------------------------------------------*/
static uint32_t
decode_prop_fixed_len_int(struct mqtt_connection *conn,
                          uint8_t *buf_in, int len,
                          uint8_t *data)
{
  int8_t i;
  uint32_t *data_out;

  DBG("MQTT - Decoding %d-byte int property\n", len);

  if(len > MQTT_PROP_MAX_PROP_LENGTH) {
    DBG("MQTT - Error, property too long (max %i bytes)", MQTT_PROP_MAX_PROP_LENGTH);
    return 0;
  }

  /* All integer input properties will be returned as uint32_t */
  memset(data, 0, 4);

  data_out = (uint32_t *)data;

  for(i = 0; i < 4; i++) {
    *data_out = *data_out << 8;

    if(i < len) {
      *data_out += buf_in[i];
    }
  }

  return len;
}
/*---------------------------------------------------------------------------*/
static uint32_t
decode_prop_vbi(struct mqtt_connection *conn,
                uint8_t *buf_in,
                uint8_t *data)
{
  uint8_t prop_len_bytes;

  DBG("MQTT - Decoding Variable Byte Integer property\n");

  /* All integer input properties will be returned as uint32_t */
  memset(data, 0, 4);

  prop_len_bytes =
    mqtt_decode_var_byte_int(buf_in, 4, NULL, NULL, (uint16_t *)data);

  if(prop_len_bytes == 0) {
    DBG("MQTT - Error decoding Variable Byte Integer\n");
    return 0;
  }

  if(prop_len_bytes > MQTT_PROP_MAX_PROP_LENGTH) {
    DBG("MQTT - Error, property too long (max %i bytes)", MQTT_PROP_MAX_PROP_LENGTH);
    return 0;
  }

  return prop_len_bytes;
}
/*---------------------------------------------------------------------------*/
static uint32_t
decode_prop_binary_data(struct mqtt_connection *conn,
                        uint8_t *buf_in,
                        uint8_t *data)
{
  uint8_t data_len;

  DBG("MQTT - Decoding Binary Data property\n");

  data_len = (buf_in[0] << 8) + buf_in[1];

  if(data_len == 0) {
    DBG("MQTT - Error decoding Binary Data property length\n");
    return 0;
  }

  if((data_len + 2) > MQTT_PROP_MAX_PROP_LENGTH) {
    DBG("MQTT - Error, property too long (max %i bytes)", MQTT_PROP_MAX_PROP_LENGTH);
    return 0;
  }

  memcpy(data, buf_in, data_len + 2);

  return data_len + 2;
}
/*---------------------------------------------------------------------------*/
static uint32_t
decode_prop_utf8_pair(struct mqtt_connection *conn,
                      uint8_t *buf_in,
                      uint8_t *data)
{
  uint32_t len1;
  uint32_t len2;
  uint32_t total_len;

  len1 = (buf_in[0] << 8) + buf_in[1];
  len2 = (buf_in[len1 + MQTT_STRING_LEN_SIZE] << 8) + buf_in[len1 + MQTT_STRING_LEN_SIZE + 1];
  total_len = len1 + len2;

  DBG("MQTT - Decoding %d-char UTF8 string pair property (%i + %i)\n", total_len, len1, len2);

  if((total_len + 2 * MQTT_STRING_LEN_SIZE) > MQTT_PROP_MAX_PROP_LENGTH) {
    DBG("MQTT - Error, property too long (max %i bytes)", MQTT_PROP_MAX_PROP_LENGTH);
    return 0;
  }

  memcpy(data, buf_in, total_len + 2 * MQTT_STRING_LEN_SIZE);

  /* Length of string + 2 bytes for length */
  return total_len + 2 * MQTT_STRING_LEN_SIZE;
}
/*---------------------------------------------------------------------------*/
uint32_t
parse_prop(struct mqtt_connection *conn,
           mqtt_vhdr_prop_t prop_id, uint8_t *buf_in, uint8_t *data)
{
  switch(prop_id) {
  case MQTT_VHDR_PROP_PAYLOAD_FMT_IND:
  case MQTT_VHDR_PROP_REQ_PROBLEM_INFO:
  case MQTT_VHDR_PROP_REQ_RESP_INFO:
  case MQTT_VHDR_PROP_MAX_QOS:
  case MQTT_VHDR_PROP_RETAIN_AVAIL:
  case MQTT_VHDR_PROP_WILD_SUB_AVAIL:
  case MQTT_VHDR_PROP_SUB_ID_AVAIL:
  case MQTT_VHDR_PROP_SHARED_SUB_AVAIL: {
    return decode_prop_fixed_len_int(conn, buf_in, 1, data);
  }
  case MQTT_VHDR_PROP_RECEIVE_MAX:
  case MQTT_VHDR_PROP_TOPIC_ALIAS_MAX:
  case MQTT_VHDR_PROP_SERVER_KEEP_ALIVE: {
    return decode_prop_fixed_len_int(conn, buf_in, 2, data);
  }
  case MQTT_VHDR_PROP_MSG_EXP_INT:
  case MQTT_VHDR_PROP_SESS_EXP_INT:
  case MQTT_VHDR_PROP_WILL_DELAY_INT:
  case MQTT_VHDR_PROP_MAX_PKT_SZ: {
    return decode_prop_fixed_len_int(conn, buf_in, 4, data);
  }
  case MQTT_VHDR_PROP_CONTENT_TYPE:
  case MQTT_VHDR_PROP_RESP_TOPIC:
  case MQTT_VHDR_PROP_AUTH_METHOD:
  case MQTT_VHDR_PROP_ASSIGNED_CLIENT_ID:
  case MQTT_VHDR_PROP_RESP_INFO:
  case MQTT_VHDR_PROP_SERVER_REFERENCE:
  case MQTT_VHDR_PROP_REASON_STRING: {
    return decode_prop_utf8(conn, buf_in, data);
  }
  case MQTT_VHDR_PROP_CORRELATION_DATA:
  case MQTT_VHDR_PROP_AUTH_DATA: {
    return decode_prop_binary_data(conn, buf_in, data);
  }
  case MQTT_VHDR_PROP_SUB_ID: {
    return decode_prop_vbi(conn, buf_in, data);
  }
  case MQTT_VHDR_PROP_USER_PROP: {
    return decode_prop_utf8_pair(conn, buf_in, data);
  }
  default:
    DBG("MQTT - Error, no such property '%i'", prop_id);
    return 0;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
#if MQTT_5
uint32_t
mqtt_get_next_in_prop(struct mqtt_connection *conn,
                      mqtt_vhdr_prop_t *prop_id, uint8_t *data)
{
  uint32_t prop_len;
  uint8_t prop_id_len_bytes;

  if(!conn->in_packet.has_props) {
    DBG("MQTT - Message has no input properties");
    return 0;
  }

  DBG("MQTT - Curr prop pos %i; len %i; byte %i\n", (conn->in_packet.curr_props_pos - conn->in_packet.props_start),
      conn->in_packet.properties_len,
      *conn->in_packet.curr_props_pos);

  if((conn->in_packet.curr_props_pos - conn->in_packet.props_start)
     >= conn->in_packet.properties_len) {
    DBG("MQTT - Message has no more input properties\n");
    return 0;
  }

  prop_id_len_bytes =
    mqtt_decode_var_byte_int(conn->in_packet.curr_props_pos,
                             conn->in_packet.properties_len - (conn->in_packet.curr_props_pos - conn->in_packet.props_start),
                             NULL, NULL, (uint16_t *)prop_id);

  DBG("MQTT - Decoded property ID %i (encoded using %i bytes)\n", *prop_id, prop_id_len_bytes);

  prop_len = parse_prop(conn, *prop_id, conn->in_packet.curr_props_pos + 1, data);

  DBG("MQTT - Decoded property len %i bytes\n", prop_len);

  conn->in_packet.curr_props_pos += prop_id_len_bytes + prop_len;

  return prop_len;
}
/*---------------------------------------------------------------------------*/
void
mqtt_prop_parse_connack_props(struct mqtt_connection *conn)
{
  uint32_t prop_len;
  mqtt_vhdr_prop_t prop_id;
  uint8_t data[MQTT_PROP_MAX_PROP_LENGTH];
  uint32_t val_int;

  DBG("MQTT - Parsing CONNACK properties for server capabilities\n");

  prop_len = mqtt_get_next_in_prop(conn, &prop_id, data);
  while(prop_len) {
    switch(prop_id) {
    case MQTT_VHDR_PROP_RETAIN_AVAIL: {
      val_int = (uint32_t)*data;
      if(val_int == 0) {
        conn->srv_feature_en &= ~MQTT_CAP_RETAIN_AVAIL;
      }
      break;
    }
    case MQTT_VHDR_PROP_WILD_SUB_AVAIL: {
      val_int = (uint32_t)*data;
      if(val_int == 0) {
        conn->srv_feature_en &= ~MQTT_CAP_WILD_SUB_AVAIL;
      }
      break;
    }
    case MQTT_VHDR_PROP_SUB_ID_AVAIL: {
      val_int = (uint32_t)*data;
      if(val_int == 0) {
        conn->srv_feature_en &= ~MQTT_CAP_SUB_ID_AVAIL;
      }
      break;
    }
    case MQTT_VHDR_PROP_SHARED_SUB_AVAIL:  {
      val_int = (uint32_t)*data;
      if(val_int == 0) {
        conn->srv_feature_en &= ~MQTT_CAP_SHARED_SUB_AVAIL;
      }
      break;
    }
    default:
      DBG("MQTT - Error, unexpected CONNACK property '%i'", prop_id);
      return;
    }

    prop_id = 0;
    prop_len = mqtt_get_next_in_prop(conn, &prop_id, data);
  }
}
/*---------------------------------------------------------------------------*/
void
mqtt_prop_parse_auth_props(struct mqtt_connection *conn, struct mqtt_prop_auth_event *event)
{
  uint32_t prop_len;
  mqtt_vhdr_prop_t prop_id;
  uint8_t data[MQTT_PROP_MAX_PROP_LENGTH];

  DBG("MQTT - Parsing CONNACK properties for server capabilities\n");

  event->auth_data.len = 0;
  event->auth_method.length = 0;

  prop_len = mqtt_get_next_in_prop(conn, &prop_id, data);
  while(prop_len) {
    switch(prop_id) {
    case MQTT_VHDR_PROP_AUTH_DATA: {
      event->auth_data.len = prop_len - 2; /* 2 bytes are used to encode len */
      memcpy(event->auth_data.data, data, prop_len - 2);
      break;
    }
    case MQTT_VHDR_PROP_AUTH_METHOD: {
      event->auth_method.length = prop_len - 2; /* 2 bytes are used to encode len */
      memcpy(event->auth_method.string, data, prop_len - 2);
      break;
    }
    default:
      DBG("MQTT - Unhandled AUTH property '%i'", prop_id);
      return;
    }

    prop_id = 0;
    prop_len = mqtt_get_next_in_prop(conn, &prop_id, data);
  }
}
/*---------------------------------------------------------------------------*/
void
mqtt_prop_print_input_props(struct mqtt_connection *conn)
{
  uint32_t prop_len;
  mqtt_vhdr_prop_t prop_id;
  uint8_t data[MQTT_PROP_MAX_PROP_LENGTH];
  uint32_t i;

  DBG("MQTT - Printing all input properties\n");

  prop_len = mqtt_get_next_in_prop(conn, &prop_id, data);
  while(prop_len) {
    DBG("MQTT - Property ID %i, length %i\n", prop_id, prop_len);

    switch(prop_id) {
    case MQTT_VHDR_PROP_PAYLOAD_FMT_IND:
    case MQTT_VHDR_PROP_REQ_PROBLEM_INFO:
    case MQTT_VHDR_PROP_REQ_RESP_INFO:
    case MQTT_VHDR_PROP_MSG_EXP_INT:
    case MQTT_VHDR_PROP_SESS_EXP_INT:
    case MQTT_VHDR_PROP_WILL_DELAY_INT:
    case MQTT_VHDR_PROP_MAX_PKT_SZ:
    case MQTT_VHDR_PROP_RECEIVE_MAX:
    case MQTT_VHDR_PROP_TOPIC_ALIAS_MAX:
    case MQTT_VHDR_PROP_SUB_ID: {
      DBG("MQTT - Decoded property value '%i'\n", (uint32_t)*data);
      break;
    }
    case MQTT_VHDR_PROP_CONTENT_TYPE:
    case MQTT_VHDR_PROP_RESP_TOPIC:
    case MQTT_VHDR_PROP_AUTH_METHOD: {
      DBG("MQTT - Decoded property value ");
      DBG("(%i %i) %s", data[0], data[1], data + MQTT_STRING_LEN_SIZE);
      DBG("\n");
      break;
    }
    case MQTT_VHDR_PROP_CORRELATION_DATA:
    case MQTT_VHDR_PROP_AUTH_DATA: {
      DBG("MQTT - Decoded property value (%i %i) ", data[0], data[1]);
      for(i = 2; i < prop_len; i++) {
        DBG("%x ", data[i]);
      }
      DBG("\n");
      break;
    }
    case MQTT_VHDR_PROP_USER_PROP: {
#if DEBUG_MQTT
      uint32_t len1;
      uint32_t len2;

      len1 = (data[0] << 8) + data[1];
      len2 = (data[len1 + MQTT_STRING_LEN_SIZE] << 8) + data[len1 + MQTT_STRING_LEN_SIZE + 1];
#endif
      DBG("MQTT - Decoded property value [(%i %i) %.*s, (%i %i) %.*s]",
          data[0], data[1], len1, data + MQTT_STRING_LEN_SIZE,
          data[len1 + MQTT_STRING_LEN_SIZE], data[len1 + MQTT_STRING_LEN_SIZE + 1], len2, data + len1 + 2 * MQTT_STRING_LEN_SIZE);
      DBG("\n");
      break;
    }
    default:
      DBG("MQTT - Error, no such property '%i'\n", prop_id);
      return;
    }

    prop_id = 0;
    prop_len = mqtt_get_next_in_prop(conn, &prop_id, data);
  }
}
/*
 * Functions to manipulate property lists
 */
/*----------------------------------------------------------------------------*/
/* Creates a property list for the requested message type */
void
mqtt_prop_create_list(struct mqtt_prop_list **prop_list_out)
{
  DBG("MQTT - Creating Property List\n");

#if MQTT_PROP_USE_MEMB
  *prop_list_out = memb_alloc(&prop_lists_mem);
#endif

  if(!(*prop_list_out)) {
    DBG("MQTT - Error, allocated too many property lists (max %i)\n", MQTT_PROP_MAX_OUT_PROP_LISTS);
    return;
  }

  DBG("MQTT - Allocated Property list\n");

  LIST_STRUCT_INIT(*prop_list_out, props);

  DBG("MQTT - mem %p prop_list\n", *prop_list_out);

  (*prop_list_out)->properties_len = 0;
  (*prop_list_out)->properties_len_enc_bytes = 1; /* 1 byte needed for len = 0 */
}
/*----------------------------------------------------------------------------*/
/* Prints all properties in the given property list (debug)
 * If ID == MQTT_VHDR_PROP_ANY, prints all properties, otherwise it filters them
 * by property ID
 */
void
mqtt_prop_print_list(struct mqtt_prop_list *prop_list, mqtt_vhdr_prop_t prop_id)
{
  struct mqtt_prop_out_property *prop;

  if(prop_list == NULL || prop_list->props == NULL) {
    DBG("MQTT - Prop list empty\n");
  } else {
    prop = (struct mqtt_prop_out_property *)list_head(prop_list->props);

    do {
      if(prop != NULL && (prop->id == prop_id || prop_id == MQTT_VHDR_PROP_ANY)) {
        DBG("Property %p ID %i len %i\n", prop, prop->id, prop->property_len);
      }
      prop = (struct mqtt_prop_out_property *)list_item_next(prop);
    } while(prop != NULL);
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
mqtt_prop_register(struct mqtt_prop_list **prop_list,
                   struct mqtt_prop_out_property **prop_out,
#if !MQTT_PROP_USE_MEMB
                   struct mqtt_prop_out_property *prop,
#endif
                   mqtt_msg_type_t msg,
                   mqtt_vhdr_prop_t prop_id, ...)
{
#if MQTT_PROP_USE_MEMB
  struct mqtt_prop_out_property *prop;
#endif
  va_list args;
  uint32_t prop_len;

  va_start(args, prop_id);

  /* check that the property is compatible with the message? */

  if(!prop_list) {
    DBG("MQTT - Error encoding prop %i on msg %i; list NULL\n", prop_id, msg);
    va_end(args);
    return 1;
  }

  DBG("MQTT - prop list %p\n", *prop_list);
  DBG("MQTT - prop list->list %p\n", (*prop_list)->props);

#if MQTT_PROP_USE_MEMB
  prop = (struct mqtt_prop_out_property *)memb_alloc(&props_mem);
#endif

  if(!prop) {
    DBG("MQTT - Error, allocated too many properties (max %i)\n", MQTT_PROP_MAX_OUT_PROPS);
    prop_out = NULL;
    return 1;
  }

  DBG("MQTT - Allocated prop %p\n", prop);

  prop_len = mqtt_prop_encode(&prop, prop_id, args);

  if(prop) {
    DBG("MQTT - Adding prop %p to prop_list %p\n", prop, *prop_list);
    list_add((*prop_list)->props, prop);
    (*prop_list)->properties_len += 1; /* Property ID */
    (*prop_list)->properties_len += prop_len;
    mqtt_encode_var_byte_int(
      (*prop_list)->properties_len_enc,
      &((*prop_list)->properties_len_enc_bytes),
      (*prop_list)->properties_len);
    DBG("MQTT - New prop_list length %i\n", (*prop_list)->properties_len);
  } else {
    DBG("MQTT - Error encoding prop %i on msg %i\n", prop_id, msg);
#if MQTT_PROP_USE_MEMB
    memb_free(&props_mem, prop);
#endif
    va_end(args);
    prop_out = NULL;
    return 1;
  }

  if(prop_out) {
    *prop_out = prop;
  }
  va_end(args);
  return 0;
}
/*----------------------------------------------------------------------------*/
/* Remove one property from list and free its memory */
uint8_t
mqtt_remove_prop(struct mqtt_prop_list **prop_list,
                 struct mqtt_prop_out_property *prop)
{
  if(prop != NULL && prop_list != NULL && list_contains((*prop_list)->props, prop)) {
    DBG("MQTT - Removing property %p from list %p\n", prop, *prop_list);

    /* Remove from list */
    list_remove((*prop_list)->props, prop);

    /* Fix the property list length */
    (*prop_list)->properties_len -= prop->property_len;
    (*prop_list)->properties_len -= 1; /* Property ID */

    mqtt_encode_var_byte_int(
      (*prop_list)->properties_len_enc,
      &((*prop_list)->properties_len_enc_bytes),
      (*prop_list)->properties_len);

    /* Free memory */
#if MQTT_PROP_USE_MEMB
    memb_free(&props_mem, prop);
#endif
    return 0;
  } else {
    DBG("MQTT - Cannot remove property\n");
    return 1;
  }
}
/* Remove & frees all properties in the list */
void
mqtt_prop_clear_list(struct mqtt_prop_list **prop_list)
{
  struct mqtt_prop_out_property *prop;

  DBG("MQTT - Clearing Property List\n");

  if(prop_list == NULL || list_length((*prop_list)->props) == 0) {
    DBG("MQTT - Prop list empty\n");
    return;
  } else {
    prop = (struct mqtt_prop_out_property *)list_head((*prop_list)->props);

    do {
      if(prop != NULL) {
        (void)mqtt_remove_prop(prop_list, prop);
      }
      prop = (struct mqtt_prop_out_property *)list_head((*prop_list)->props);
    } while(prop != NULL);
  }

  LIST_STRUCT_INIT(*prop_list, props);

  if((*prop_list)->properties_len != 0 || (*prop_list)->properties_len_enc_bytes != 1) {
    DBG("MQTT - Something went wrong when clearing property list!\n");
  }
}
#endif
