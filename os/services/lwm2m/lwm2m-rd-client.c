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
 *         Registration and bootstrap client
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Joel Hoglund <joel@sics.se>
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */
#include "lwm2m-engine.h"
#include "lwm2m-object.h"
#include "lwm2m-device.h"
#include "lwm2m-plain-text.h"
#include "lwm2m-json.h"
#include "lwm2m-rd-client.h"
#include "coap.h"
#include "coap-engine.h"
#include "coap-endpoint.h"
#include "coap-callback-api.h"
#include "lwm2m-security.h"
#include "lib/list.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#if UIP_CONF_IPV6_RPL
#include "rpl.h"
#endif /* UIP_CONF_IPV6_RPL */

#if LWM2M_QUEUE_MODE_ENABLED
#include "lwm2m-queue-mode.h"
#include "lwm2m-notification-queue.h"
#endif /* LWM2M_QUEUE_MODE_ENABLED */

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "lwm2m-rd"
#define LOG_LEVEL  LOG_LEVEL_LWM2M

#ifndef LWM2M_DEFAULT_CLIENT_LIFETIME
#define LWM2M_DEFAULT_CLIENT_LIFETIME 30 /* sec */
#endif

#define REMOTE_PORT        UIP_HTONS(COAP_DEFAULT_PORT)
#define BS_REMOTE_PORT     UIP_HTONS(5685)

#define STATE_MACHINE_UPDATE_INTERVAL 500

/* The states for the RD client state machine */
/* When node is unregistered it ends up in UNREGISTERED
   and this is going to be there until use X or Y kicks it
   back into INIT again */
#define INIT               0
#define WAIT_NETWORK       1
#define DO_BOOTSTRAP       3
#define BOOTSTRAP_SENT     4
#define BOOTSTRAP_DONE     5
#define DO_REGISTRATION    6
#define REGISTRATION_SENT  7
#define REGISTRATION_DONE  8
#define UPDATE_SENT        9
#define DEREGISTER        10
#define DEREGISTER_SENT   11
#define DEREGISTER_FAILED 12
#define DEREGISTERED      13
#if LWM2M_QUEUE_MODE_ENABLED
#define QUEUE_MODE_AWAKE 14
#define QUEUE_MODE_SEND_UPDATE 15
#endif

#define FLAG_RD_DATA_DIRTY            0x01
#define FLAG_RD_DATA_UPDATE_TRIGGERED 0x02
#define FLAG_RD_DATA_UPDATE_ON_DIRTY  0x10

LIST(session_info_list);

/* Shared by all sessions, used by only one at a time in the FSM */
static char query_data[64]; /* allocate some data for queries and updates */
static uint8_t rd_data[128]; /* allocate some data for the RD */

static coap_timer_t rd_timer; /* Timer to tick the FSM periodically */
static char default_ep[20];

#if LWM2M_QUEUE_MODE_ENABLED
static coap_timer_t queue_mode_client_awake_timer; /* Timer to control the client's
                                                    * awake time
                                                    */
static uint8_t queue_mode_client_awake; /* 1 - client is awake,
                                         * 0 - client is sleeping
                                         */
static uint16_t queue_mode_client_awake_time; /* The time to be awake */
/* Callback for the client awake timer */
static void queue_mode_awake_timer_callback(coap_timer_t *timer);
#endif

static void check_periodic_observations();
static void update_callback(coap_callback_request_state_t *callback_state);
/*---------------------------------------------------------------------------*/
static int
set_rd_data(lwm2m_session_info_t *session_info)
{
  lwm2m_buffer_t outbuf;

  /* setup the output buffer */
  outbuf.buffer = rd_data;
  outbuf.size = sizeof(rd_data);
  outbuf.len = 0;

  /* this will also set the request payload */
  session_info->rd_more = lwm2m_engine_set_rd_data(&outbuf, 0);
  coap_set_payload(session_info->request, rd_data, outbuf.len);

  if(session_info->rd_more) {
    /* set the first block here */
    LOG_DBG("Setting block1 in request\n");
    coap_set_header_block1(session_info->request, 0, 1, sizeof(rd_data));
  }
  return outbuf.len;
}
/*---------------------------------------------------------------------------*/
static void
prepare_update(lwm2m_session_info_t *session_info, int triggered)
{
  coap_init_message(session_info->request, COAP_TYPE_CON, COAP_POST, 0);
  coap_set_header_uri_path(session_info->request, session_info->assigned_ep);

  snprintf(query_data, sizeof(query_data) - 1, "?lt=%d&b=%s", session_info->lifetime, session_info->binding);
  LOG_DBG("UPDATE:%s %s\n", session_info->assigned_ep, query_data);
  coap_set_header_uri_query(session_info->request, query_data);

  if((triggered || session_info->rd_flags & FLAG_RD_DATA_UPDATE_ON_DIRTY) && (session_info->rd_flags & FLAG_RD_DATA_DIRTY)) {
    session_info->rd_flags &= ~FLAG_RD_DATA_DIRTY;
    set_rd_data(session_info);
    session_info->rd_callback = update_callback;
  }
}
/*---------------------------------------------------------------------------*/
static int
has_network_access(void)
{
#if UIP_CONF_IPV6_RPL
/* NATIVE PLATFORM is not really running RPL */
#ifndef CONTIKI_TARGET_NATIVE
  if(rpl_get_any_dag() == NULL) {
    return 0;
  }
#endif
#endif /* UIP_CONF_IPV6_RPL */
  return 1;
}
/*---------------------------------------------------------------------------*/
int
lwm2m_rd_client_is_registered(lwm2m_session_info_t *session_info)
{
  return session_info->rd_state == REGISTRATION_DONE || session_info->rd_state == UPDATE_SENT;
}
/*---------------------------------------------------------------------------*/
/* will take another argument when we support multiple sessions */
void
lwm2m_rd_client_set_session_callback(lwm2m_session_info_t *session_info, session_callback_t cb)
{
  session_info->callback = cb;
}
/*---------------------------------------------------------------------------*/
static void
perform_session_callback(lwm2m_session_info_t *session_info, int state)
{
  if(session_info->callback != NULL) {
    LOG_DBG("Performing session callback: %d cb:%p\n",
            state, session_info->callback);
    session_info->callback(session_info, state);
  }
}
/*---------------------------------------------------------------------------*/
uint16_t
lwm2m_rd_client_get_lifetime(lwm2m_session_info_t *session_info)
{
  return session_info->lifetime;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_rd_client_set_lifetime(lwm2m_session_info_t *session_info, uint16_t lifetime)
{
  if(lifetime > 0) {
    session_info->lifetime = lifetime;
  } else {
    session_info->lifetime = LWM2M_DEFAULT_CLIENT_LIFETIME;
  }
}
/*---------------------------------------------------------------------------*/
void
lwm2m_rd_client_set_endpoint_name(lwm2m_session_info_t *session_info, const char *endpoint)
{
  if(endpoint != NULL) {
    session_info->ep = endpoint;
  }
}
/*---------------------------------------------------------------------------*/
void
lwm2m_rd_client_set_default_endpoint_name(const char *endpoint)
{
  strncpy(default_ep, endpoint, sizeof(default_ep) - 1);
  default_ep[sizeof(default_ep) - 1] = '\0';
}
/*---------------------------------------------------------------------------*/
void
lwm2m_rd_client_set_update_rd(void)
{
  lwm2m_session_info_t *session_info = (lwm2m_session_info_t *)list_head(session_info_list);
  while(session_info != NULL) {
    session_info->rd_flags |= FLAG_RD_DATA_DIRTY;
    session_info = session_info->next;
  }
}
/*---------------------------------------------------------------------------*/
void
lwm2m_rd_client_set_automatic_update(lwm2m_session_info_t *session_info, int update)
{
  session_info->rd_flags = (session_info->rd_flags & ~FLAG_RD_DATA_UPDATE_ON_DIRTY) |
    (update != 0 ? FLAG_RD_DATA_UPDATE_ON_DIRTY : 0);
}
/*---------------------------------------------------------------------------*/
void
lwm2m_rd_client_register_with_server(lwm2m_session_info_t *session_info,
                                     const coap_endpoint_t *server,
                                     lwm2m_rd_client_server_type_t server_type)
{
  if(session_info->ep == NULL) {
    session_info->ep = default_ep;
  }
  /* default binding U = UDP, UQ = UDP Q-mode*/
#if LWM2M_QUEUE_MODE_CONF_ENABLED
  session_info->binding = "UQ";
  /* Enough margin to ensure that the client is not unregistered (we
   * do not know the time it can stay awake)
   */
  session_info->lifetime = (LWM2M_QUEUE_MODE_DEFAULT_CLIENT_SLEEP_TIME / 1000) * 2;
#else
  session_info->binding = "U";
  if(session_info->lifetime == 0) {
    session_info->lifetime = LWM2M_DEFAULT_CLIENT_LIFETIME;
  }
#endif /* LWM2M_QUEUE_MODE_CONF_ENABLED */

  session_info->rd_flags = FLAG_RD_DATA_UPDATE_ON_DIRTY;
  session_info->has_bs_server_info = 0;
  session_info->has_registration_server_info = 0;
  session_info->wait_until_network_check = 0;
  session_info->last_update = 0;
  session_info->last_rd_progress = 0;
  session_info->bootstrapped = 0;
  session_info->rd_state = INIT;

  if(server_type == LWM2M_RD_CLIENT_BOOTSTRAP_SERVER) {
    coap_endpoint_copy(&session_info->bs_server_ep, server);
    session_info->has_bs_server_info = 1;
    session_info->use_server_type = LWM2M_RD_CLIENT_BOOTSTRAP_SERVER;
  } else {
    coap_endpoint_copy(&session_info->server_ep, server);
    session_info->use_server_type = LWM2M_RD_CLIENT_LWM2M_SERVER;
    session_info->has_registration_server_info = 1;
  }

  list_add(session_info_list, session_info); /* Add to the list of sessions */
}
/*---------------------------------------------------------------------------*/
int
lwm2m_rd_client_deregister(lwm2m_session_info_t *session_info)
{
  if(lwm2m_rd_client_is_registered(session_info)) {
    session_info->rd_state = DEREGISTER;
    return 1;
  }
  /* Not registered */
  return 0;
}
/*---------------------------------------------------------------------------*/
static lwm2m_session_info_t *
get_session_info_from_server_ep(const coap_endpoint_t *server_ep)
{
  lwm2m_session_info_t *session_info = (lwm2m_session_info_t *)list_head(session_info_list);
  while(session_info != NULL) {
    if(coap_endpoint_cmp(&session_info->server_ep, server_ep)) {
      return session_info;
    }
    session_info = session_info->next;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_rd_client_update_triggered(const coap_endpoint_t *server_ep)
{
  lwm2m_session_info_t *session_info = get_session_info_from_server_ep(server_ep);
  if(session_info) {
    session_info->rd_flags |= FLAG_RD_DATA_UPDATE_TRIGGERED;
  }
  /* Here we need to do an CoAP timer poll - to get a quick request transmission! */
}
/*---------------------------------------------------------------------------*/
/*
 * A client initiated bootstrap starts with a POST to /bs?ep={session_info.ep},
 * on the bootstrap server. The server should reply with 2.04.
 * The server will thereafter do DELETE and or PUT to write new client objects.
 * The bootstrap finishes with the server doing POST to /bs on the client.
 *
 * Page 64 in 07 April 2016 spec.
 *
 * TODO
 */
static void
bootstrap_callback(coap_callback_request_state_t *callback_state)
{
  coap_request_state_t *state = &callback_state->state;
  LOG_DBG("Bootstrap callback Response: %d, ", state->response != NULL);

  lwm2m_session_info_t *session_info = (lwm2m_session_info_t *)state->user_data;

  if(state->status == COAP_REQUEST_STATUS_RESPONSE) {
    if(CHANGED_2_04 == state->response->code) {
      LOG_DBG_("Considered done!\n");
      session_info->rd_state = BOOTSTRAP_DONE;
      return;
    }
    /* Possible error response codes are 4.00 Bad request & 4.15 Unsupported content format */
    LOG_DBG_("Failed with code %d. Retrying\n", state->response->code);
    /* TODO Application callback? */
    session_info->rd_state = INIT;
  } else if(state->status == COAP_REQUEST_STATUS_TIMEOUT) {
    LOG_DBG_("Server not responding! Retry?");
    session_info->rd_state = DO_BOOTSTRAP;
  } else if(state->status == COAP_REQUEST_STATUS_FINISHED) {
    LOG_DBG_("Request finished. Ignore\n");
  } else {
    LOG_DBG_("Unexpected error! Retry?");
    session_info->rd_state = DO_BOOTSTRAP;
  }
}
/*---------------------------------------------------------------------------*/
static void
produce_more_rd(lwm2m_session_info_t *session_info)
{
  lwm2m_buffer_t outbuf;

  LOG_DBG("GOT Continue!\n");

  /* setup the output buffer */
  outbuf.buffer = rd_data;
  outbuf.size = sizeof(rd_data);
  outbuf.len = 0;

  session_info->rd_block1++;

  /* this will also set the request payload */
  session_info->rd_more = lwm2m_engine_set_rd_data(&outbuf, session_info->rd_block1);
  coap_set_payload(session_info->request, rd_data, outbuf.len);

  LOG_DBG("Setting block1 in request - block: %d more: %d\n",
          (int)session_info->rd_block1, (int)session_info->rd_more);
  coap_set_header_block1(session_info->request, session_info->rd_block1, session_info->rd_more, sizeof(rd_data));

  coap_send_request(&session_info->rd_request_state, &session_info->server_ep, session_info->request, session_info->rd_callback);
}
/*---------------------------------------------------------------------------*/
static void
block1_rd_callback(coap_timer_t *timer)
{
  produce_more_rd((lwm2m_session_info_t *)timer->user_data);
}
/*---------------------------------------------------------------------------*/
/*
 * Page 65-66 in 07 April 2016 spec.
 */
static void
registration_callback(coap_callback_request_state_t *callback_state)
{
  coap_request_state_t *state = &callback_state->state;
  LOG_DBG("Registration callback. Status: %d. Response: %d, ", state->status, state->response != NULL);
  lwm2m_session_info_t *session_info = (lwm2m_session_info_t *)state->user_data;

  if(state->status == COAP_REQUEST_STATUS_RESPONSE) {
    /* check state and possibly set registration to done */
    /* If we get a continue - we need to call the rd generator one more time */

    if(CONTINUE_2_31 == state->response->code) {
      /* We assume that size never change?! */
      coap_get_header_block1(state->response, &session_info->rd_block1, NULL, NULL, NULL);
      coap_timer_set_user_data(&session_info->block1_timer, (void *)session_info);
      coap_timer_set_callback(&session_info->block1_timer, block1_rd_callback);
      coap_timer_set(&session_info->block1_timer, 1); /* delay 1 ms */
      LOG_DBG_("Continue\n");
    } else if(CREATED_2_01 == state->response->code) {
      if(state->response->location_path_len < LWM2M_RD_CLIENT_ASSIGNED_ENDPOINT_MAX_LEN) {
        memcpy(session_info->assigned_ep, state->response->location_path,
               state->response->location_path_len);
        session_info->assigned_ep[state->response->location_path_len] = 0;
        /* if we decide to not pass the lt-argument on registration, we should force an initial "update" to register lifetime with server */
#if LWM2M_QUEUE_MODE_ENABLED
#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
        if(lwm2m_queue_mode_get_dynamic_adaptation_flag()) {
          lwm2m_queue_mode_set_first_request();
        }
#endif
        lwm2m_rd_client_fsm_execute_queue_mode_awake(session_info); /* Avoid 500 ms delay and move directly to the state*/
#else
        session_info->rd_state = REGISTRATION_DONE;
#endif
        /* remember the last reg time */
        session_info->last_update = coap_timer_uptime();
        LOG_DBG_("Done (assigned EP='%s')!\n", session_info->assigned_ep);
        perform_session_callback(session_info, LWM2M_RD_CLIENT_REGISTERED);
        return;
      }

      LOG_DBG_("failed to handle assigned EP: '");
      LOG_DBG_COAP_STRING(state->response->location_path,
                          state->response->location_path_len);
      LOG_DBG_("'. Re-init network.\n");
    } else {
      /* Possible error response codes are 4.00 Bad request & 4.03 Forbidden */
      LOG_DBG_("failed with code %d. Re-init network\n", state->response->code);
    }
    /* TODO Application callback? */
    session_info->rd_state = INIT;
  } else if(state->status == COAP_REQUEST_STATUS_TIMEOUT) {
    LOG_DBG_("Server not responding, trying to reconnect\n");
    session_info->rd_state = INIT;
  } else if(state->status == COAP_REQUEST_STATUS_FINISHED) {
    LOG_DBG_("Request finished. Ignore\n");
  } else {
    LOG_DBG_("Unexpected error, trying to reconnect\n");
    session_info->rd_state = INIT;
  }
}
/*---------------------------------------------------------------------------*/
/*
 * Page 65-66 in 07 April 2016 spec.
 */
static void
update_callback(coap_callback_request_state_t *callback_state)
{
  coap_request_state_t *state = &callback_state->state;
  LOG_DBG("Update callback. Status: %d. Response: %d, ", state->status, state->response != NULL);

  lwm2m_session_info_t *session_info = (lwm2m_session_info_t *)state->user_data;

  if(state->status == COAP_REQUEST_STATUS_RESPONSE) {
    /* If we get a continue - we need to call the rd generator one more time */
    if(CONTINUE_2_31 == state->response->code) {
      /* We assume that size never change?! */
      LOG_DBG_("Continue\n");
      coap_get_header_block1(state->response, &session_info->rd_block1, NULL, NULL, NULL);
      coap_timer_set_callback(&session_info->block1_timer, block1_rd_callback);
      coap_timer_set(&session_info->block1_timer, 1); /* delay 1 ms */
    } else if(CHANGED_2_04 == state->response->code) {
      LOG_DBG_("Done!\n");
      /* remember the last reg time */
      session_info->last_update = coap_timer_uptime();
#if LWM2M_QUEUE_MODE_ENABLED
      /* If it has been waked up by a notification, send the stored notifications in queue */
      if(lwm2m_queue_mode_is_waked_up_by_notification()) {

        lwm2m_queue_mode_clear_waked_up_by_notification();
        lwm2m_notification_queue_send_notifications();
      }
#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
      if(lwm2m_queue_mode_get_dynamic_adaptation_flag()) {
        lwm2m_queue_mode_set_first_request();
      }
#endif /* LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION */
      lwm2m_rd_client_fsm_execute_queue_mode_awake(session_info); /* Avoid 500 ms delay and move directly to the state*/
#else

      session_info->rd_state = REGISTRATION_DONE;
      session_info->rd_flags &= ~FLAG_RD_DATA_UPDATE_TRIGGERED;
#endif /* LWM2M_QUEUE_MODE_DEFAULT_CLIENT_SLEEP_TIME */
    } else {
      /* Possible error response codes are 4.00 Bad request & 4.04 Not Found */
      LOG_DBG_("Failed with code %d. Retrying registration\n",
               state->response->code);
      session_info->rd_state = DO_REGISTRATION;
    }
  } else if(state->status == COAP_REQUEST_STATUS_TIMEOUT) {
    LOG_DBG_("Server not responding, trying to reconnect\n");
    session_info->rd_state = INIT;
  } else if(state->status == COAP_REQUEST_STATUS_FINISHED) {
    LOG_DBG_("Request finished. Ignore\n");
  } else {
    LOG_DBG_("Unexpected error, trying to reconnect\n");
    session_info->rd_state = INIT;
  }
}
/*---------------------------------------------------------------------------*/
static void
deregister_callback(coap_callback_request_state_t *callback_state)
{
  coap_request_state_t *state = &callback_state->state;
  LOG_DBG("Deregister callback. Status: %d. Response Code: %d\n",
          state->status,
          state->response != NULL ? state->response->code : 0);

  lwm2m_session_info_t *session_info = (lwm2m_session_info_t *)state->user_data;

  if(state->status == COAP_REQUEST_STATUS_RESPONSE && (DELETED_2_02 == state->response->code)) {
    LOG_DBG("Deregistration success\n");
    session_info->rd_state = DEREGISTERED;
    perform_session_callback(session_info, LWM2M_RD_CLIENT_DEREGISTERED);
  } else {
    LOG_DBG("Deregistration failed\n");
    if(session_info->rd_state == DEREGISTER_SENT) {
      session_info->rd_state = DEREGISTER_FAILED;
      perform_session_callback(session_info, LWM2M_RD_CLIENT_DEREGISTER_FAILED);
    }
  }
}
/*---------------------------------------------------------------------------*/
#if LWM2M_QUEUE_MODE_ENABLED
static int
all_sessions_in_queue_mode_state(void)
{
  lwm2m_session_info_t *session_info = (lwm2m_session_info_t *)list_head(session_info_list);
  while(session_info != NULL) {
    if(((session_info->rd_state & 0xF) != 0xE)) {
      return 0;
    }
    session_info = session_info->next;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
all_sessions_in_queue_mode_awake(void)
{
  lwm2m_session_info_t *session_info = (lwm2m_session_info_t *)list_head(session_info_list);
  while(session_info != NULL) {
    if(session_info->rd_state != QUEUE_MODE_AWAKE) {
      return 0;
    }
    session_info = session_info->next;
  }
  return 1;
}
#endif /* LWM2M_QUEUE_MODE_ENABLED */
/*---------------------------------------------------------------------------*/
/* CoAP timer callback */
static void
periodic_process(coap_timer_t *timer)
{
  uint64_t now;

  /* reschedule the CoAP timer */
#if LWM2M_QUEUE_MODE_ENABLED
  /* In Queue Mode, the machine is not executed periodically, but with the awake/sleeping times */
  if(!all_sessions_in_queue_mode_state()) {
    coap_timer_reset(&rd_timer, STATE_MACHINE_UPDATE_INTERVAL);
  }
#else
  coap_timer_reset(&rd_timer, STATE_MACHINE_UPDATE_INTERVAL);
#endif

  now = coap_timer_uptime();

  lwm2m_session_info_t *session_info = (lwm2m_session_info_t *)list_head(session_info_list);
  while(session_info != NULL) {

    LOG_DBG("RD Client with assigned ep: %s - state: %d, ms: %lu\n", session_info->assigned_ep, session_info->rd_state,
            (unsigned long)coap_timer_uptime());

    switch(session_info->rd_state) {
    case INIT:
      LOG_DBG("RD Client started with endpoint '%s' and client lifetime %d\n", session_info->ep, session_info->lifetime);
      session_info->rd_state = WAIT_NETWORK;
      break;
    case WAIT_NETWORK:
      if(now > session_info->wait_until_network_check) {
        /* check each 10 seconds before next check */
        LOG_DBG("Checking for network... %lu\n",
                (unsigned long)session_info->wait_until_network_check);
        session_info->wait_until_network_check = now + 10000;
        if(has_network_access()) {
          /* Either do bootstrap then registration */
          if(session_info->use_server_type == LWM2M_RD_CLIENT_BOOTSTRAP_SERVER) {
            session_info->rd_state = DO_BOOTSTRAP;
          } else {
            session_info->rd_state = DO_REGISTRATION;
          }
        }
        /* Otherwise wait until for a network to join */
      }
      break;
    case DO_BOOTSTRAP:
      if(session_info->use_server_type == LWM2M_RD_CLIENT_BOOTSTRAP_SERVER &&
         session_info->bootstrapped == 0 &&
         session_info->has_bs_server_info) {

        /* prepare request, TID is set by COAP_BLOCKING_REQUEST() */
        coap_init_message(session_info->request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_header_uri_path(session_info->request, "/bs");

        snprintf(query_data, sizeof(query_data) - 1, "?ep=%s", session_info->ep);
        coap_set_header_uri_query(session_info->request, query_data);
        LOG_INFO("Registering ID with bootstrap server [");
        LOG_INFO_COAP_EP(&session_info->bs_server_ep);
        LOG_INFO_("] as '%s'\n", query_data);
        /* Add session info as user data to use it in the callbacks */
        session_info->rd_request_state.state.user_data = (void *)session_info;
        if(coap_send_request(&session_info->rd_request_state, &session_info->bs_server_ep,
                             session_info->request, bootstrap_callback)) {
          session_info->rd_state = BOOTSTRAP_SENT;
        }
      }
      break;
    case BOOTSTRAP_SENT:
      /* Just wait for bootstrap to be done...  */
      break;
    case BOOTSTRAP_DONE:
      /* check that we should still use bootstrap */
      if(session_info->use_server_type == LWM2M_RD_CLIENT_BOOTSTRAP_SERVER) {
        lwm2m_security_server_t *security;
        LOG_DBG("*** Bootstrap - checking for server info...\n");
        /* get the security object - ignore bootstrap servers */
        for(security = lwm2m_security_get_first();
            security != NULL;
            security = lwm2m_security_get_next(security)) {
          if(security->bootstrap == 0) {
            break;
          }
        }

        if(security != NULL) {
          /* get the server URI */
          if(security->server_uri_len > 0) {
            uint8_t secure = 0;

            LOG_DBG("**** Found security instance using: ");
            LOG_DBG_COAP_STRING((const char *)security->server_uri,
                                security->server_uri_len);
            LOG_DBG_(" (len %d) \n", security->server_uri_len);
            /* TODO Should verify it is a URI */
            /* Check if secure */
            secure = strncmp((const char *)security->server_uri,
                             "coaps:", 6) == 0;

            if(!coap_endpoint_parse((const char *)security->server_uri,
                                    security->server_uri_len,
                                    &session_info->server_ep)) {
              LOG_DBG("Failed to parse server URI!\n");
            } else {
              LOG_DBG("Server address:");
              LOG_DBG_COAP_EP(&session_info->server_ep);
              LOG_DBG_("\n");
              if(secure) {
                LOG_DBG("Secure CoAP requested but not supported - can not bootstrap\n");
              } else {
                lwm2m_rd_client_register_with_server(session_info, &session_info->server_ep, LWM2M_RD_CLIENT_LWM2M_SERVER);
                session_info->bootstrapped++;
              }
            }
          } else {
            LOG_DBG("** failed to parse URI ");
            LOG_DBG_COAP_STRING((const char *)security->server_uri,
                                security->server_uri_len);
            LOG_DBG_("\n");
          }
        }

        /* if we did not register above - then fail this and restart... */
        if(session_info->bootstrapped == 0) {
          /* Not ready. Lets retry with the bootstrap server again */
          session_info->rd_state = DO_BOOTSTRAP;
        } else {
          session_info->rd_state = DO_REGISTRATION;
        }
      }
      break;
    case DO_REGISTRATION:
      if(!coap_endpoint_is_connected(&session_info->server_ep)) {
        /* Not connected... wait a bit... and retry connection */
        coap_endpoint_connect(&session_info->server_ep);
        LOG_DBG("Wait until connected... \n");
        return;
      }

      if(session_info->use_server_type == LWM2M_RD_CLIENT_LWM2M_SERVER &&
         !lwm2m_rd_client_is_registered(session_info) &&
         session_info->has_registration_server_info) {
        int len;

        /* prepare request, TID was set by COAP_BLOCKING_REQUEST() */
        coap_init_message(session_info->request, COAP_TYPE_CON, COAP_POST, 0);
        coap_set_header_uri_path(session_info->request, "/rd");

        snprintf(query_data, sizeof(query_data) - 1, "?lwm2m=%s&ep=%s&lt=%d&b=%s",
                             LWM2M_PROTOCOL_VERSION, session_info->ep, session_info->lifetime, session_info->binding);
        coap_set_header_uri_query(session_info->request, query_data);

        len = set_rd_data(session_info);
        session_info->rd_callback = registration_callback;

        LOG_INFO("Registering with [");
        LOG_INFO_COAP_EP(&session_info->server_ep);
        LOG_INFO_("] lwm2m endpoint '%s': '", query_data);
        if(len) {
          LOG_INFO_COAP_STRING((const char *)rd_data, len);
        }
        LOG_INFO_("' More:%d\n", session_info->rd_more);

        /* Add session info as user data to use it in the callbacks */
        session_info->rd_request_state.state.user_data = (void *)session_info;
        if(coap_send_request(&session_info->rd_request_state, &session_info->server_ep,
                             session_info->request, registration_callback)) {

          session_info->rd_state = REGISTRATION_SENT;
        }
        session_info->last_rd_progress = coap_timer_uptime();
      }
      break;
    case REGISTRATION_SENT:
      /* just wait until the callback kicks us to the next state... */
      break;
    case REGISTRATION_DONE:
      /* All is done! */

      check_periodic_observations(); /* TODO: manage periodic observations */

      /* check if it is time for the next update */
      if((session_info->rd_flags & FLAG_RD_DATA_UPDATE_TRIGGERED) ||
         ((uint32_t)session_info->lifetime * 500) <= now - session_info->last_update) {
        /* triggered or time to send an update to the server, at half-time! sec vs ms */
        prepare_update(session_info, session_info->rd_flags & FLAG_RD_DATA_UPDATE_TRIGGERED);

        /* Add session info as user data to use it in the callbacks */
        session_info->rd_request_state.state.user_data = (void *)session_info;
        if(coap_send_request(&session_info->rd_request_state, &session_info->server_ep, session_info->request,
                             update_callback)) {
          session_info->rd_state = UPDATE_SENT;
        }
        session_info->last_rd_progress = coap_timer_uptime();
      }
      break;

#if LWM2M_QUEUE_MODE_ENABLED
    case QUEUE_MODE_AWAKE:
      LOG_DBG("Queue Mode: Client is AWAKE at %lu\n", (unsigned long)coap_timer_uptime());
      if((queue_mode_client_awake = all_sessions_in_queue_mode_awake())) {
        queue_mode_client_awake_time = lwm2m_queue_mode_get_awake_time();
        coap_timer_set(&queue_mode_client_awake_timer, queue_mode_client_awake_time);
      }
      break;
    case QUEUE_MODE_SEND_UPDATE:
      /* Define this macro to make the necessary actions for waking up,
       * depending on the platform
       */
#ifdef LWM2M_QUEUE_MODE_WAKE_UP
      LWM2M_QUEUE_MODE_WAKE_UP();
#endif /* LWM2M_QUEUE_MODE_WAKE_UP */
      prepare_update(session_info, session_info->rd_flags & FLAG_RD_DATA_UPDATE_TRIGGERED);
      /* Add session info as user data to use it in the callbacks */
      session_info->rd_request_state.state.user_data = (void *)session_info;
      if(coap_send_request(&session_info->rd_request_state, &session_info->server_ep, session_info->request,
                           update_callback)) {
        session_info->rd_state = UPDATE_SENT;
      }
      session_info->last_rd_progress = coap_timer_uptime();
      break;
#endif /* LWM2M_QUEUE_MODE_ENABLED */

    case UPDATE_SENT:
      /* just wait until the callback kicks us to the next state... */
      break;
    case DEREGISTER:
      LOG_INFO("DEREGISTER %s\n", session_info->assigned_ep);
      coap_init_message(session_info->request, COAP_TYPE_CON, COAP_DELETE, 0);
      coap_set_header_uri_path(session_info->request, session_info->assigned_ep);

      /* Add session info as user data to use it in the callbacks */
      session_info->rd_request_state.state.user_data = (void *)session_info;
      if(coap_send_request(&session_info->rd_request_state, &session_info->server_ep, session_info->request,
                           deregister_callback)) {
        session_info->rd_state = DEREGISTER_SENT;
      }
      break;
    case DEREGISTER_SENT:
      break;
    case DEREGISTER_FAILED:
      break;
    case DEREGISTERED:
      break;

    default:
      LOG_WARN("Unhandled state: %d\n", session_info->rd_state);
    }
    session_info = session_info->next;
  }
}
/*---------------------------------------------------------------------------*/
void
lwm2m_rd_client_init(const char *ep)
{
  lwm2m_rd_client_set_default_endpoint_name(ep);

  /* call the RD client periodically */
  coap_timer_set_callback(&rd_timer, periodic_process);
  coap_timer_set(&rd_timer, STATE_MACHINE_UPDATE_INTERVAL);
#if LWM2M_QUEUE_MODE_ENABLED
  coap_timer_set_callback(&queue_mode_client_awake_timer, queue_mode_awake_timer_callback);
#endif
}
/*---------------------------------------------------------------------------*/
static void
check_periodic_observations(void)
{
/* TODO */
}
/*---------------------------------------------------------------------------*/
/*
 * Queue Mode Support
 */
#if LWM2M_QUEUE_MODE_ENABLED
/*---------------------------------------------------------------------------*/
void
lwm2m_rd_client_restart_client_awake_timer(void)
{
  coap_timer_set(&queue_mode_client_awake_timer, queue_mode_client_awake_time);
}
/*---------------------------------------------------------------------------*/
uint8_t
lwm2m_rd_client_is_client_awake(void)
{
  return queue_mode_client_awake;
}
/*---------------------------------------------------------------------------*/
static void
queue_mode_awake_timer_callback(coap_timer_t *timer)
{
  /* Timer has expired, no requests has been received, client can go to sleep */
  LOG_DBG("Queue Mode: Client is SLEEPING at %lu\n", (unsigned long)coap_timer_uptime());
  queue_mode_client_awake = 0;

  lwm2m_session_info_t *session_info = (lwm2m_session_info_t *)list_head(session_info_list);
  while(session_info != NULL) {
    session_info->rd_state = QUEUE_MODE_SEND_UPDATE;
    session_info = session_info->next;
  }
  coap_timer_set(&rd_timer, lwm2m_queue_mode_get_sleep_time());
  /* Define this macro to enter sleep mode depending on the platform */
#ifdef LWM2M_QUEUE_MODE_SLEEP_MS
  LWM2M_QUEUE_MODE_SLEEP_MS(lwm2m_queue_mode_get_sleep_time());
#endif /* LWM2M_QUEUE_MODE_SLEEP_MS */
}
/*---------------------------------------------------------------------------*/
void
lwm2m_rd_client_fsm_execute_queue_mode_awake(lwm2m_session_info_t *session_info)
{
  coap_timer_stop(&rd_timer);
  session_info->rd_state = QUEUE_MODE_AWAKE;
  periodic_process(&rd_timer);
}
/*---------------------------------------------------------------------------*/
void
lwm2m_rd_client_fsm_execute_queue_mode_update(lwm2m_session_info_t *session_info)
{
  coap_timer_stop(&rd_timer);
  session_info->rd_state = QUEUE_MODE_SEND_UPDATE;
  periodic_process(&rd_timer);
}
/*---------------------------------------------------------------------------*/
#endif /* LWM2M_QUEUE_MODE_ENABLED */
/*---------------------------------------------------------------------------*/
/** @} */
