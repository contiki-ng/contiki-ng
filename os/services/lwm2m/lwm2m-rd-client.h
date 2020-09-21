/*
 * Copyright (c) 2016-2018, SICS Swedish ICT AB.
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
 *         Header file for the Contiki OMA LWM2M Registration and Bootstrap
 *         Client.
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */

#ifndef LWM2M_RD_CLIENT_H_
#define LWM2M_RD_CLIENT_H_

/* The type of server to use for registration: bootstrap or LWM2M */
typedef enum {
  LWM2M_RD_CLIENT_BOOTSTRAP_SERVER,
  LWM2M_RD_CLIENT_LWM2M_SERVER
} lwm2m_rd_client_server_type_t;

/* Session callback states */
#define LWM2M_RD_CLIENT_BOOTSTRAPPED       1
#define LWM2M_RD_CLIENT_REGISTERED         2
#define LWM2M_RD_CLIENT_DEREGISTERED       3
#define LWM2M_RD_CLIENT_DEREGISTER_FAILED  4
#define LWM2M_RD_CLIENT_DISCONNECTED       5

#define LWM2M_PROTOCOL_VERSION     "1.0"

#include "lwm2m-object.h"
#include "lwm2m-queue-mode-conf.h"
#include "coap-endpoint.h"
#include "coap-callback-api.h"

struct lwm2m_session_info;
typedef void (*session_callback_t)(struct lwm2m_session_info *session, int status);

#ifndef LWM2M_RD_CLIENT_ASSIGNED_ENDPOINT_MAX_LEN
#define LWM2M_RD_CLIENT_ASSIGNED_ENDPOINT_MAX_LEN    15
#endif /* LWM2M_RD_CLIENT_ASSIGNED_ENDPOINT_MAX_LEN */
/*---------------------------------------------------------------------------*/
/*- Server session----------------------------*/
/*---------------------------------------------------------------------------*/
typedef struct lwm2m_session_info {

  struct lwm2m_session_info *next;
  /* Information */
  const char *ep;
  const char *binding;
  char assigned_ep[LWM2M_RD_CLIENT_ASSIGNED_ENDPOINT_MAX_LEN];
  uint16_t lifetime;
  coap_endpoint_t bs_server_ep;
  coap_endpoint_t server_ep;
  lwm2m_rd_client_server_type_t use_server_type;
  uint8_t has_bs_server_info;
  uint8_t has_registration_server_info;
  uint8_t bootstrapped; /* bootstrap done */
  session_callback_t callback;

  /* CoAP Request */
  coap_callback_request_state_t rd_request_state;
  coap_message_t request[1];      /* This way the message can be treated as pointer as usual. */

  /* RD parameters */
  uint8_t rd_state;
  uint8_t rd_flags;
  uint64_t wait_until_network_check;
  uint64_t last_update;
  uint64_t last_rd_progress;

  /* Blosk Transfer */
  uint32_t rd_block1;
  uint8_t rd_more;
  void (*rd_callback)(coap_callback_request_state_t *state);
  coap_timer_t block1_timer;
} lwm2m_session_info_t;

int  lwm2m_rd_client_is_registered(lwm2m_session_info_t *session_info);
void lwm2m_rd_client_register_with_server(lwm2m_session_info_t *session_info, const coap_endpoint_t *server, lwm2m_rd_client_server_type_t server_type);
uint16_t lwm2m_rd_client_get_lifetime(lwm2m_session_info_t *session_info);
void lwm2m_rd_client_set_lifetime(lwm2m_session_info_t *session_info, uint16_t lifetime);
void lwm2m_rd_client_set_endpoint_name(lwm2m_session_info_t *session_info, const char *endpoint);
void lwm2m_rd_client_set_default_endpoint_name(const char *endpoint);

/* Indicate that something in the object list have changed */
void lwm2m_rd_client_set_update_rd(void);
/* Control if the object list should be automatically updated at updates of lifetime */
void lwm2m_rd_client_set_automatic_update(lwm2m_session_info_t *session_info, int update);
/* trigger an immediate update */
void lwm2m_rd_client_update_triggered(const coap_endpoint_t *server_ep);

int  lwm2m_rd_client_deregister(lwm2m_session_info_t *session_info);
void lwm2m_rd_client_init(const char *ep);

void lwm2m_rd_client_set_session_callback(lwm2m_session_info_t *session_info, session_callback_t cb);

#if LWM2M_QUEUE_MODE_ENABLED
uint8_t lwm2m_rd_client_is_client_awake(void);
void lwm2m_rd_client_restart_client_awake_timer(void);
void lwm2m_rd_client_fsm_execute_queue_mode_awake();
void lwm2m_rd_client_fsm_execute_queue_mode_update();
#endif

#endif /* LWM2M_RD_CLIENT_H_ */
/** @} */
