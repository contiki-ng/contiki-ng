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
 *         Implementation of functions to manage the queue to store notifications
           when waiting for the response to the update message in Queue Mode.
 * \author
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */
/*---------------------------------------------------------------------------*/
#include "lwm2m-notification-queue.h"

#if LWM2M_QUEUE_MODE_ENABLED

#include "lwm2m-queue-mode.h"
#include "lwm2m-engine.h"
#include "coap-engine.h"
#include "lib/memb.h"
#include "lib/list.h"
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "lwm2m-notification-queue"
#define LOG_LEVEL  LOG_LEVEL_LWM2M

#ifdef LWM2M_NOTIFICATION_QUEUE_CONF_LENGTH
#define LWM2M_NOTIFICATION_QUEUE_LENGTH LWM2M_NOTIFICATION_QUEUE_CONF_LENGTH
#else
#define LWM2M_NOTIFICATION_QUEUE_LENGTH COAP_MAX_OBSERVERS
#endif

/*---------------------------------------------------------------------------*/
/* Queue to store the notifications in the period when the client has woken up, sent the update and it's waiting for the server response*/
MEMB(notification_memb, notification_path_t, LWM2M_NOTIFICATION_QUEUE_LENGTH); /* Length + 1 to allocate the new path to add */
LIST(notification_paths_queue);
/*---------------------------------------------------------------------------*/
void
lwm2m_notification_queue_init(void)
{
  list_init(notification_paths_queue);
}
/*---------------------------------------------------------------------------*/
static void
extend_path(notification_path_t *path_object, char *path, int path_size)
{
  switch(path_object->level) {
  case 1:
    snprintf(path, path_size, "%u", path_object->reduced_path[0]);
    break;
  case 2:
    snprintf(path, path_size, "%u/%u", path_object->reduced_path[0], path_object->reduced_path[1]);
    break;
  case 3:
    snprintf(path, path_size, "%u/%u/%u", path_object->reduced_path[0], path_object->reduced_path[1], path_object->reduced_path[2]);
    break;
  }
}
/*---------------------------------------------------------------------------*/
static int
is_notification_path_present(uint16_t object_id, uint16_t instance_id, uint16_t resource_id)
{
  notification_path_t *iteration_path = (notification_path_t *)list_head(notification_paths_queue);
  while(iteration_path != NULL) {
    if(iteration_path->reduced_path[0] == object_id && iteration_path->reduced_path[1] == instance_id
       && iteration_path->reduced_path[2] == resource_id) {
      return 1;
    }
    iteration_path = iteration_path->next;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
remove_notification_path(notification_path_t *path)
{
  list_remove(notification_paths_queue, path);
  memb_free(&notification_memb, path);
}
/*---------------------------------------------------------------------------*/
void
lwm2m_notification_queue_add_notification_path(uint16_t object_id, uint16_t instance_id, uint16_t resource_id)
{
  if(is_notification_path_present(object_id, instance_id, resource_id)) {
    LOG_DBG("Notification path already present, not queueing it\n");
    return;
  }
  notification_path_t *path_object = memb_alloc(&notification_memb);
  if(path_object == NULL) {
    LOG_DBG("Queue is full, could not allocate new notification\n");
    return;
  }
  path_object->reduced_path[0] = object_id;
  path_object->reduced_path[1] = instance_id;
  path_object->reduced_path[2] = resource_id;
  path_object->level = 3;
  list_add(notification_paths_queue, path_object);
  LOG_DBG("Notification path added to the list: %u/%u/%u\n", object_id, instance_id, resource_id);
}
/*---------------------------------------------------------------------------*/
void
lwm2m_notification_queue_send_notifications()
{
  char path[20];
  notification_path_t *iteration_path = (notification_path_t *)list_head(notification_paths_queue);
  notification_path_t *aux = iteration_path;

  while(iteration_path != NULL) {
    extend_path(iteration_path, path, sizeof(path));
#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
    if(lwm2m_queue_mode_get_dynamic_adaptation_flag()) {
      lwm2m_queue_mode_set_handler_from_notification();
    }
#endif
    LOG_DBG("Sending stored notification with path: %s\n", path);
    coap_notify_observers_sub(NULL, path);
    aux = iteration_path;
    iteration_path = iteration_path->next;
    remove_notification_path(aux);
  }
}
#endif /* LWM2M_QUEUE_MODE_ENABLED */
/** @} */