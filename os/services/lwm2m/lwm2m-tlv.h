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

/** \addtogroup lwm2m
 * @{ */

/**
 * \file
 *         Header file for the Contiki OMA LWM2M TLV
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef LWM2M_TLV_H_
#define LWM2M_TLV_H_

#include <stdint.h>
#include <stddef.h>

enum {
  LWM2M_TLV_TYPE_OBJECT_INSTANCE   = 0,
  LWM2M_TLV_TYPE_RESOURCE_INSTANCE = 1,
  LWM2M_TLV_TYPE_MULTI_RESOURCE    = 2,
  LWM2M_TLV_TYPE_RESOURCE          = 3
};
typedef uint8_t lwm2m_tlv_type_t;

typedef enum {
  LWM2M_TLV_LEN_TYPE_NO_LEN    = 0,
  LWM2M_TLV_LEN_TYPE_8BIT_LEN  = 1,
  LWM2M_TLV_LEN_TYPE_16BIT_LEN = 2,
  LWM2M_TLV_LEN_TYPE_24BIT_LEN = 3
} lwm2m_tlv_len_type_t;

typedef struct {
  lwm2m_tlv_type_t type;
  uint16_t id; /* can be 8-bit or 16-bit when serialized */
  uint32_t length;
  const uint8_t *value;
} lwm2m_tlv_t;

size_t lwm2m_tlv_get_size(const lwm2m_tlv_t *tlv);

/* read a TLV from the buffer */
size_t lwm2m_tlv_read(lwm2m_tlv_t *tlv, const uint8_t *buffer, size_t len);

/* write a TLV to the buffer */
size_t lwm2m_tlv_write(const lwm2m_tlv_t *tlv, uint8_t *buffer, size_t len);

int32_t lwm2m_tlv_get_int32(const lwm2m_tlv_t *tlv);

/* write a int as a TLV to the buffer */
size_t lwm2m_tlv_write_int32(uint8_t type, int16_t id, int32_t value, uint8_t *buffer, size_t len);

/* write a float converted from fixpoint as a TLV to the buffer */
size_t lwm2m_tlv_write_float32(uint8_t type, int16_t id, int32_t value, int bits, uint8_t *buffer, size_t len);

/* convert TLV with float32 to fixpoint */
size_t lwm2m_tlv_float32_to_fix(const lwm2m_tlv_t *tlv, int32_t *value, int bits);

#endif /* LWM2M_TLV_H_ */
/** @} */
