/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
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

/*
 * \file
 *   Normal-world radio driver proxy for TrustZone.
 *
 *   This thin driver implements the Contiki-NG radio_driver interface
 *   by forwarding all operations to the secure world via NSC calls.
 *   A process handles asynchronous frame delivery from the secure
 *   world to the normal-world MAC layer.
 *
 * \author
 *   Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "trustzone/tz-radio.h"

/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "TZRadio"
#define LOG_LEVEL LOG_LEVEL_INFO
/*---------------------------------------------------------------------------*/
static volatile bool rx_poll_requested;
/*---------------------------------------------------------------------------*/
PROCESS(tz_radio_process, "TZ radio process");
/*---------------------------------------------------------------------------*/
static bool
rx_poll_callback(void)
{
  rx_poll_requested = true;
  process_poll(&tz_radio_process);
  return true;
}
/*---------------------------------------------------------------------------*/
static int
nr_init(void)
{
  int result;

  result = tz_radio_init();

  if(result) {
    if(!tz_radio_register_rx_callback(rx_poll_callback)) {
      LOG_ERR("Failed to register RX callback\n");
    }
    process_start(&tz_radio_process, NULL);
  }

  return result;
}
/*---------------------------------------------------------------------------*/
static int
nr_prepare(const void *payload, unsigned short payload_len)
{
  return tz_radio_prepare(payload, payload_len);
}
/*---------------------------------------------------------------------------*/
static int
nr_transmit(unsigned short transmit_len)
{
  return tz_radio_transmit(transmit_len);
}
/*---------------------------------------------------------------------------*/
static int
nr_send(const void *payload, unsigned short payload_len)
{
  return tz_radio_send(payload, payload_len);
}
/*---------------------------------------------------------------------------*/
static int
nr_read(void *buf, unsigned short buf_len)
{
  return tz_radio_read(buf, buf_len);
}
/*---------------------------------------------------------------------------*/
static int
nr_channel_clear(void)
{
  return tz_radio_channel_clear();
}
/*---------------------------------------------------------------------------*/
static int
nr_receiving_packet(void)
{
  return tz_radio_receiving_packet();
}
/*---------------------------------------------------------------------------*/
static int
nr_pending_packet(void)
{
  return tz_radio_pending_packet();
}
/*---------------------------------------------------------------------------*/
static int
nr_on(void)
{
  return tz_radio_on();
}
/*---------------------------------------------------------------------------*/
static int
nr_off(void)
{
  return tz_radio_off();
}
/*---------------------------------------------------------------------------*/
static radio_result_t
nr_get_value(radio_param_t param, radio_value_t *value)
{
  return tz_radio_get_value(param, value);
}
/*---------------------------------------------------------------------------*/
static radio_result_t
nr_set_value(radio_param_t param, radio_value_t value)
{
  return tz_radio_set_value(param, value);
}
/*---------------------------------------------------------------------------*/
static radio_result_t
nr_get_object(radio_param_t param, void *dest, size_t size)
{
  return tz_radio_get_object(param, dest, size);
}
/*---------------------------------------------------------------------------*/
static radio_result_t
nr_set_object(radio_param_t param, const void *src, size_t size)
{
  return tz_radio_set_object(param, src, size);
}
/*---------------------------------------------------------------------------*/
const struct radio_driver tz_radio_driver = {
  nr_init,
  nr_prepare,
  nr_transmit,
  nr_send,
  nr_read,
  nr_channel_clear,
  nr_receiving_packet,
  nr_pending_packet,
  nr_on,
  nr_off,
  nr_get_value,
  nr_set_value,
  nr_get_object,
  nr_set_object
};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tz_radio_process, ev, data)
{
  PROCESS_BEGIN();

  LOG_INFO("TZ radio process started\n");

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

    if(rx_poll_requested) {
      rx_poll_requested = false;

      if(tz_radio_pending_packet()) {
        int8_t rssi;
        uint8_t lqi;
        int len;

        tz_radio_get_rx_attributes(&rssi, &lqi);

        packetbuf_clear();
        len = tz_radio_read(packetbuf_dataptr(), PACKETBUF_SIZE);
        if(len > 0) {
          packetbuf_set_datalen(len);
          packetbuf_set_attr(PACKETBUF_ATTR_RSSI, (int)rssi);
          packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, lqi);
          LOG_DBG("RX %d bytes, delivering to MAC\n", len);
          NETSTACK_MAC.input();
        }
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
