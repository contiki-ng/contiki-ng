/*
 * Copyright (c) 2017, Hasso-Plattner-Institut.
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

#include "dev/radio.h"
#include "net/netstack.h"
#include "net/packetbuf.h"

const uint8_t radio_shr[RADIO_SHR_LEN] = { 0x00, 0x00, 0x00, 0x00, 0xA7 };

/*---------------------------------------------------------------------------*/
radio_value_t
radio_get_rssi(void)
{
  radio_value_t rssi;
  NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &rssi);
  return rssi;
}
/*---------------------------------------------------------------------------*/
uint_fast16_t
radio_read_phy_header_to_packetbuf(void)
{
  uint_fast16_t frame_length = NETSTACK_RADIO.async_read_phy_header();
  packetbuf_set_datalen(frame_length);
  return frame_length;
}
/*---------------------------------------------------------------------------*/
radio_async_result_t
radio_read_payload_to_packetbuf(uint_fast16_t bytes)
{
  return NETSTACK_RADIO.async_read_payload(packetbuf_hdrptr()
         + NETSTACK_RADIO.async_read_payload_bytes(), bytes);
}
/*---------------------------------------------------------------------------*/
uint_fast16_t
radio_remaining_payload_bytes(void)
{
  return packetbuf_totlen() - NETSTACK_RADIO.async_read_payload_bytes();
}
/*---------------------------------------------------------------------------*/
