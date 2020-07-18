/*
 * Copyright (c) 2015, Texas Instruments Incorporated - http://www.ti.com/
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
/**
 * \addtogroup apps
 * @{
 *
 * \defgroup mqtt-engine An implementation of MQTT v3.1
 * @{
 *
 * This application is an engine for MQTT v3.1. It supports QoS Levels 0 and 1.
 *
 * MQTT is a Client Server publish/subscribe messaging transport protocol.
 * It is light weight, open, simple, and designed so as to be easy to implement.
 * These characteristics make it ideal for use in many situations, including
 * constrained environments such as for communication in Machine to Machine
 * (M2M) and Internet of Things (IoT) contexts where a small code footprint is
 * required and/or network bandwidth is at a premium.
 *
 * The protocol runs over TCP/IP, more specifically tcp_socket.
 * Its features include:
 *
 * - Use of the publish/subscribe message pattern which provides
 * one-to-many message distribution and decoupling of applications.
 * - A messaging transport that is agnostic to the content of the payload.
 * Three qualities of service for message delivery:
 * -- "At most once" (0), where messages are delivered according to the best
 *  efforts of the operating environment. Message loss can occur.
 *  This level could be used, for example, with ambient sensor data where it
 *  does not matter if an individual reading is lost as the next one will be
 *  published soon after.
 *  --"At least once" (1), where messages are assured to arrive but duplicates
 *  can occur.
 *  -- "Exactly once" (2), where message are assured to arrive exactly once.
 *  This level could be used, for example, with billing systems where duplicate
 *  or lost messages could lead to incorrect charges being applied. This QoS
 *  level is currently not supported in this implementation.
 *
 * - A small transport overhead and protocol exchanges minimized to reduce
 *   network traffic.
 * - A mechanism, Last Will, to notify interested parties when an abnormal
 *   disconnection occurs.
 *
 *   The protocol specification and other useful information can be found
 *   here: http://mqtt.org
 *
 */
/**
 * \file
 *    Header file for the Contiki MQTT engine
 *
 * \author
 *    Texas Instruments
 */
/*---------------------------------------------------------------------------*/
#ifndef MQTT_H_
#define MQTT_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/leds.h"

#include "tcp-socket.h"
#include "udp-socket.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Protocol constants */
#define MQTT_PROTOCOL_VERSION_3_1    3
#define MQTT_PROTOCOL_VERSION_3_1_1  4
#define MQTT_PROTOCOL_VERSION_5      5

#ifdef MQTT_CONF_VERSION
#define MQTT_PROTOCOL_VERSION MQTT_CONF_VERSION
#else
#define MQTT_PROTOCOL_VERSION MQTT_PROTOCOL_VERSION_3_1
#endif

#if MQTT_PROTOCOL_VERSION == MQTT_PROTOCOL_VERSION_5
#define MQTT_5 1
#elif MQTT_PROTOCOL_VERSION == MQTT_PROTOCOL_VERSION_3_1_1
#define MQTT_311 1
#elif MQTT_PROTOCOL_VERSION == MQTT_PROTOCOL_VERSION_3_1
#define MQTT_31 1
#endif

#define MQTT_CLIENT_ID_MAX_LEN 23

/* Size of the underlying TCP buffers */
#define MQTT_TCP_INPUT_BUFF_SIZE 512
#define MQTT_TCP_OUTPUT_BUFF_SIZE 512

#define MQTT_INPUT_BUFF_SIZE 512
#define MQTT_MAX_TOPIC_LENGTH 64
#define MQTT_MAX_TOPICS_PER_SUBSCRIBE 1

#define MQTT_FHDR_SIZE 1
#define MQTT_MAX_REMAINING_LENGTH_BYTES 4
#if MQTT_31
#define MQTT_PROTOCOL_NAME "MQIsdp"
#else
#define MQTT_PROTOCOL_NAME "MQTT"
#endif

#define MQTT_TOPIC_MAX_LENGTH 128

#if MQTT_PROTOCOL_VERSION >= MQTT_PROTOCOL_VERSION_3_1_1
#ifdef MQTT_CONF_SUPPORTS_EMPTY_CLIENT_ID
#define MQTT_SRV_SUPPORTS_EMPTY_CLIENT_ID MQTT_CONF_SUPPORTS_EMPTY_CLIENT_ID
#else
#define MQTT_SRV_SUPPORTS_EMPTY_CLIENT_ID 0
#endif /* MQTT_CONF_SUPPORTS_EMPTY_CLIENT_ID */
#else
#define MQTT_SRV_SUPPORTS_EMPTY_CLIENT_ID 0
#endif

#if MQTT_31
/* Len MSB(0)
 * Len LSB(6)
 * 'M'
 * 'Q'
 * 'I'
 * 's'
 * 'd'
 * 'p'
 * Protocol Level (3)
 * Connect Flags
 * Keep Alive MSB
 * Keep Alive LSB
 */
#define MQTT_CONNECT_VHDR_SIZE 12
#else
/* Len MSB(0)
 * Len LSB(4)
 * 'M'
 * 'Q'
 * 'T'
 * 'T'
 * Protocol Level (4)
 * Connect Flags
 * Keep Alive MSB
 * Keep Alive LSB
 */
#define MQTT_CONNECT_VHDR_SIZE 10
#endif

#define MQTT_STRING_LEN_SIZE 2
#define MQTT_MID_SIZE 2
#define MQTT_QOS_SIZE 1
/*---------------------------------------------------------------------------*/
/*
 * Debug configuration, this is similar but not exactly like the Debugging
 * System discussion at https://github.com/contiki-os/contiki/wiki.
 */
#define DEBUG_MQTT 0

#if DEBUG_MQTT == 1
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...)
#endif /* DEBUG */
/*---------------------------------------------------------------------------*/
extern process_event_t mqtt_update_event;

/* Forward declaration */
struct mqtt_connection;
/* Only defined in MQTTv5 */
struct mqtt_prop_list;

typedef enum {
  MQTT_RETAIN_OFF,
  MQTT_RETAIN_ON,
} mqtt_retain_t;

typedef enum {
  MQTT_CLEAN_SESSION_OFF,
  MQTT_CLEAN_SESSION_ON,
} mqtt_clean_session_t;

/**
 * \brief MQTT engine events
 */
typedef enum {
  MQTT_EVENT_CONNECTED,
  MQTT_EVENT_DISCONNECTED,

  MQTT_EVENT_SUBACK,
  MQTT_EVENT_UNSUBACK,
  MQTT_EVENT_PUBLISH,
  MQTT_EVENT_PUBACK,

  /* Errors */
  MQTT_EVENT_ERROR = 0x80,
  MQTT_EVENT_PROTOCOL_ERROR,
  MQTT_EVENT_CONNECTION_REFUSED_ERROR,
  MQTT_EVENT_DNS_ERROR,
  MQTT_EVENT_NOT_IMPLEMENTED_ERROR,

  MQTT_EVENT_AUTH,
  /* Add more */
} mqtt_event_t;

typedef enum {
  MQTT_STATUS_OK,

  MQTT_STATUS_OUT_QUEUE_FULL,

  /* Errors */
  MQTT_STATUS_ERROR = 0x80,
  MQTT_STATUS_NOT_CONNECTED_ERROR,
  MQTT_STATUS_INVALID_ARGS_ERROR,
  MQTT_STATUS_DNS_ERROR,
} mqtt_status_t;

typedef enum {
  MQTT_QOS_LEVEL_0,
  MQTT_QOS_LEVEL_1,
  MQTT_QOS_LEVEL_2,
} mqtt_qos_level_t;

typedef enum {
  MQTT_QOS_STATE_NO_ACK,
  MQTT_QOS_STATE_GOT_ACK,

  /* Expand for QoS 2 */
} mqtt_qos_state_t;

typedef enum {
  MQTT_PUBLISH_OK,
  MQTT_PUBLISH_ERR,
} mqtt_pub_status_t;
/*---------------------------------------------------------------------------*/
/*
 * This is the state of the connection itself.
 *
 * N.B. The order is important because of runtime checks on how far the
 *      connection has proceeded.
 */
typedef enum {
  MQTT_CONN_STATE_ERROR,
  MQTT_CONN_STATE_DNS_ERROR,
  MQTT_CONN_STATE_DISCONNECTING,
  MQTT_CONN_STATE_ABORT_IMMEDIATE,
  MQTT_CONN_STATE_NOT_CONNECTED,
  MQTT_CONN_STATE_DNS_LOOKUP,
  MQTT_CONN_STATE_TCP_CONNECTING,
  MQTT_CONN_STATE_TCP_CONNECTED,
  MQTT_CONN_STATE_CONNECTING_TO_BROKER,
  MQTT_CONN_STATE_CONNECTED_TO_BROKER,
  MQTT_CONN_STATE_SENDING_MQTT_DISCONNECT,
} mqtt_conn_state_t;
/*---------------------------------------------------------------------------*/
typedef enum {
  MQTT_FHDR_MSG_TYPE_CONNECT       = 0x10,
  MQTT_FHDR_MSG_TYPE_CONNACK       = 0x20,
  MQTT_FHDR_MSG_TYPE_PUBLISH       = 0x30,
  MQTT_FHDR_MSG_TYPE_PUBACK        = 0x40,
  MQTT_FHDR_MSG_TYPE_PUBREC        = 0x50,
  MQTT_FHDR_MSG_TYPE_PUBREL        = 0x60,
  MQTT_FHDR_MSG_TYPE_PUBCOMP       = 0x70,
  MQTT_FHDR_MSG_TYPE_SUBSCRIBE     = 0x80,
  MQTT_FHDR_MSG_TYPE_SUBACK        = 0x90,
  MQTT_FHDR_MSG_TYPE_UNSUBSCRIBE   = 0xA0,
  MQTT_FHDR_MSG_TYPE_UNSUBACK      = 0xB0,
  MQTT_FHDR_MSG_TYPE_PINGREQ       = 0xC0,
  MQTT_FHDR_MSG_TYPE_PINGRESP      = 0xD0,
  MQTT_FHDR_MSG_TYPE_DISCONNECT    = 0xE0,
  MQTT_FHDR_MSG_TYPE_AUTH          = 0xF0,
} mqtt_msg_type_t;
/*---------------------------------------------------------------------------*/
/* MQTTv5.0 VHDR Properties */
typedef enum {
  MQTT_VHDR_PROP_ANY                = 0x00, /* not in standard; for library use */
  MQTT_VHDR_PROP_PAYLOAD_FMT_IND    = 0x01,
  MQTT_VHDR_PROP_MSG_EXP_INT        = 0x02,
  MQTT_VHDR_PROP_CONTENT_TYPE       = 0x03,
  MQTT_VHDR_PROP_RESP_TOPIC         = 0x08,
  MQTT_VHDR_PROP_CORRELATION_DATA   = 0x09,
  MQTT_VHDR_PROP_SUB_ID             = 0x0B,
  MQTT_VHDR_PROP_SESS_EXP_INT       = 0x11,
  MQTT_VHDR_PROP_ASSIGNED_CLIENT_ID = 0x12,
  MQTT_VHDR_PROP_SERVER_KEEP_ALIVE  = 0x13,
  MQTT_VHDR_PROP_AUTH_METHOD        = 0x15,
  MQTT_VHDR_PROP_AUTH_DATA          = 0x16,
  MQTT_VHDR_PROP_REQ_PROBLEM_INFO   = 0x17,
  MQTT_VHDR_PROP_WILL_DELAY_INT     = 0x18,
  MQTT_VHDR_PROP_REQ_RESP_INFO      = 0x19,
  MQTT_VHDR_PROP_RESP_INFO          = 0x1A,
  MQTT_VHDR_PROP_SERVER_REFERENCE   = 0x1C,
  MQTT_VHDR_PROP_REASON_STRING      = 0x1F,
  MQTT_VHDR_PROP_RECEIVE_MAX        = 0x21,
  MQTT_VHDR_PROP_TOPIC_ALIAS_MAX    = 0x22,
  MQTT_VHDR_PROP_TOPIC_ALIAS        = 0x23,
  MQTT_VHDR_PROP_MAX_QOS            = 0x24,
  MQTT_VHDR_PROP_RETAIN_AVAIL       = 0x25,
  MQTT_VHDR_PROP_USER_PROP          = 0x26,
  MQTT_VHDR_PROP_MAX_PKT_SZ         = 0x27,
  MQTT_VHDR_PROP_WILD_SUB_AVAIL     = 0x28,
  MQTT_VHDR_PROP_SUB_ID_AVAIL       = 0x29,
  MQTT_VHDR_PROP_SHARED_SUB_AVAIL   = 0x2A,
} mqtt_vhdr_prop_t;
/*---------------------------------------------------------------------------*/
/* MQTTv5.0 Binary Capabilities */
typedef enum {
  MQTT_CAP_RETAIN_AVAIL        = 0x00,
  MQTT_CAP_WILD_SUB_AVAIL      = 0x01,
  MQTT_CAP_SUB_ID_AVAIL        = 0x02,
  MQTT_CAP_SHARED_SUB_AVAIL    = 0x04,
} mqtt_srv_capability_t;

typedef enum {
  MQTT_CAP_OFF,
  MQTT_CAP_ON,
} mqtt_capability_t;

typedef enum {
  MQTT_TOPIC_ALIAS_OFF,
  MQTT_TOPIC_ALIAS_ON,
} mqtt_topic_alias_en_t;

typedef enum {
  MQTT_SUB_OPTION_QOS             = 0x03,
  MQTT_SUB_OPTION_NL              = 0x04,
  MQTT_SUB_OPTION_RAP             = 0x08,
  MQTT_SUB_OPTION_RETAIN_HANDLING = 0x30,
} mqtt_sub_option_t;

typedef enum {
  MQTT_NL_OFF,
  MQTT_NL_ON,
} mqtt_nl_en_t;

typedef enum {
  MQTT_RAP_OFF,
  MQTT_RAP_ON,
} mqtt_rap_en_t;

typedef enum {
  MQTT_RET_H_SEND_ALL  = 0x00,
  MQTT_RET_H_SEND_NEW  = 0x01,
  MQTT_RET_H_SEND_NONE = 0x02,
} mqtt_retain_handling_t;
/*---------------------------------------------------------------------------*/
struct mqtt_string {
  char *string;
  uint16_t length;
};

/*
 * Note that the pairing mid <-> QoS level only applies one-to-one if we only
 * allow the subscription of one topic at a time. Otherwise we will have an
 * ordered list of QoS levels corresponding to the order of topics.
 *
 * This could be part of a union of event data structures.
 */
struct mqtt_suback_event {
  uint16_t mid;
  mqtt_qos_level_t qos_level;
#if !MQTT_31
  uint8_t return_code;
  uint8_t success;
#endif
};

struct mqtt_connack_event {
  uint8_t session_present;
};

typedef enum {
  MQTT_AUTH_NORMAL,
  MQTT_AUTH_RE_AUTH,
} mqtt_auth_type_t;

/* This is the MQTT message that is exposed to the end user. */
struct mqtt_message {
  uint32_t mid;
  char topic[MQTT_MAX_TOPIC_LENGTH + 1]; /* +1 for string termination */

  uint8_t *payload_chunk;
  uint16_t payload_chunk_length;

  uint8_t first_chunk;
  uint16_t payload_length;
  uint16_t payload_left;
};

/* This struct represents a packet received from the MQTT server. */
struct mqtt_in_packet {
  /* Used by the list interface, must be first in the struct. */
  struct mqtt_connection *next;

  /* Total bytes read so far. Compared to the remaining length to to decide when
   * we've read the payload. */
  uint32_t byte_counter;
  uint8_t packet_received;

  uint8_t fhdr;
  uint16_t remaining_length;
  uint16_t mid;

  /* Helper variables needed to decode the remaining_length */
  uint8_t has_remaining_length;

  /* Not the same as payload in the MQTT sense, it also contains the variable
   * header.
   */
  uint16_t payload_pos;
  uint8_t payload[MQTT_INPUT_BUFF_SIZE];

  /* Start of MQTT payload (after VHDR) */
  uint8_t *payload_start;

  /* Message specific data */
  uint16_t topic_len;
  uint16_t topic_pos;
  uint8_t topic_len_received;
  uint8_t topic_received;

  /* Properties */
#if MQTT_5
  uint8_t has_reason_code;
  uint8_t reason_code;

  uint8_t has_props;  /* the properties have been decoded */
  uint8_t properties_enc_len;  /* number of bytes used to encode property length */
  uint16_t properties_len; /* length of properties excluding encoded length */
  uint8_t *props_start;  /* pointer to first byte in first property */
  uint8_t *curr_props_pos;  /* pointer to property to parse next */
#endif
};

/* This struct represents a packet sent to the MQTT server. */
struct mqtt_out_packet {
  uint8_t fhdr;
  uint32_t remaining_length;
  uint8_t remaining_length_enc[MQTT_MAX_REMAINING_LENGTH_BYTES];
  uint8_t remaining_length_enc_bytes;
  uint16_t mid;
  char *topic;
  uint16_t topic_length;
  uint8_t *payload;
  uint32_t payload_size;
  mqtt_qos_level_t qos;
  mqtt_qos_state_t qos_state;
  mqtt_retain_t retain;
#if MQTT_5
  uint8_t topic_alias;
  uint8_t sub_options;
  /* Continue Auth or Re-auth */
  uint8_t auth_reason_code;
#endif
};
/*---------------------------------------------------------------------------*/
/**
 * \brief           MQTT event callback function
 * \param m         A pointer to a MQTT connection
 * \param event     The event number
 * \param data      A user-defined pointer
 *
 * The MQTT socket event callback function gets called whenever there is an
 * event on a MQTT connection, such as the connection getting connected
 * or closed.
 */
typedef void (*mqtt_event_callback_t)(struct mqtt_connection *m,
                                      mqtt_event_t event,
                                      void *data);

typedef void (*mqtt_topic_callback_t)(struct mqtt_connection *m,
                                      struct mqtt_message *msg);
/*---------------------------------------------------------------------------*/
struct mqtt_will {
  struct mqtt_string topic;
  struct mqtt_string message;
  mqtt_qos_level_t qos;
#if MQTT_5
  LIST_STRUCT(properties);
#endif
};

struct mqtt_credentials {
  struct mqtt_string username;
  struct mqtt_string password;
};

struct mqtt_connection {
  /* Used by the list interface, must be first in the struct */
  struct mqtt_connection *next;
  struct timer t;

  struct mqtt_string client_id;

  uint8_t connect_vhdr_flags;
  uint8_t auto_reconnect;

  uint16_t keep_alive;
  struct ctimer keep_alive_timer;
  uint8_t waiting_for_pingresp;

  struct mqtt_will will;
  struct mqtt_credentials credentials;

  mqtt_conn_state_t state;
  mqtt_event_callback_t event_callback;

  /* Internal data */
  uint16_t mid_counter;

  /* Used for communication between MQTT API and APP */
  uint8_t out_queue_full;
  struct process *app_process;

  /* Outgoing data related */
  uint8_t *out_buffer_ptr;
  uint8_t out_buffer[MQTT_TCP_OUTPUT_BUFF_SIZE];
  uint8_t out_buffer_sent;
  struct mqtt_out_packet out_packet;
  struct pt out_proto_thread;
  uint32_t out_write_pos;
  uint16_t max_segment_size;

  /* Incoming data related */
  uint8_t in_buffer[MQTT_TCP_INPUT_BUFF_SIZE];
  struct mqtt_in_packet in_packet;
  struct mqtt_message in_publish_msg;

  /* TCP related information */
  char *server_host;
  uip_ipaddr_t server_ip;
  uint16_t server_port;
  struct tcp_socket socket;

#if MQTT_5
  /* Server Capabilities */
  /* Binary capabilities (default: enabled) */
  uint8_t srv_feature_en;
  struct mqtt_prop_list *out_props;
#endif
};
/* This is the API exposed to the user. */
/*---------------------------------------------------------------------------*/
/**
 * \brief Initializes the MQTT engine.
 * \param conn A pointer to the MQTT connection.
 * \param app_process A pointer to the application process handling the MQTT
 *        connection.
 * \param client_id A pointer to the MQTT client ID.
 * \param event_callback Callback function responsible for handling the
 *        callback from MQTT engine.
 * \param max_segment_size The TCP segment size to use for this MQTT/TCP
 *        connection.
 * \return MQTT_STATUS_OK or MQTT_STATUS_INVALID_ARGS_ERROR
 *
 * This function initializes the MQTT engine and shall be called before any
 * other MQTT function.
 */
mqtt_status_t mqtt_register(struct mqtt_connection *conn,
                            struct process *app_process,
                            char *client_id,
                            mqtt_event_callback_t event_callback,
                            uint16_t max_segment_size);
/*---------------------------------------------------------------------------*/
/**
 * \brief Connects to a MQTT broker.
 * \param conn A pointer to the MQTT connection.
 * \param host IP address of the broker to connect to.
 * \param port Port of the broker to connect to, default is MQTT port is 1883.
 * \param keep_alive Keep alive timer in seconds. Used by broker to handle
 *        client disc. Defines the maximum time interval between two messages
 *        from the client. Shall be min 1.5 x report interval.
 * \param clean_session Request a new session and discard pending messages with
 *        QoS > 0, as well as client subscriptions
 * \param prop_list Output properties (MQTTv5-only).
 * \return MQTT_STATUS_OK or an error status
 *
 * This function connects to a MQTT broker.
 */
mqtt_status_t mqtt_connect(struct mqtt_connection *conn,
                           char *host,
                           uint16_t port,
                           uint16_t keep_alive,
#if MQTT_5
                           uint8_t clean_session,
                           struct mqtt_prop_list *prop_list);
#else
                           uint8_t clean_session);
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Disconnects from a MQTT broker.
 * \param conn A pointer to the MQTT connection.
 * \param prop_list Output properties (MQTTv5-only).
 *
 * This function disconnects from a MQTT broker.
 */
#if MQTT_5
void mqtt_disconnect(struct mqtt_connection *conn,
                     struct mqtt_prop_list *prop_list);
#else
void mqtt_disconnect(struct mqtt_connection *conn);
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Subscribes to a MQTT topic.
 * \param conn A pointer to the MQTT connection.
 * \param mid A pointer to message ID.
 * \param topic A pointer to the topic to subscribe to.
 * \param qos_level Quality Of Service level to use. Currently supports 0, 1.
 * \param nl No Local (MQTTv5-only).
 * \param rap Retain As Published (MQTTv5-only).
 * \param ret_handling Retain handling options (MQTTv5-only).
 * \param prop_list Output properties (MQTTv5-only).
 * \return MQTT_STATUS_OK or some error status
 *
 * This function subscribes to a topic on a MQTT broker.
 */
mqtt_status_t mqtt_subscribe(struct mqtt_connection *conn,
                             uint16_t *mid,
                             char *topic,
#if MQTT_5
                             mqtt_qos_level_t qos_level,
                             mqtt_nl_en_t nl, mqtt_rap_en_t rap,
                             mqtt_retain_handling_t ret_handling,
                             struct mqtt_prop_list *prop_list);
#else
                             mqtt_qos_level_t qos_level);
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Unsubscribes from a MQTT topic.
 * \param conn A pointer to the MQTT connection.
 * \param mid A pointer to message ID.
 * \param topic A pointer to the topic to unsubscribe from.
 * \param prop_list Output properties (MQTTv5-only).
 * \return MQTT_STATUS_OK or some error status
 *
 * This function unsubscribes from a topic on a MQTT broker.
 */
mqtt_status_t mqtt_unsubscribe(struct mqtt_connection *conn,
                               uint16_t *mid,
#if MQTT_5
                               char *topic,
                               struct mqtt_prop_list *prop_list);
#else
                               char *topic);
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Publish to a MQTT topic.
 * \param conn A pointer to the MQTT connection.
 * \param mid A pointer to message ID.
 * \param topic A pointer to the topic to subscribe to.
 * \param payload A pointer to the topic payload.
 * \param payload_size Payload size.
 * \param qos_level Quality Of Service level to use. Currently supports 0, 1.
 * \param retain If the RETAIN flag is set to 1, in a PUBLISH Packet sent by a
 *        Client to a Server, the Server MUST store the Application Message
 *        and its QoS, so that it can be delivered to future subscribers whose
 *        subscriptions match its topic name
 * \param topic_alias Topic alias to send (MQTTv5-only).
 * \param topic_alias_en Control whether or not to discard topic and only send
 *        topic alias s(MQTTv5-only).
 * \param prop_list Output properties (MQTTv5-only).
 * \return MQTT_STATUS_OK or some error status
 *
 * This function publishes to a topic on a MQTT broker.
 */
mqtt_status_t mqtt_publish(struct mqtt_connection *conn,
                           uint16_t *mid,
                           char *topic,
                           uint8_t *payload,
                           uint32_t payload_size,
                           mqtt_qos_level_t qos_level,
#if MQTT_5
                           mqtt_retain_t retain,
                           uint8_t topic_alias,
                           mqtt_topic_alias_en_t topic_alias_en,
                           struct mqtt_prop_list *prop_list);
#else
                           mqtt_retain_t retain);
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Set the user name and password for a MQTT client.
 * \param conn A pointer to the MQTT connection.
 * \param username A pointer to the user name.
 * \param password A pointer to the password.
 *
 * This function sets clients user name and password to use when connecting to
 * a MQTT broker.
 */
void mqtt_set_username_password(struct mqtt_connection *conn,
                                char *username,
                                char *password);
/*---------------------------------------------------------------------------*/
/**
 * \brief Set the last will topic and message for a MQTT client.
 * \param conn A pointer to the MQTT connection.
 * \param topic A pointer to the Last Will topic.
 * \param message A pointer to the Last Will message (payload).
 * \param qos The desired QoS level.
 * \param will_props Will message properties (MQTTv5-only).
 *
 * This function sets clients Last Will topic and message (payload).
 * If the Will Flag is set to 1 (using the function) this indicates that,
 * if the Connect request is accepted, a Will Message MUST be stored on the
 * Server and associated with the Network Connection. The Will Message MUST
 * be published when the Network Connection is subsequently closed.
 *
 * This functionality can be used to get notified that a device has
 * disconnected from the broker.
 *
 */
void mqtt_set_last_will(struct mqtt_connection *conn,
                        char *topic,
                        char *message,
#if MQTT_5
                        mqtt_qos_level_t qos,
                        struct mqtt_prop_list *will_props);
#else
                        mqtt_qos_level_t qos);
#endif

#define mqtt_connected(conn) \
  ((conn)->state == MQTT_CONN_STATE_CONNECTED_TO_BROKER ? 1 : 0)

#define mqtt_ready(conn) \
  (!(conn)->out_queue_full && mqtt_connected((conn)))
/*---------------------------------------------------------------------------*/
void mqtt_encode_var_byte_int(uint8_t *vbi_out,
                              uint8_t *vbi_bytes,
                              uint32_t val);
/*---------------------------------------------------------------------------*/
uint8_t mqtt_decode_var_byte_int(const uint8_t *input_data_ptr,
                                 int input_data_len,
                                 uint32_t *input_pos,
                                 uint32_t *pkt_byte_count,
                                 uint16_t *dest);
/*---------------------------------------------------------------------------*/
/**
 * \brief Send authentication message (MQTTv5-only).
 * \param conn A pointer to the MQTT connection.
 * \param auth_type The type of auth to send (continue authentication or
 *        re-authentication).
 * \param prop_list Output properties.
 * \return MQTT_STATUS_OK or some error status
 *
 * This function send an MQTT authentication message.
 */
mqtt_status_t mqtt_auth(struct mqtt_connection *conn,
                        mqtt_auth_type_t auth_type,
                        struct mqtt_prop_list *prop_list);
/*---------------------------------------------------------------------------*/
#endif /* MQTT_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
