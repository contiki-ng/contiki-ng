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
#ifndef LWM2M_SECURITY_H
#define LWM2M_SECURITY_H

#define URI_SIZE 64
#define KEY_SIZE 32

typedef struct {
  lwm2m_object_instance_t instance;
  uint16_t server_id;
  uint8_t bootstrap;
  uint8_t security_mode;
  uint8_t server_uri[URI_SIZE];
  uint8_t server_uri_len;
  uint8_t public_key[KEY_SIZE];
  uint8_t public_key_len;
  uint8_t secret_key[KEY_SIZE];
  uint8_t secret_key_len;
  uint8_t server_public_key[KEY_SIZE];
  uint8_t server_public_key_len;
} lwm2m_security_server_t;

lwm2m_security_server_t *lwm2m_security_get_first(void);
lwm2m_security_server_t *lwm2m_security_get_next(lwm2m_security_server_t *last);

lwm2m_security_server_t *lwm2m_security_add_server(uint16_t instance_id,
                                                  uint16_t server_id,
                                                  const uint8_t *server_uri,
                                                  uint8_t server_uri_len);

int lwm2m_security_set_server_psk(lwm2m_security_server_t *server,
                                  const uint8_t *identity,
                                  uint8_t identity_len,
                                  const uint8_t *key,
                                  uint8_t key_len);

void lwm2m_security_init(void);

#endif /* LWM2M_SECURITY_H */
/** @} */
