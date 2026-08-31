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
 *
 */

/**
 * \file
 *         Implementation of the Contiki OMA LWM2M TLV
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "lwm2m-tlv.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "lwm2m-tlv"
#define LOG_LEVEL  LOG_LEVEL_NONE

/*---------------------------------------------------------------------------*/
static inline uint8_t
get_len_type(const lwm2m_tlv_t *tlv)
{
  if(tlv->length < 8) {
    return 0;
  } else if(tlv->length < 256) {
    return 1;
  } else if(tlv->length < 0x10000) {
    return 2;
  } else {
    return 3;
  }
}
/*---------------------------------------------------------------------------*/
size_t
lwm2m_tlv_read(lwm2m_tlv_t *tlv, const uint8_t *buffer, size_t len)
{
  uint8_t len_type;
  uint8_t len_pos = 1;
  size_t tlv_len;

  /* A TLV needs at least the type byte and a single ID byte. */
  if(len < 2) {
    LOG_WARN("Truncated TLV header (%u bytes available).\n", (unsigned)len);
    return 0;
  }

  tlv->type = (buffer[0] >> 6) & 3;
  len_type = (buffer[0] >> 3) & 3;
  len_pos = 1 + (((buffer[0] & (1 << 5)) != 0) ? 2 : 1);

  /* len_pos is the offset of the length field (== header size when there
     is no separate length field). Ensure the ID bytes and any length
     bytes are within the buffer before reading them. */
  if(len < (size_t)len_pos + len_type) {
    LOG_WARN("Truncated TLV header (need %u bytes, have %u).\n",
             (unsigned)(len_pos + len_type), (unsigned)len);
    return 0;
  }

  tlv->id = buffer[1];
  /* if len_pos is larger than two it means that there is more ID to read */
  if(len_pos > 2) {
    tlv->id = (tlv->id << 8) + buffer[2];
  }

  if(len_type == 0) {
    tlv_len = buffer[0] & 7;
  } else {
    /* read the length */
    tlv_len = 0;
    while(len_type > 0) {
      tlv_len = tlv_len << 8 | buffer[len_pos++];
      len_type--;
    }
  }

  /* Ensure the value fits within the remaining buffer. len_pos now points
     to the first value byte and is guaranteed to be <= len. */
  if(tlv_len > len - len_pos) {
    LOG_WARN("Truncated TLV value (need %u bytes, have %u).\n",
             (unsigned)tlv_len, (unsigned)(len - len_pos));
    return 0;
  }

  /* and read out the data??? */
  tlv->length = tlv_len;
  tlv->value = &buffer[len_pos];

  return len_pos + tlv_len;
}
/*---------------------------------------------------------------------------*/
size_t
lwm2m_tlv_get_size(const lwm2m_tlv_t *tlv)
{
  size_t size;
  /* first hdr + len size */
  size = 1 + get_len_type(tlv);
  /* id size */
  size += (tlv->id > 255) ? 2 : 1;

  /* and the length */
  size += tlv->length;
  return size;
}
/*---------------------------------------------------------------------------*/
/* If the tlv->value is NULL - only the header will be generated - useful for
 * large strings or opaque streaming (block transfer)
 */
size_t
lwm2m_tlv_write(const lwm2m_tlv_t *tlv, uint8_t *buffer, size_t buffersize)
{
  int pos;
  uint8_t len_type;

  /* len type is the same as number of bytes required for length */
  len_type = get_len_type(tlv);
  pos = 1 + len_type;
  /* ensure that we do not write too much */
  if(tlv->value != NULL && buffersize < tlv->length + pos) {
    LOG_WARN("Could not write the TLV - buffer overflow.\n");
    return 0;
  }

  if(buffersize < pos + 2) {
    return 0;
  }

  /* first type byte in TLV header */
  buffer[0] = (tlv->type << 6) |
    (tlv->id > 255 ? (1 << 5) : 0) |
    (len_type << 3) |
    (len_type == 0 ? tlv->length : 0);

  pos = 1;
  /* The ID */
  if(tlv->id > 255) {
    buffer[pos++] = (tlv->id >> 8) & 0xff;
  }
  buffer[pos++] = tlv->id & 0xff;
  /* Add length if needed - unrolled loop ? */
  if(len_type > 2) {
    buffer[pos++] = (tlv->length >> 16) & 0xff;
  }
  if(len_type > 1) {
    buffer[pos++] = (tlv->length >> 8) & 0xff;
  }
  if(len_type > 0) {
    buffer[pos++] = tlv->length & 0xff;
  }

  /* finally add the value */
  if(tlv->value != NULL && tlv->length > 0) {
    memcpy(&buffer[pos], tlv->value, tlv->length);
  }

  if(LOG_DBG_ENABLED) {
    int i;
    LOG_DBG("TLV: ");
    for(i = 0; i < pos + ((tlv->value != NULL) ? tlv->length : 0); i++) {
      LOG_DBG_("%02x", buffer[i]);
    }
    LOG_DBG_("\n");
  }

  return pos + ((tlv->value != NULL) ? tlv->length : 0);
}
/*---------------------------------------------------------------------------*/
int32_t
lwm2m_tlv_get_int32(const lwm2m_tlv_t *tlv)
{
  int i;
  int32_t value = 0;

  /* A zero-length integer represents the value 0. Returning here also
     avoids reading tlv->value[0] below when there is no value at all. */
  if(tlv->length == 0) {
    return 0;
  }

  for(i = 0; i < tlv->length; i++) {
    value = (value << 8) | tlv->value[i];
  }

  /* Ensure that we set all the bits above what we read in */
  if(tlv->value[0] & 0x80) {
    while(i < 4) {
      value = value | (0xff << (i * 8));
      i++;
    }
  }

  return value;
}
/*---------------------------------------------------------------------------*/
size_t
lwm2m_tlv_write_int32(uint8_t type, int16_t id, int32_t value, uint8_t *buffer, size_t len)
{
  lwm2m_tlv_t tlv;
  uint8_t buf[4];
  int i;
  int v;
  int last_bit;
  LOG_DBG("Exporting int32 %d %"PRId32" ", id, value);

  v = value < 0 ? -1 : 0;
  i = 0;
  do {
    buf[3 - i] = value & 0xff;
    /* check if the last MSB indicates that we need another byte */
    last_bit = (v == 0 && (value & 0x80) > 0) || (v == -1 && (value & 0x80) == 0);
    value = value >> 8;
    i++;
  } while((value != v || last_bit) && i < 4);

  /* export INT as TLV */
  LOG_DBG("len: %d\n", i);
  tlv.type = type;
  tlv.length = i;
  tlv.value = &buf[3 - (i - 1)];
  tlv.id = id;
  return lwm2m_tlv_write(&tlv, buffer, len);
}
/*---------------------------------------------------------------------------*/
/* convert fixpoint 32-bit to a IEEE Float in the byte array*/
size_t
lwm2m_tlv_write_float32(uint8_t type, int16_t id, int32_t value, int bits,
                      uint8_t *buffer, size_t len)
{
  int i;
  int e = 0;
  int32_t val = 0;
  int32_t v;
  uint8_t b[4];
  lwm2m_tlv_t tlv;

  v = value;
  if(v < 0) {
    v = -v;
  }

  while(v > 1) {
    val = (val >> 1);
    if (v & 1) {
      val = val | (1L << 22);
    }
    v = (v >> 1);
    e++;
  }

  LOG_DBG("Sign: %d, Fraction: %06"PRIx32"  0b", value < 0, val);
  for(i = 0; i < 23; i++) {
    LOG_DBG_("%"PRId32"", ((val >> (22 - i)) & 1));
  }
  LOG_DBG_("\nExp:%d\n", e);

  /* convert to the thing we should have */
  e = e - bits + 127;

  if(value == 0) {
    e = 0;
  }

  /* is this the right byte order? */
  b[0] = (value < 0 ? 0x80 : 0) | (e >> 1);
  b[1] = ((e & 1) << 7) | ((val >> 16) & 0x7f);
  b[2] = (val >> 8) & 0xff;
  b[3] = val & 0xff;

  LOG_DBG("B=%02x%02x%02x%02x\n", b[0], b[1], b[2], b[3]);
  /* construct the TLV */
  tlv.type = type;
  tlv.length = 4;
  tlv.value = b;
  tlv.id = id;

  return lwm2m_tlv_write(&tlv, buffer, len);
}
/*---------------------------------------------------------------------------*/
/* convert float to fixpoint */
/* Both widths normalise their significand to a leading bit at this position,
   which is the widest that keeps the scaled result inside int32_t. */
#define SIGNIFICAND_BIT 30
/*---------------------------------------------------------------------------*/
/*
 * Scale a significand held with its leading bit at SIGNIFICAND_BIT by 2^e and
 * apply the sign. Both widths normalise to this form, so the saturating shift
 * below is written once.
 */
static void
significand_to_fix(int sign, int e, uint32_t val, int32_t *value)
{
  /*
   * e is derived from an exponent taken off the wire, so neither shift can be
   * taken at face value: a shift count of 32 or more is undefined, and even a
   * small left shift can push the significand out of the range of int32_t.
   * Saturate at both ends instead.
   */
  if(e >= 32) {
    val = INT32_MAX;
  } else if(e > 0) {
    if(val > ((uint32_t)INT32_MAX >> e)) {
      val = INT32_MAX;
    } else {
      val <<= e;
    }
  } else if(e <= -32) {
    /* Every significant bit would have been shifted out. */
    val = 0;
  } else {
    val >>= -e;
  }

  *value = sign ? -(int32_t)val : (int32_t)val;
}
/*---------------------------------------------------------------------------*/
/* Convert an IEEE 754 binary32 value, 8 bits of exponent and 23 of
   significand, to the fixpoint representation. */
static size_t
binary32_to_fix(const uint8_t *v, int32_t *value, int bits)
{
  int sign = (v[0] & 0x80) != 0;
  int e = ((v[0] << 1) & 0xff) | (v[1] >> 7);
  /* 23 significand bits, left-aligned to SIGNIFICAND_BIT. */
  uint32_t val = ((((uint32_t)v[1] & 0x7f) << 16) | (v[2] << 8) | v[3])
    << (SIGNIFICAND_BIT - 23);

  LOG_DBG("binary32 sign %d exp %d significand %06"PRIx32"\n", sign, e,
          (uint32_t)val);

  if(e == 0xff) {
    /* Infinity and NaN have no fixpoint representation; reject rather than
       hand the caller a number it cannot tell apart from a measurement. */
    *value = 0;
    return 0;
  }

  if(e == 0) {
    /* Zero and the subnormals, which carry no implicit significand bit. Every
       subnormal is far below the resolution of the fixpoint format. */
    *value = 0;
    return 4;
  }

  significand_to_fix(sign, e - 127 + bits - SIGNIFICAND_BIT,
                     val | (UINT32_C(1) << SIGNIFICAND_BIT), value);
  return 4;
}
/*---------------------------------------------------------------------------*/
/*
 * Convert an IEEE 754 binary64 value, 11 bits of exponent and 52 of
 * significand, to the fixpoint representation. Only the top 30 significand
 * bits are read. That is not an approximation: the result is an int32_t and
 * so holds at most 31 significant bits, and truncating the bits that the
 * scaling below would discard anyway gives the same answer as carrying all
 * 52 in a 64-bit intermediate, which no target then has to pay for.
 */
static size_t
binary64_to_fix(const uint8_t *v, int32_t *value, int bits)
{
  int sign = (v[0] & 0x80) != 0;
  int e = (((int)v[0] & 0x7f) << 4) | (v[1] >> 4);
  /* Top 30 significand bits, left-aligned to SIGNIFICAND_BIT. */
  uint32_t val = (((uint32_t)v[1] & 0x0f) << 26) | ((uint32_t)v[2] << 18)
    | ((uint32_t)v[3] << 10) | ((uint32_t)v[4] << 2) | (v[5] >> 6);

  LOG_DBG("binary64 sign %d exp %d significand %06"PRIx32"\n", sign, e,
          (uint32_t)val);

  if(e == 0x7ff) {
    *value = 0;
    return 0;
  }

  if(e == 0) {
    *value = 0;
    return 8;
  }

  significand_to_fix(sign, e - 1023 + bits - SIGNIFICAND_BIT,
                     val | (UINT32_C(1) << SIGNIFICAND_BIT), value);
  return 8;
}
/*---------------------------------------------------------------------------*/
size_t
lwm2m_tlv_float32_to_fix(const lwm2m_tlv_t *tlv, int32_t *value, int bits)
{
  /*
   * A Float resource is carried as either binary32 or binary64, whichever the
   * sender chose, and the length field says which (Appendix C of the LwM2M
   * specification). Any other length is malformed.
   */
  if(tlv->length == 4) {
    return binary32_to_fix(tlv->value, value, bits);
  }

  if(tlv->length == 8) {
    return binary64_to_fix(tlv->value, value, bits);
  }

  *value = 0;
  return 0;
}
/*---------------------------------------------------------------------------*/
/** @} */

#if 0
int main(int argc, char *argv[])
{
  lwm2m_tlv_t tlv;
  uint8_t data[24];
  /* Make -1 */
  tlv.length = 2;
  tlv.value = data;
  data[0] = 0x00;
  data[1] = 0x80,

  printf("TLV:%d\n", lwm2m_tlv_get_int32(&tlv));

  printf("Len: %d\n", lwm2m_tlv_write_int32(0, 1, -0x88987f, data, 24));
}
#endif
