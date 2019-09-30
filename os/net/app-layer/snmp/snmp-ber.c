/*
 * Copyright (C) 2019 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/

/**
 * \file
 *      An implementation of the Simple Network Management Protocol (RFC 3411-3418)
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

#include "contiki.h"

#include "snmp.h"
#include "snmp-ber.h"

#define LOG_MODULE "SNMP [ber]"
#define LOG_LEVEL LOG_LEVEL_SNMP

/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_encode_type(unsigned char *out, uint32_t *out_len, uint8_t type)
{
  *out-- = type;
  (*out_len)++;
  return out;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_encode_length(unsigned char *out, uint32_t *out_len, uint8_t length)
{
  *out-- = length;
  (*out_len)++;
  return out;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_encode_integer(unsigned char *out, uint32_t *out_len, uint32_t number)
{
  uint32_t original_out_len;

  original_out_len = *out_len;
  do {
    (*out_len)++;
    *out-- = (uint8_t)(number & 0xFF);
    number >>= 8;
  } while(number);

  out = snmp_ber_encode_length(out, out_len, ((*out_len - original_out_len) & 0xFF));
  out = snmp_ber_encode_type(out, out_len, BER_DATA_TYPE_INTEGER);

  return out;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_encode_unsigned_integer(unsigned char *out, uint32_t *out_len, uint8_t type, uint32_t number)
{
  uint32_t original_out_len;

  original_out_len = *out_len;
  do {
    (*out_len)++;
    *out-- = (uint8_t)(number & 0xFF);
    number >>= 8;
  } while(number);

  out = snmp_ber_encode_length(out, out_len, ((*out_len - original_out_len) & 0xFF));
  out = snmp_ber_encode_type(out, out_len, type);

  return out;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_encode_string_len(unsigned char *out, uint32_t *out_len, const char *str, uint32_t length)
{
  uint32_t i;

  str += length - 1;
  for(i = 0; i < length; ++i) {
    (*out_len)++;
    *out-- = (uint8_t)*str--;
  }

  out = snmp_ber_encode_length(out, out_len, length);
  out = snmp_ber_encode_type(out, out_len, BER_DATA_TYPE_OCTET_STRING);

  return out;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_encode_null(unsigned char *out, uint32_t *out_len, uint8_t type)
{
  (*out_len)++;
  *out-- = 0x00;
  out = snmp_ber_encode_type(out, out_len, type);

  return out;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_decode_type(unsigned char *buff, uint32_t *buff_len, uint8_t *type)
{
  *type = *buff++;
  (*buff_len)--;

  return buff;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_decode_length(unsigned char *buff, uint32_t *buff_len, uint8_t *length)
{
  *length = *buff++;
  (*buff_len)--;

  return buff;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_decode_integer(unsigned char *buf, uint32_t *buff_len, uint32_t *num)
{
  uint8_t i, len, type;

  buf = snmp_ber_decode_type(buf, buff_len, &type);

  if(type != BER_DATA_TYPE_INTEGER) {
    /*
     * Sanity check
     * Invalid type in buffer
     */
    return NULL;
  }

  buf = snmp_ber_decode_length(buf, buff_len, &len);

  if(len > 4) {
    /*
     * Sanity check
     * It will not fit in the uint32_t
     */
    return NULL;
  }

  *num = (uint32_t)(*buf++ & 0xFF);
  (*buff_len)--;
  for(i = 1; i < len; ++i) {
    *num <<= 8;
    *num |= (uint8_t)(*buf++ & 0xFF);
    (*buff_len)--;
  }

  return buf;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_decode_unsigned_integer(unsigned char *buf, uint32_t *buff_len, uint8_t expected_type, uint32_t *num)
{
  uint8_t i, len, type;

  buf = snmp_ber_decode_type(buf, buff_len, &type);

  if(type != expected_type) {
    /*
     * Sanity check
     * Invalid type in buffer
     */
    return NULL;
  }

  buf = snmp_ber_decode_length(buf, buff_len, &len);

  if(len > 4) {
    /*
     * Sanity check
     * It will not fit in the uint32_t
     */
    return NULL;
  }

  *num = (uint32_t)(*buf++ & 0xFF);
  (*buff_len)--;
  for(i = 1; i < len; ++i) {
    *num <<= 8;
    *num |= (uint8_t)(*buf++ & 0xFF);
    (*buff_len)--;
  }

  return buf;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_decode_string_len_buffer(unsigned char *buf, uint32_t *buff_len, const char **str, uint32_t *length)
{
  uint8_t type, i, length_bytes;

  buf = snmp_ber_decode_type(buf, buff_len, &type);

  if(type != BER_DATA_TYPE_OCTET_STRING) {
    /*
     * Sanity check
     * Invalid type in buffer
     */
    return NULL;
  }

  if((*buf & 0x80) == 0) {
    *length = (uint32_t)*buf++;
    (*buff_len)--;
  } else {

    length_bytes = (uint8_t)(*buf++ & 0x7F);
    (*buff_len)--;
    if(length_bytes > 4) {
      /*
       * Sanity check
       * It will not fit in the uint32_t
       */
      return NULL;
    }

    *length = (uint32_t)*buf++;
    (*buff_len)--;
    for(i = 1; i < length_bytes; ++i) {
      *length <<= 8;
      *length |= *buf++;
      (*buff_len)--;
    }
  }

  *str = (const char *)buf;
  *buff_len -= *length;

  return buf + *length;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_ber_decode_null(unsigned char *buf, uint32_t *buff_len)
{
  buf++;
  (*buff_len)--;

  buf++;
  (*buff_len)--;

  return buf;
}
/*---------------------------------------------------------------------------*/
