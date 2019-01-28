/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         packetbuf data package utility implementation.
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Takeshi Sakoda <takeshi1.sakoda@toshiba.co.jp>
 *
 */

/**
 * \addtogroup packetutils
 * @{
*/

#include "contiki.h"
#include "packetutils.h"
#include "net/packetbuf.h"

/* Log configuration */
#define LOG_MODULE "P-utils"
#define LOG_LEVEL LOG_LEVEL_PACKETUTILS

/*---------------------------------------------------------------------------*/
int
packetutils_serialize_atts(uint8_t *data, int size)
{
  int i;
  /* set the length first later */
  int pos = 1;
  int cnt = 0;
  /* assume that values are 16-bit */
  uint16_t val;
  LOG_INFO("packetutils: serializing packet atts\n");
  for(i = 0; i < PACKETBUF_NUM_ATTRS; i++) {
    val = packetbuf_attr(i);
    if(val != 0) {
      if(pos + 3 > size) {
        return -1;
      }
      data[pos++] = i;
      data[pos++] = val >> 8;
      data[pos++] = val & 255;
      cnt++;
      LOG_DBG(" %2d=%d\n", i, val);
    }
  }
  LOG_INFO("serialized %d packet atts\n", cnt);

  data[0] = cnt;
  return pos;
}
/*---------------------------------------------------------------------------*/
int
packetutils_deserialize_atts(const uint8_t *data, int size)
{
  int i, cnt, pos;

  pos = 0;
  cnt = data[pos++];
  LOG_INFO("deserializing %d packet atts\n", cnt);
  if(cnt > PACKETBUF_NUM_ATTRS) {
    LOG_ERR(" *** too many: %u!\n", PACKETBUF_NUM_ATTRS);
    return -1;
  }
  for(i = 0; i < cnt; i++) {
    if(data[pos] >= PACKETBUF_NUM_ATTRS) {
      /* illegal attribute identifier */
      LOG_ERR(" *** unknown attribute %u\n", data[pos]);
      return -1;
    }
    LOG_DBG(" %2d=%d\n", data[pos], (data[pos + 1] << 8) | data[pos + 2]);
    packetbuf_set_attr(data[pos], (data[pos + 1] << 8) | data[pos + 2]);
    pos += 3;
  }
  return pos;
}
/*---------------------------------------------------------------------------*/
int
packetutils_serialize_addrs(uint8_t *data, int size)
{
  int i, j;
  /* set the length first later */
  int pos = 1;
  int cnt = 0;
  /* assume that values are 16-bit */
  linkaddr_t *val;

  for(i = PACKETBUF_ADDR_FIRST;
      i < (PACKETBUF_ADDR_FIRST + PACKETBUF_NUM_ADDRS);
      i++) {
    val = (linkaddr_t *)packetbuf_addr(i);
    if(linkaddr_cmp(val, &linkaddr_null)) {
      continue;
    } else {
      if(pos + 1 + LINKADDR_SIZE > size) {
        /* serialize data length is over */
        LOG_ERR(" *** too large serialize data length %u\n", pos + 1 + LINKADDR_SIZE);
        return -1;
      }
      data[pos++] = i;
      LOG_DBG(" %2d=", i);
      for(j = 0; j < LINKADDR_SIZE; j++) {
        if(j > 0 && j % 2 == 0)
          LOG_DBG_(".");
        data[pos++] = val->u8[j];
        LOG_DBG_("%02x", val->u8[j]);
      }
      LOG_DBG_("\n");
      cnt++;
    }
  }
  LOG_INFO("serialized %d packet addrs\n", cnt);

  data[0] = cnt;
  return pos;
}
/*---------------------------------------------------------------------------*/
int
packetutils_deserialize_addrs(const uint8_t *data, int size)
{
  int i, j, cnt, pos;
  linkaddr_t addr;

  pos = 0;
  cnt = data[pos++];
  LOG_INFO("deserializing %d packet addrs\n", cnt);
  if(cnt > PACKETBUF_NUM_ADDRS) {
    LOG_ERR(" *** too many: %u!\n", PACKETBUF_NUM_ADDRS);
    return -1;
  }
  for(i = 0; i < cnt; i++) {
    if(data[pos] < PACKETBUF_ADDR_FIRST ||
       data[pos] > PACKETBUF_ADDR_FIRST + PACKETBUF_NUM_ADDRS) {
      /* illegal attribute identifier */
      LOG_ERR(" *** unknown attribute %u\n", data[pos]);
      return -1;
    }
    LOG_DBG(" %2d=", data[pos]);
    for(j = 0; j < LINKADDR_SIZE; j++) {
      if(j > 0 && j % 2 == 0)
        LOG_DBG_(".");
      addr.u8[j] = data[pos + 1 + j];
      LOG_DBG_("%02x", addr.u8[j]);
    }
    LOG_DBG_("\n");

    packetbuf_set_addr(data[pos], &addr);
    pos += (1 + LINKADDR_SIZE);
  }
  return pos;
}
/*---------------------------------------------------------------------------*/
/** @} */
