/*
 * Copyright (c) 2017, SICS Swedish ICT
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
 *
 */
#ifndef LWM2M_SERVER_H
#define LWM2M_SERVER_H

#include "contiki.h"

#define LWM2M_SERVER_SHORT_SERVER_ID            0
#define LWM2M_SERVER_LIFETIME_ID                1
#define LWM2M_SERVER_BINDING_ID                 7
#define LWM2M_SERVER_REG_UPDATE_TRIGGER_ID      8

#ifdef LWM2M_SERVER_CONF_MAX_COUNT
#define LWM2M_SERVER_MAX_COUNT LWM2M_SERVER_CONF_MAX_COUNT
#else
#define LWM2M_SERVER_MAX_COUNT 2
#endif

typedef struct {
  lwm2m_object_instance_t instance;
  uint16_t server_id;
  uint32_t lifetime;
} lwm2m_server_t;

lwm2m_server_t *lwm2m_server_add(uint16_t instance_id,
                                 uint16_t server_id,
                                 uint32_t lifetime);

void lwm2m_server_init(void);

#endif /* LWM2M_SERVER_H */
/** @} */
