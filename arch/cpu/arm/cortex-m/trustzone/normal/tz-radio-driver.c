/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "contiki.h"
#include "dev/radio.h"
#include "net/linkaddr.h"
#include "trustzone/tz-api.h"

#include <string.h>

/*---------------------------------------------------------------------------*/
static int
init(void)
{
  uint8_t device_address[8];

  if(tz_api_radio_get_object(RADIO_PARAM_64BIT_ADDR, device_address,
                             sizeof(device_address)) == RADIO_RESULT_OK) {
    memcpy(&linkaddr_node_addr, &device_address[8 - LINKADDR_SIZE],
           LINKADDR_SIZE);
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  return tz_api_radio_prepare(payload, payload_len);
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  return tz_api_radio_transmit(transmit_len);
}
/*---------------------------------------------------------------------------*/
static int
send(const void *payload, unsigned short payload_len)
{
  return tz_api_radio_send(payload, payload_len);
}
/*---------------------------------------------------------------------------*/
static int
radio_read(void *buf, unsigned short buf_len)
{
  return tz_api_radio_read(buf, buf_len);
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  return tz_api_radio_channel_clear();
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  return tz_api_radio_receiving_packet();
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  return tz_api_radio_pending_packet();
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return tz_api_radio_set_power_mode(true);
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return tz_api_radio_set_power_mode(false);
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  return tz_api_radio_get_value(param, value);
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  return tz_api_radio_set_value(param, value);
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  return tz_api_radio_get_object(param, dest, size);
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  return tz_api_radio_set_object(param, src, size);
}
/*---------------------------------------------------------------------------*/
const struct radio_driver tz_radio_driver = {
  init,
  prepare,
  transmit,
  send,
  radio_read,
  channel_clear,
  receiving_packet,
  pending_packet,
  on,
  off,
  get_value,
  set_value,
  get_object,
  set_object
};
/*---------------------------------------------------------------------------*/
