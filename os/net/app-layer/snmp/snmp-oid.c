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

#include "snmp-oid.h"
#include "snmp-ber.h"

#define LOG_MODULE "SNMP [oid]"
#define LOG_LEVEL LOG_LEVEL_SNMP

/*---------------------------------------------------------------------------*/
int
snmp_oid_cmp_oid(uint32_t *oid1, uint32_t *oid2)
{
  uint8_t i;

  i = 0;
  while(oid1[i] != ((uint32_t)-1) &&
        oid2[i] != ((uint32_t)-1)) {
    if(oid1[i] != oid2[i]) {
      if(oid1[i] < oid2[i]) {
        return -1;
      }
      return 1;
    }
    i++;
  }

  if(oid1[i] == ((uint32_t)-1) &&
     oid2[i] != ((uint32_t)-1)) {
    return -1;
  }

  if(oid1[i] != ((uint32_t)-1) &&
     oid2[i] == ((uint32_t)-1)) {
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_oid_encode_oid(unsigned char *out, uint32_t *out_len, uint32_t *oid)
{
  uint32_t original_out_len;
  uint32_t *oid_start = oid;
  uint32_t num;

  original_out_len = *out_len;
  while(*oid != ((uint32_t)-1)) {
    ++oid;
  }
  --oid;

  while(oid != oid_start) {
    num = *oid;
    (*out_len)++;
    *out-- = (uint8_t)(num & 0x7F);
    num >>= 7;

    while(num) {
      (*out_len)++;
      *out-- = (uint8_t)((num & 0x7F) | 0x80);
      num >>= 7;
    }
    --oid;
  }

  num = *(out + 1) + 40 * *oid;
  (*out_len)--;
  out++;
  (*out_len)++;
  *out-- = (uint8_t)(num & 0x7F);
  num >>= 7;

  while(num) {
    (*out_len)++;
    *out-- = (uint8_t)((num & 0x7F) | 0x80);
    num >>= 7;
  }

  out = snmp_ber_encode_length(out, out_len, ((*out_len - original_out_len) & 0xFF));
  out = snmp_ber_encode_type(out, out_len, SNMP_DATA_TYPE_OBJECT);

  return out;
}
/*---------------------------------------------------------------------------*/
uint8_t *
snmp_oid_decode_oid(uint8_t *buf, uint32_t *buff_len, uint32_t *oid, uint32_t *oid_len)
{
  uint32_t *start;
  uint8_t *buf_end, type;
  uint8_t len;
  div_t first;

  start = oid;

  buf = snmp_ber_decode_type(buf, buff_len, &type);
  if(buf == NULL) {
    return NULL;
  }

  if(type != SNMP_DATA_TYPE_OBJECT) {
    return NULL;
  }

  buf = snmp_ber_decode_length(buf, buff_len, &len);
  if(buf == NULL) {
    return NULL;
  }

  buf_end = buf + len;

  (*buff_len)--;
  first = div(*buf++, 40);
  *oid++ = (uint32_t)first.quot;
  *oid++ = (uint32_t)first.rem;

  while(buf != buf_end) {
    --(*oid_len);
    if(*oid_len == 0) {
      return NULL;
    }

    int i;

    *oid = (uint32_t)(*buf & 0x7F);
    for(i = 0; i < 4; i++) {
      (*buff_len)--;
      if((*buf++ & 0x80) == 0) {
        break;
      }

      *oid <<= 7;
      *oid |= (*buf & 0x7F);
    }

    ++oid;
  }

  *oid++ = ((uint32_t)-1);
  *oid_len = (uint32_t)(oid - start);

  return buf;
}
/*---------------------------------------------------------------------------*/
void
snmp_oid_copy(uint32_t *dst, uint32_t *src)
{
  uint8_t i;

  i = 0;
  while(src[i] != ((uint32_t)-1)) {
    dst[i] = src[i];
    i++;
  }
  /*
   * Copy the "null" terminator
   */
  dst[i] = src[i];
}
/*---------------------------------------------------------------------------*/
#if LOG_LEVEL == LOG_LEVEL_DBG
void
snmp_oid_print(uint32_t *oid)
{
  uint8_t i;

  i = 0;
  LOG_DBG("{");
  while(oid[i] != ((uint32_t)-1)) {
    LOG_DBG_("%lu", (unsigned long)oid[i]);
    i++;
    if(oid[i] != ((uint32_t)-1)) {
      LOG_DBG_(".");
    }
  }
  LOG_DBG_("}\n");
}
#endif /* LOG_LEVEL == LOG_LEVEL_DBG  */
