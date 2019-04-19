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
 *         Header file for the Contiki OMA LWM2M Queue Mode implementation
       to manage the parameters
 * \author
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */

#ifndef LWM2M_QUEUE_MODE_H_
#define LWM2M_QUEUE_MODE_H_

#include "lwm2m-queue-mode-conf.h"
#include <inttypes.h>

uint16_t lwm2m_queue_mode_get_awake_time();
void lwm2m_queue_mode_set_awake_time(uint16_t time);
uint32_t lwm2m_queue_mode_get_sleep_time();
void lwm2m_queue_mode_set_sleep_time(uint32_t time);

#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
uint8_t lwm2m_queue_mode_get_dynamic_adaptation_flag();
void lwm2m_queue_mode_set_dynamic_adaptation_flag(uint8_t flag);
void lwm2m_queue_mode_add_time_to_window(uint16_t time);
#endif

uint8_t lwm2m_queue_mode_is_waked_up_by_notification();
void lwm2m_queue_mode_clear_waked_up_by_notification();
void lwm2m_queue_mode_set_waked_up_by_notification();

void lwm2m_queue_mode_set_first_request();
void lwm2m_queue_mode_set_handler_from_notification();

void lwm2m_queue_mode_request_received();

#endif /* LWM2M_QUEUE_MODE_H_ */
/** @} */
