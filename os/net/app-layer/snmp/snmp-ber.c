/*
 * Copyright (C) 2019-2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 *      SNMP Implementation of the BER encoding
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

#include "contiki.h"

#include "snmp.h"
#include "snmp-ber.h"

#define LOG_MODULE "SNMP [ber]"
#define LOG_LEVEL LOG_LEVEL_SNMP

/*---------------------------------------------------------------------------*/
static inline int
snmp_ber_encode_unsigned_integer(snmp_packet_t *snmp_packet, uint8_t type, uint32_t number)
{
  uint16_t original_out_len;

  original_out_len = snmp_packet->used;
  do {
    if(snmp_packet->used == snmp_packet->max) {
      return 0;
    }

    *snmp_packet->out-- = (uint8_t)number & 0xFF;
    snmp_packet->used++;
    /* I'm not sure why but on MSPGCC the >> 8 operation goes haywire here */
#ifdef __MSPGCC__
    number >>= 4;
    number >>= 4;
#else /* __MSPGCC__ */
    number >>= 8;
#endif /* __MSPGCC__ */
  } while(number);

  if(!snmp_ber_encode_length(snmp_packet, snmp_packet->used - original_out_len)) {
    return 0;
  }

  if(!snmp_ber_encode_type(snmp_packet, type)) {
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_encode_type(snmp_packet_t *snmp_packet, uint8_t type)
{
  if(snmp_packet->used == snmp_packet->max) {
    return 0;
  }

  *snmp_packet->out-- = type;
  snmp_packet->used++;

  return 1;
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_encode_length(snmp_packet_t *snmp_packet, uint16_t length)
{
  if(length > 0xFF) {
    if(snmp_packet->used == snmp_packet->max) {
      return 0;
    }

    *snmp_packet->out-- = (uint8_t)length & 0xFF;
    snmp_packet->used++;

    if(snmp_packet->used == snmp_packet->max) {
      return 0;
    }

    *snmp_packet->out-- = (uint8_t)(length >> 8) & 0xFF;
    snmp_packet->used++;

    if(snmp_packet->used == snmp_packet->max) {
      return 0;
    }

    *snmp_packet->out-- = 0x82;
    snmp_packet->used++;
  } else if(length > 0x7F) {
    if(snmp_packet->used == snmp_packet->max) {
      return 0;
    }

    *snmp_packet->out-- = (uint8_t)length & 0xFF;
    snmp_packet->used++;

    if(snmp_packet->used == snmp_packet->max) {
      return 0;
    }

    *snmp_packet->out-- = 0x81;
    snmp_packet->used++;
  } else {
    if(snmp_packet->used == snmp_packet->max) {
      return 0;
    }

    *snmp_packet->out-- = (uint8_t)length & 0x7F;
    snmp_packet->used++;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_encode_timeticks(snmp_packet_t *snmp_packet, uint32_t timeticks)
{
  return snmp_ber_encode_unsigned_integer(snmp_packet, BER_DATA_TYPE_TIMETICKS, timeticks);
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_encode_integer(snmp_packet_t *snmp_packet, uint32_t number)
{
  return snmp_ber_encode_unsigned_integer(snmp_packet, BER_DATA_TYPE_INTEGER, number);
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_encode_string_len(snmp_packet_t *snmp_packet, const char *str, uint32_t length)
{
  uint32_t i;

  if(length > 0) {
    str += length - 1;
    for(i = 0; i < length; ++i) {
      if(snmp_packet->used == snmp_packet->max) {
        return 0;
      }

      *snmp_packet->out-- = (uint8_t)*str--;
      snmp_packet->used++;
    }
  }

  if(!snmp_ber_encode_length(snmp_packet, length)) {
    return 0;
  }

  if(!snmp_ber_encode_type(snmp_packet, BER_DATA_TYPE_OCTET_STRING)) {
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_encode_oid(snmp_packet_t *snmp_packet, snmp_oid_t *oid)
{
  uint32_t val;
  uint16_t original_out_len;
  uint8_t pos;

  original_out_len = snmp_packet->used;

  pos = oid->length - 1;
  while(pos) {
    val = oid->data[pos];

    if(snmp_packet->used == snmp_packet->max) {
      return 0;
    }

    *snmp_packet->out-- = (uint8_t)(val & 0x7F);
    snmp_packet->used++;
    val >>= 7;

    while(val) {
      if(snmp_packet->used == snmp_packet->max) {
        return 0;
      }

      *snmp_packet->out-- = (uint8_t)((val & 0x7F) | 0x80);
      snmp_packet->used++;

      val >>= 7;
    }
    pos--;
  }

  if(snmp_packet->used == snmp_packet->max) {
    return 0;
  }

  val = *(snmp_packet->out + 1) + 40 * oid->data[pos];
  snmp_packet->used--;
  snmp_packet->out++;

  if(snmp_packet->used == snmp_packet->max) {
    return 0;
  }

  *snmp_packet->out-- = (uint8_t)(val & 0x7F);
  snmp_packet->used++;

  val >>= 7;

  while(val) {
    if(snmp_packet->used == snmp_packet->max) {
      return 0;
    }

    *snmp_packet->out-- = (uint8_t)((val & 0x7F) | 0x80);
    snmp_packet->used++;

    val >>= 7;
  }

  if(!snmp_ber_encode_length(snmp_packet, snmp_packet->used - original_out_len)) {
    return 0;
  }

  if(!snmp_ber_encode_type(snmp_packet, BER_DATA_TYPE_OBJECT_IDENTIFIER)) {
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_encode_null(snmp_packet_t *snmp_packet, uint8_t type)
{

  if(snmp_packet->used == snmp_packet->max) {
    return 0;
  }

  *snmp_packet->out-- = 0x00;
  snmp_packet->used++;

  return snmp_ber_encode_type(snmp_packet, type);
}
/*---------------------------------------------------------------------------*/
static inline int
snmp_ber_decode_unsigned_integer(snmp_packet_t *snmp_packet, uint8_t expected_type, uint32_t *num)
{
  uint8_t i, len, type;

  if(!snmp_ber_decode_type(snmp_packet, &type)) {
    return 0;
  }

  if(type != expected_type) {
    /*
     * Sanity check
     * Invalid type in buffer
     */
    return 0;
  }

  if(!snmp_ber_decode_length(snmp_packet, &len)) {
    return 0;
  }

  if(len > 4) {
    /*
     * Sanity check
     * It will not fit in the uint32_t
     */
    return 0;
  }

  if(snmp_packet->used == 0) {
    return 0;
  }

  *num = (uint32_t)(*snmp_packet->in++ & 0xFF);
  snmp_packet->used--;

  for(i = 1; i < len; ++i) {
    *num <<= 8;
    if(snmp_packet->used == 0) {
      return 0;
    }
    *num |= (uint8_t)(*snmp_packet->in++ & 0xFF);
    snmp_packet->used--;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_decode_type(snmp_packet_t *snmp_packet, uint8_t *type)
{
  if(snmp_packet->used == 0) {
    return 0;
  }

  *type = *snmp_packet->in++;
  snmp_packet->used--;

  return 1;
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_decode_length(snmp_packet_t *snmp_packet, uint8_t *length)
{
  if(snmp_packet->used == 0) {
    return 0;
  }

  *length = *snmp_packet->in++;
  snmp_packet->used--;

  return 1;
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_decode_timeticks(snmp_packet_t *snmp_packet, uint32_t *timeticks)
{
  return snmp_ber_decode_unsigned_integer(snmp_packet, BER_DATA_TYPE_TIMETICKS, timeticks);
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_decode_integer(snmp_packet_t *snmp_packet, uint32_t *num)
{
  return snmp_ber_decode_unsigned_integer(snmp_packet, BER_DATA_TYPE_INTEGER, num);
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_decode_string_len_buffer(snmp_packet_t *snmp_packet, const char **str, uint32_t *length)
{
  uint8_t type, i, length_bytes;

  if(!snmp_ber_decode_type(snmp_packet, &type)) {
    return 0;
  }

  if(type != BER_DATA_TYPE_OCTET_STRING) {
    /*
     * Sanity check
     * Invalid type in buffer
     */
    return 0;
  }

  if((*snmp_packet->in & 0x80) == 0) {

    if(snmp_packet->used == 0) {
      return 0;
    }

    *length = (uint32_t)*snmp_packet->in++;
    snmp_packet->used--;
  } else {

    if(snmp_packet->used == 0) {
      return 0;
    }

    length_bytes = (uint8_t)(*snmp_packet->in++ & 0x7F);
    snmp_packet->used--;

    if(length_bytes > 4) {
      /*
       * Sanity check
       * It will not fit in the uint32_t
       */
      return 0;
    }

    if(snmp_packet->used == 0) {
      return 0;
    }

    *length = (uint32_t)*snmp_packet->in++;
    snmp_packet->used--;

    for(i = 1; i < length_bytes; ++i) {
      *length <<= 8;

      if(snmp_packet->used == 0) {
        return 0;
      }

      *length |= *snmp_packet->in++;
      snmp_packet->used--;
    }
  }

  *str = (const char *)snmp_packet->in;

  if(snmp_packet->used == 0 || snmp_packet->used < *length) {
    return 0;
  }

  snmp_packet->used -= *length;
  snmp_packet->in += *length;

  return 1;
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_decode_oid(snmp_packet_t *snmp_packet, snmp_oid_t *oid)
{
  uint8_t *buf_end, type;
  uint8_t len, j;
  div_t first;

  if(!snmp_ber_decode_type(snmp_packet, &type)) {
    return 0;
  }

  if(type != BER_DATA_TYPE_OBJECT_IDENTIFIER) {
    return 0;
  }

  if(!snmp_ber_decode_length(snmp_packet, &len)) {
    return 0;
  }

  buf_end = snmp_packet->in + len;

  if(snmp_packet->used == 0) {
    return 0;
  }

  snmp_packet->used--;
  first = div(*snmp_packet->in++, 40);

  oid->length = 0;

  oid->data[oid->length++] = (uint32_t)first.quot;
  oid->data[oid->length++] = (uint32_t)first.rem;

  while(snmp_packet->in != buf_end) {
    if(oid->length >= SNMP_MSG_OID_MAX_LEN) {
      return 0;
    }

    if(snmp_packet->used == 0) {
      return 0;
    }
    oid->data[oid->length] = (uint32_t)(*snmp_packet->in & 0x7F);
    for(j = 0; j < 4; j++) {
      snmp_packet->used--;
      if((*snmp_packet->in++ & 0x80) == 0) {
        break;
      }

      if(snmp_packet->used == 0) {
        return 0;
      }

      oid->data[oid->length] <<= 7;
      oid->data[oid->length] |= (*snmp_packet->in & 0x7F);
    }

    oid->length++;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
int
snmp_ber_decode_null(snmp_packet_t *snmp_packet)
{
  if(snmp_packet->used == 0) {
    return 0;
  }

  snmp_packet->in++;
  snmp_packet->used--;

  if(snmp_packet->used == 0) {
    return 0;
  }

  snmp_packet->in++;
  snmp_packet->used--;

  return 1;
}
/*---------------------------------------------------------------------------*/
