/*
 * Copyright (c) 2017, RISE SICS AB.
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
 *         Implementation of the Contiki OMA LWM2M Queue Mode for managing the parameters
 * \author
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */

#include "lwm2m-queue-mode.h"

#if LWM2M_QUEUE_MODE_ENABLED

#include "lwm2m-engine.h"
#include "lwm2m-rd-client.h"
#include "lib/memb.h"
#include "lib/list.h"
#include <string.h>

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "lwm2m-queue-mode"
#define LOG_LEVEL  LOG_LEVEL_LWM2M

/* Queue Mode dynamic adaptation masks */
#define FIRST_REQUEST_MASK 0x01
#define HANDLER_FROM_NOTIFICATION_MASK 0x02

static uint16_t queue_mode_awake_time = LWM2M_QUEUE_MODE_DEFAULT_CLIENT_AWAKE_TIME;
static uint32_t queue_mode_sleep_time = LWM2M_QUEUE_MODE_DEFAULT_CLIENT_SLEEP_TIME;

/* Flag for notifications */
static uint8_t waked_up_by_notification;

/* For the dynamic adaptation of the awake time */
#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
static uint8_t queue_mode_dynamic_adaptation_flag = LWM2M_QUEUE_MODE_DEFAULT_DYNAMIC_ADAPTATION_FLAG;

/* Window to save the times and do the dynamic adaptation of the awake time*/
uint16_t times_window[LWM2M_QUEUE_MODE_DYNAMIC_ADAPTATION_WINDOW_LENGTH] = { 0 };
uint8_t times_window_index = 0;
static uint8_t dynamic_adaptation_params = 0x00; /* bit0: first_request, bit1: handler from notification */
static uint64_t previous_request_time;
static inline void clear_first_request();
static inline uint8_t is_first_request();
static inline void clear_handler_from_notification();
static inline uint8_t get_handler_from_notification();
#endif /* LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION */
/*---------------------------------------------------------------------------*/
uint16_t
lwm2m_queue_mode_get_awake_time()
{
  LOG_DBG("Client Awake Time: %d ms\n", (int)queue_mode_awake_time);
  return queue_mode_awake_time;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_queue_mode_set_awake_time(uint16_t time)
{
  queue_mode_awake_time = time;
}
/*---------------------------------------------------------------------------*/
uint32_t
lwm2m_queue_mode_get_sleep_time()
{
  LOG_DBG("Client Sleep Time: %d ms\n", (int)queue_mode_sleep_time);
  return queue_mode_sleep_time;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_queue_mode_set_sleep_time(uint32_t time)
{
  queue_mode_sleep_time = time;
}
/*---------------------------------------------------------------------------*/
#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
uint8_t
lwm2m_queue_mode_get_dynamic_adaptation_flag()
{
  LOG_DBG("Dynamic Adaptation Flag: %d ms\n", (int)queue_mode_dynamic_adaptation_flag);
  return queue_mode_dynamic_adaptation_flag;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_queue_mode_set_dynamic_adaptation_flag(uint8_t flag)
{
  queue_mode_dynamic_adaptation_flag = flag;
}
#endif
/*---------------------------------------------------------------------------*/
#if !UPDATE_WITH_MEAN
static uint16_t
get_maximum_time()
{
  uint16_t max_time = 0;
  uint8_t i;
  for(i = 0; i < LWM2M_QUEUE_MODE_DYNAMIC_ADAPTATION_WINDOW_LENGTH; i++) {
    if(times_window[i] > max_time) {
      max_time = times_window[i];
    }
  }
  return max_time;
}
#endif
/*---------------------------------------------------------------------------*/
#if UPDATE_WITH_MEAN
static uint16_t
get_mean_time()
{
  uint16_t mean_time = 0;
  uint8_t i;
  for(i = 0; i < LWM2M_QUEUE_MODE_DYNAMIC_ADAPTATION_WINDOW_LENGTH; i++) {
    if(mean_time == 0) {
      mean_time = times_window[i];
    } else {
      if(times_window[i] != 0) {
        mean_time = (mean_time + times_window[i]) / 2;
      }
    }
  }
  return mean_time;
}
#endif
/*---------------------------------------------------------------------------*/
static void
update_awake_time()
{
#if UPDATE_WITH_MEAN
  uint16_t mean_time = get_mean_time();
  LOG_DBG("Dynamic Adaptation: updated awake time: %d ms\n", (int)mean_time);
  lwm2m_queue_mode_set_awake_time(mean_time + (mean_time >> 1)); /* 50% margin */
  return;
#else
  uint16_t max_time = get_maximum_time();
  LOG_DBG("Dynamic Adaptation: updated awake time: %d ms\n", (int)max_time);
  lwm2m_queue_mode_set_awake_time(max_time + (max_time >> 1)); /* 50% margin */
  return;
#endif
}
/*---------------------------------------------------------------------------*/
void
lwm2m_queue_mode_add_time_to_window(uint16_t time)
{
  if(times_window_index == LWM2M_QUEUE_MODE_DYNAMIC_ADAPTATION_WINDOW_LENGTH) {
    times_window_index = 0;
  }
  times_window[times_window_index] = time;
  times_window_index++;
  update_awake_time();
}
/*---------------------------------------------------------------------------*/
uint8_t
lwm2m_queue_mode_is_waked_up_by_notification()
{
  return waked_up_by_notification;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_queue_mode_clear_waked_up_by_notification()
{
  waked_up_by_notification = 0;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_queue_mode_set_waked_up_by_notification()
{
  waked_up_by_notification = 1;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_queue_mode_request_received()
{
  if(lwm2m_rd_client_is_client_awake()) {
    lwm2m_rd_client_restart_client_awake_timer();
  }
#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
  if(lwm2m_queue_mode_get_dynamic_adaptation_flag() && !get_handler_from_notification()) {
    if(is_first_request()) {
      previous_request_time = coap_timer_uptime();
      clear_first_request();
    } else {
      if(coap_timer_uptime() - previous_request_time >= 0) {
        if(coap_timer_uptime() - previous_request_time > 0xffff) {
          lwm2m_queue_mode_add_time_to_window(0xffff);
        } else {
          lwm2m_queue_mode_add_time_to_window(coap_timer_uptime() - previous_request_time);
        }
      }
      previous_request_time = coap_timer_uptime();
    }
  }
  if(get_handler_from_notification()) {
    clear_handler_from_notification();
  }
#endif /* LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION */
}
/*---------------------------------------------------------------------------*/
#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
void
lwm2m_queue_mode_set_first_request()
{
  dynamic_adaptation_params |= FIRST_REQUEST_MASK;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_queue_mode_set_handler_from_notification()
{
  dynamic_adaptation_params |= HANDLER_FROM_NOTIFICATION_MASK;
}
/*---------------------------------------------------------------------------*/
static inline uint8_t
is_first_request()
{
  return dynamic_adaptation_params & FIRST_REQUEST_MASK;
}
/*---------------------------------------------------------------------------*/
static inline uint8_t
get_handler_from_notification()
{
  return (dynamic_adaptation_params & HANDLER_FROM_NOTIFICATION_MASK) != 0;
}
/*---------------------------------------------------------------------------*/
static inline void
clear_first_request()
{
  dynamic_adaptation_params &= ~FIRST_REQUEST_MASK;
}
/*---------------------------------------------------------------------------*/
static inline void
clear_handler_from_notification()
{
  dynamic_adaptation_params &= ~HANDLER_FROM_NOTIFICATION_MASK;
}
#endif /* LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION */
#endif /* LWM2M_QUEUE_MODE_ENABLED */
/** @} */

