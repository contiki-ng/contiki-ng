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
 *         Implementation of functions to manage the queue of notifications
 * \author
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */
/*---------------------------------------------------------------------------*/
#include "lwm2m-notification-queue.h"

#if LWM2M_Q_MODE_ENABLED

#include "lwm2m-qmode-object.h"
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
#define LWM2M_NOTIFICATION_QUEUE_LENGTH 3
#endif

/*---------------------------------------------------------------------------*/
/* Queue to store the notifications in the period when the client has woken up, sent the update and it's waiting for the server response*/
MEMB(notification_memb, notification_path_t, LWM2M_NOTIFICATION_QUEUE_LENGTH + 1); /* Length + 1 to allocate the new path to add */
LIST(notification_paths_queue);
/*---------------------------------------------------------------------------*/
void
lwm2m_notification_queue_init(void)
{
  list_init(notification_paths_queue);
}
/*---------------------------------------------------------------------------*/
static void
reduce_path(notification_path_t *path_object, char *path)
{
  char *cut = strtok(path, "/");
  int i;
  for(i = 0; i < 3; i++) {
    if(cut != NULL) {
      path_object->reduced_path[i] = (uint16_t)atoi(cut);
      cut = strtok(NULL, "/");
    } else {
      break;
    }
  }
  path_object->level = i;
}
/*---------------------------------------------------------------------------*/
static void
extend_path(notification_path_t *path_object, char *path)
{
  switch(path_object->level) {
  case 1:
    snprintf(path, sizeof(path) - 1, "%u", path_object->reduced_path[0]);
    break;
  case 2:
    snprintf(path, sizeof(path) - 1, "%u/%u", path_object->reduced_path[0], path_object->reduced_path[1]);
    break;
  case 3:
    snprintf(path, sizeof(path) - 1, "%u/%u/%u", path_object->reduced_path[0], path_object->reduced_path[1], path_object->reduced_path[2]);
    break;
  }
}
/*---------------------------------------------------------------------------*/
static void
add_notification_path_object_ordered(notification_path_t *path)
{
  notification_path_t *iteration_path = (notification_path_t *)list_head(notification_paths_queue);
  if(list_length(notification_paths_queue) == 0) {
    list_add(notification_paths_queue, path);
  } else if(path->level < iteration_path->level) {
    list_push(notification_paths_queue, path);
  } else if(memcmp((path->reduced_path), (iteration_path->reduced_path), (path->level) * sizeof(uint16_t)) <= 0) {
    list_push(notification_paths_queue, path);
  } else {
    notification_path_t *previous_path = iteration_path;
    while(iteration_path != NULL) {
      if(path->level < iteration_path->level) {
        path->next = iteration_path;
        previous_path->next = path;
        return;
      }
      if(memcmp((path->reduced_path), (iteration_path->reduced_path), (path->level) * sizeof(uint16_t)) <= 0) {
        path->next = iteration_path;
        previous_path->next = path;
        return;
      }
      previous_path = iteration_path;
      iteration_path = iteration_path->next;
    }
    list_add(notification_paths_queue, path);
  }
}
/*---------------------------------------------------------------------------*/
static void
remove_notification_path(notification_path_t *path)
{
  list_remove(notification_paths_queue, path);
  memb_free(&notification_memb, path);
}
/*---------------------------------------------------------------------------*/
static void
notification_queue_remove_policy(uint16_t *reduced_path, uint8_t level)
{
  uint8_t path_removed_flag = 0;

  notification_path_t *path_object = NULL;
  notification_path_t *iteration_path = NULL;
  notification_path_t *previous = NULL;
  notification_path_t *next_next = NULL;
  notification_path_t *path_to_remove = NULL;

  for(iteration_path = (notification_path_t *)list_head(notification_paths_queue); iteration_path != NULL;
      iteration_path = iteration_path->next) {
    /* 1. check if there is one event of the same path -> remove it and add the new one */
    if((level == iteration_path->level) && memcmp(iteration_path->reduced_path, reduced_path, level * sizeof(uint16_t)) == 0) {
      remove_notification_path(iteration_path);
      path_object = memb_alloc(&notification_memb);
      memcpy(path_object->reduced_path, reduced_path, level * sizeof(uint16_t));
      path_object->level = level;
      add_notification_path_object_ordered(path_object);
      return;
    }
    /* 2. If there is no event of the same type, look for repeated events of the same resource and remove the oldest one */
    if(iteration_path->next != NULL && (iteration_path->level == iteration_path->next->level)
       && (memcmp(iteration_path->reduced_path, (iteration_path->next)->reduced_path, iteration_path->level * sizeof(uint16_t)) == 0)) {
      path_removed_flag = 1;
      next_next = iteration_path->next->next;
      path_to_remove = iteration_path->next;
      previous = iteration_path;
    }
  }
  /* 3. If there are no events for the same path, we remove a the oldest repeated event of another resource */
  if(path_removed_flag) {
    memb_free(&notification_memb, path_to_remove);
    previous->next = next_next;
    path_object = memb_alloc(&notification_memb);
    memcpy(path_object->reduced_path, reduced_path, level * sizeof(uint16_t));
    path_object->level = level;
    add_notification_path_object_ordered(path_object);
  } else {
    /* 4. If all the events are from different resources, remove the last one */
    list_chop(notification_paths_queue);
    path_object = memb_alloc(&notification_memb);
    memcpy(path_object->reduced_path, reduced_path, level * sizeof(uint16_t));
    path_object->level = level;
    add_notification_path_object_ordered(path_object);
  }
  return;
}
/*---------------------------------------------------------------------------*/
/* For adding objects to the list in an ordered way, depending on the path*/
void
lwm2m_notification_queue_add_notification_path(char *path)
{
  notification_path_t *path_object = memb_alloc(&notification_memb);
  if(path_object == NULL) {
    LOG_DBG("Could not allocate new notification in the queue\n");
    return;
  }
  reduce_path(path_object, path);
  if(list_length(notification_paths_queue) >= LWM2M_NOTIFICATION_QUEUE_LENGTH) {
    /* The queue is full, apply policy to remove */
    notification_queue_remove_policy(path_object->reduced_path, path_object->level);
  } else {
    add_notification_path_object_ordered(path_object);
  }
  LOG_DBG("Notification path added to the list: %s\n", path);
}
/*---------------------------------------------------------------------------*/
void
lwm2m_notification_queue_send_notifications()
{
  char path[20];
  notification_path_t *iteration_path = (notification_path_t *)list_head(notification_paths_queue);
  notification_path_t *aux = iteration_path;

  while(iteration_path != NULL) {
    extend_path(iteration_path, path);
#if LWM2M_Q_MODE_INCLUDE_DYNAMIC_ADAPTATION
    if(lwm2m_q_object_get_dynamic_adaptation_flag()) {
      lwm2m_engine_set_handler_from_notification();
    }
#endif
    LOG_DBG("Sending stored notification with path: %s\n", path);
    coap_notify_observers_sub(NULL, path);
    aux = iteration_path;
    iteration_path = iteration_path->next;
    remove_notification_path(aux);
  }
}
#endif /* LWM2M_Q_MODE_ENABLED */
/** @} */