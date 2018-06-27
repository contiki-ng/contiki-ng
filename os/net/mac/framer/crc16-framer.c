/*
 * Copyright (c) 2018, Hasso-Plattner-Institut.
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
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Appends a 16-bit ITU-T CRC to frames as per IEEE 802.15.4.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/framer/crc16-framer.h"
#include "lib/crc16.h"
#include "net/packetbuf.h"
#include <string.h>

#ifdef CRC16_FRAMER_CONF_DECORATED_FRAMER
#define CRC16_FRAMER_DECORATED_FRAMER CRC16_FRAMER_CONF_DECORATED_FRAMER
#else /* CRC16_FRAMER_CONF_DECORATED_FRAMER */
#define CRC16_FRAMER_DECORATED_FRAMER framer_802154
#endif /* CRC16_FRAMER_CONF_DECORATED_FRAMER */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "ContikiMAC-framer"
#define LOG_LEVEL LOG_LEVEL_FRAMER

extern const struct framer CRC16_FRAMER_DECORATED_FRAMER;

/*---------------------------------------------------------------------------*/
void
crc16_framer_append_checksum(uint8_t *dataptr, uint8_t datalen)
{
  uint16_t checksum;

  checksum = crc16_data(dataptr, datalen, 0);
  dataptr[datalen] = checksum & 0xff;
  dataptr[datalen + 1] = checksum >> 8;
}
/*---------------------------------------------------------------------------*/
int
crc16_framer_check_checksum(uint8_t *dataptr, uint8_t datalen)
{
  uint16_t received_checksum;
  uint16_t expected_checksum;

  expected_checksum = crc16_data(dataptr, datalen, 0);
  received_checksum = dataptr[datalen] | (dataptr[datalen + 1] << 8);
  return expected_checksum == received_checksum;
}
/*---------------------------------------------------------------------------*/
static int
length(void)
{
  return CRC16_FRAMER_DECORATED_FRAMER.length() + CRC16_FRAMER_CHECKSUM_LEN;
}
/*---------------------------------------------------------------------------*/
static int
create(void)
{
  int result;

  /* call decorated framer */
  result = CRC16_FRAMER_DECORATED_FRAMER.create();
  if(result == FRAMER_FAILED) {
    LOG_ERR("decorated framer failed\n");
    return FRAMER_FAILED;
  }

  /* append checksum */
  crc16_framer_append_checksum(packetbuf_hdrptr(), packetbuf_totlen());
  packetbuf_set_datalen(packetbuf_datalen() + CRC16_FRAMER_CHECKSUM_LEN);

  return result;
}
/*---------------------------------------------------------------------------*/
static int
parse(void)
{
  /* validate frame length */
  if(packetbuf_datalen() < CRC16_FRAMER_CHECKSUM_LEN) {
    LOG_ERR("frame too short\n");
    return FRAMER_FAILED;
  }

  /* validate checksum */
  packetbuf_set_datalen(packetbuf_datalen() - CRC16_FRAMER_CHECKSUM_LEN);
  if(!crc16_framer_check_checksum(packetbuf_hdrptr(), packetbuf_totlen())) {
    LOG_ERR("invalid checksum\n");
    return FRAMER_FAILED;
  }

  /* call decorated framer */
  return CRC16_FRAMER_DECORATED_FRAMER.parse();
}
/*---------------------------------------------------------------------------*/
const struct framer crc16_framer = {
  length,
  create,
  parse
};
/*---------------------------------------------------------------------------*/
