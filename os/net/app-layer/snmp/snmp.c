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
#include "snmp-mib.h"
#include "snmp-engine.h"

#define LOG_MODULE "SNMP"
#define LOG_LEVEL LOG_LEVEL_SNMP

/*---------------------------------------------------------------------------*/
#define SNMP_SERVER_PORT UIP_HTONS(SNMP_PORT)
PROCESS(snmp_process, "SNMP Process");

static struct uip_udp_conn *snmp_udp_conn = NULL;

/*---------------------------------------------------------------------------*/
static void
snmp_process_data(void)
{
  static unsigned char packet[SNMP_MAX_PACKET_SIZE];
  unsigned char *packet_end;
  static uint32_t packet_len;

  packet_end = packet + sizeof(packet) - 1;
  packet_len = 0;

  LOG_DBG("receiving UDP datagram from [");
  LOG_DBG_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_DBG_("]:%u", uip_ntohs(UIP_UDP_BUF->srcport));
  LOG_DBG_(" Length: %u\n", uip_datalen());

  /*
   * Handle the request
   */
  if((packet_end = snmp_engine(uip_appdata, uip_datalen(), packet_end, &packet_len)) == NULL) {
    LOG_DBG("Error while handling the request\n");
  } else {
    LOG_DBG("Sending response\n");
    /*
     * Send the response
     */
    uip_udp_packet_sendto(snmp_udp_conn, packet_end, packet_len, &UIP_IP_BUF->srcipaddr, UIP_UDP_BUF->srcport);
  }
}
/*---------------------------------------------------------------------------*/
void
snmp_init()
{
  snmp_mib_init();
  process_start(&snmp_process, NULL);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(snmp_process, ev, data)
{
  PROCESS_BEGIN();

  /* new connection with remote host */
  snmp_udp_conn = udp_new(NULL, 0, NULL);
  udp_bind(snmp_udp_conn, SNMP_SERVER_PORT);
  LOG_DBG("Listening on port %u\n", uip_ntohs(snmp_udp_conn->lport));

  while(1) {
    PROCESS_YIELD();

    if(ev == tcpip_event) {
      if(uip_newdata()) {
        snmp_process_data();
      }
    }
  } /* while (1) */

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
