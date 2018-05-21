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
 *         Header file for functions to manage the queue to store notifications
       when waiting for the response to the update message in Queue Mode.
 * \author
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */

#ifndef LWM2M_NOTIFICATION_QUEUE_H
#define LWM2M_NOTIFICATION_QUEUE_H

#include "contiki.h"
#include "lwm2m-queue-mode-conf.h"

#include <inttypes.h>

typedef struct notification_path {
  struct notification_path *next;
  uint16_t reduced_path[3];
  uint8_t level; /* The depth level of the path: 1. object, 2. object/instance, 3. object/instance/resource */
} notification_path_t;

void lwm2m_notification_queue_init(void);

void lwm2m_notification_queue_add_notification_path(uint16_t object_id, uint16_t instance_id, uint16_t resource_id);

void lwm2m_notification_queue_send_notifications();

#endif /* LWM2M_NOTIFICATION_QUEUE_H */
/** @} */