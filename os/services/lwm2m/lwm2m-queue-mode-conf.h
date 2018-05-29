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
 *         Queue Mode Configuration Parameters
 * \author
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */

#ifndef LWM2M_QUEUE_MODE_CONF_H
#define LWM2M_QUEUE_MODE_CONF_H

#include "contiki.h"

/* Enable the Queue Mode */
#ifdef LWM2M_QUEUE_MODE_CONF_ENABLED
#define LWM2M_QUEUE_MODE_ENABLED LWM2M_QUEUE_MODE_CONF_ENABLED
#else
#define LWM2M_QUEUE_MODE_ENABLED 0
#endif /* LWM2M_QUEUE_MODE_CONF_ENABLED */

/* Default Sleeping Time */
#ifdef LWM2M_QUEUE_MODE_CONF_DEFAULT_CLIENT_SLEEP_TIME
#define LWM2M_QUEUE_MODE_DEFAULT_CLIENT_SLEEP_TIME LWM2M_QUEUE_MODE_CONF_DEFAULT_CLIENT_SLEEP_TIME
#else
#define LWM2M_QUEUE_MODE_DEFAULT_CLIENT_SLEEP_TIME 10000 /* msec */
#endif /* LWM2M_QUEUE_MODE_DEFAULT_CLIENT_SLEEPING_TIME */

/* Default Awake Time */
#ifdef LWM2M_QUEUE_MODE_CONF_DEFAULT_CLIENT_AWAKE_TIME
#define LWM2M_QUEUE_MODE_DEFAULT_CLIENT_AWAKE_TIME LWM2M_QUEUE_MODE_CONF_DEFAULT_CLIENT_AWAKE_TIME
#else
#define LWM2M_QUEUE_MODE_DEFAULT_CLIENT_AWAKE_TIME 5000 /* msec */
#endif /* LWM2M_QUEUE_MODE_DEFAULT_CLIENT_AWAKE_TIME */

/* Include the possibility to do the dynamic adaptation of the client awake time */
#ifdef LWM2M_QUEUE_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION
#define LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION LWM2M_QUEUE_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION
#else
#define LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION 0 /* not included */
#endif /* LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION */

/* Default value for the dynamic adaptation flag */
#ifdef LWM2M_QUEUE_MODE_CONF_DEFAULT_DYNAMIC_ADAPTATION_FLAG
#define LWM2M_QUEUE_MODE_DEFAULT_DYNAMIC_ADAPTATION_FLAG LWM2M_QUEUE_MODE_CONF_DEFAULT_DYNAMIC_ADAPTATION_FLAG
#else
#define LWM2M_QUEUE_MODE_DEFAULT_DYNAMIC_ADAPTATION_FLAG 0 /* disabled */
#endif /* LWM2M_QUEUE_MODE_DEFAULT_DYNAMIC_ADAPTATION_FLAG */

/* Length of the list of times for the dynamic adaptation */
#define LWM2M_QUEUE_MODE_DYNAMIC_ADAPTATION_WINDOW_LENGTH 10

/* Enable and disable the Queue Mode Object */
#ifdef LWM2M_QUEUE_MODE_OBJECT_CONF_ENABLED
#define LWM2M_QUEUE_MODE_OBJECT_ENABLED LWM2M_QUEUE_MODE_OBJECT_CONF_ENABLED
#else
#define LWM2M_QUEUE_MODE_OBJECT_ENABLED 0 /* not included */
#endif /* LWM2M_QUEUE_MODE_OBJECT_ENABLED */



#endif /* LWM2M_QUEUE_MODE_CONF_H */
/** @} */
