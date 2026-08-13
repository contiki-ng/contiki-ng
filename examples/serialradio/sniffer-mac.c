/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden
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

/**
 * \file
 *         Sniffer MAC driver - CSMA-based MAC that forwards ALL received
 *         packets to network layer (no address filtering in software).
 *         Supports both TX (with CSMA) and RX (promiscuous sniffing).
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

#include "net/mac/mac.h"
#include "net/mac/csma/csma.h"
#include "net/mac/csma/csma-output.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "dev/radio.h"
#include "sys/log.h"

#define LOG_MODULE "SnifferMAC"
#define LOG_LEVEL LOG_LEVEL_DBG

/*---------------------------------------------------------------------------*/
/* Use CSMA for sending - provides proper backoff and retransmissions */
/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{
  csma_output_packet(sent, ptr);
}
/*---------------------------------------------------------------------------*/
/* Receive path: forward ALL packets without filtering */
/*---------------------------------------------------------------------------*/
static void
packet_input(void)
{
  LOG_INFO("*** RX PACKET: %u bytes ***\n", packetbuf_datalen());

  /* Forward ALL packets to network layer - no address check, no duplicate check.
   * This is essential for promiscuous sniffing mode. */
  NETSTACK_NETWORK.input();
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return NETSTACK_RADIO.on();
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return NETSTACK_RADIO.off();
}
/*---------------------------------------------------------------------------*/
static int
max_payload(void)
{
  radio_value_t max_radio_payload_len;
  radio_result_t res;

  res = NETSTACK_RADIO.get_value(RADIO_CONST_MAX_PAYLOAD_LEN,
                                 &max_radio_payload_len);
  if(res != RADIO_RESULT_OK) {
    return 0;
  }
  return max_radio_payload_len;
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  radio_value_t radio_max_payload_len;

  /* Check that the radio can correctly report its max supported payload */
  if(NETSTACK_RADIO.get_value(RADIO_CONST_MAX_PAYLOAD_LEN, &radio_max_payload_len) != RADIO_RESULT_OK) {
    LOG_ERR("Radio does not support RADIO_CONST_MAX_PAYLOAD_LEN\n");
  }

  /* Initialize CSMA output queue for sending */
  csma_output_init();

  LOG_INFO("Sniffer MAC initialized (CSMA TX, promiscuous RX)\n");
  on();
}
/*---------------------------------------------------------------------------*/
const struct mac_driver sniffer_mac_driver = {
  "sniffer-mac",
  init,
  send_packet,
  packet_input,
  on,
  off,
  max_payload,
};
/*---------------------------------------------------------------------------*/
