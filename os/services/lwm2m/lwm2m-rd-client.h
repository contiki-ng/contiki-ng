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

#define LWM2M_RD_CLIENT_BOOTSTRAPPED       1
#define LWM2M_RD_CLIENT_REGISTERED         2
#define LWM2M_RD_CLIENT_DEREGISTERED       3
#define LWM2M_RD_CLIENT_DEREGISTER_FAILED  4
#define LWM2M_RD_CLIENT_DISCONNECTED       5

#include "lwm2m-object.h"
#include "lwm2m-queue-mode-conf.h"

struct lwm2m_session_info;
typedef void (*session_callback_t)(struct lwm2m_session_info *session, int status);

int  lwm2m_rd_client_is_registered(void);
void lwm2m_rd_client_use_bootstrap_server(int use);
void lwm2m_rd_client_use_registration_server(int use);
void lwm2m_rd_client_register_with_server(const coap_endpoint_t *server);
void lwm2m_rd_client_register_with_bootstrap_server(const coap_endpoint_t *server);
uint16_t lwm2m_rd_client_get_lifetime(void);
void lwm2m_rd_client_set_lifetime(uint16_t lifetime);
/* Indicate that something in the object list have changed */
void lwm2m_rd_client_set_update_rd(void);
/* Control if the object list should be automatically updated at updates of lifetime */
void lwm2m_rd_client_set_automatic_update(int update);
/* trigger an immediate update */
void lwm2m_rd_client_update_triggered(void);

int  lwm2m_rd_client_deregister(void);
void lwm2m_rd_client_init(const char *ep);

void lwm2m_rd_client_set_session_callback(session_callback_t cb);

#if LWM2M_QUEUE_MODE_ENABLED
uint8_t lwm2m_rd_client_is_client_awake(void);
void lwm2m_rd_client_restart_client_awake_timer(void);
void lwm2m_rd_client_fsm_execute_queue_mode_awake();
void lwm2m_rd_client_fsm_execute_queue_mode_update();
#endif

#ifndef LWM2M_RD_CLIENT_ASSIGNED_ENDPOINT_MAX_LEN
#define LWM2M_RD_CLIENT_ASSIGNED_ENDPOINT_MAX_LEN    15
#endif /* LWM2M_RD_CLIENT_ASSIGNED_ENDPOINT_MAX_LEN */

/*---------------------------------------------------------------------------*/
/*- Server session-*Currently single session only*---------------------------*/
/*---------------------------------------------------------------------------*/
struct lwm2m_session_info {
  const char *ep;
  const char *binding;
  char assigned_ep[LWM2M_RD_CLIENT_ASSIGNED_ENDPOINT_MAX_LEN];
  uint16_t lifetime;
  coap_endpoint_t bs_server_ep;
  coap_endpoint_t server_ep;
  uint8_t use_bootstrap;
  uint8_t has_bs_server_info;
  uint8_t use_registration;
  uint8_t has_registration_server_info;
  uint8_t registered;
  uint8_t bootstrapped; /* bootstrap done */
  session_callback_t callback;
};

#endif /* LWM2M_RD_CLIENT_H_ */
/** @} */
