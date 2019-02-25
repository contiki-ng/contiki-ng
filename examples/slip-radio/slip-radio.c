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
 *         Slip-radio driver
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */
#include "contiki.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/slip.h"
#include <string.h>
#include "net/netstack.h"
#include "net/packetbuf.h"

#include "cmd.h"
#include "slip-radio.h"
#include "packetutils.h"
#include "os/sys/log.h"

#include <stdio.h>

#define LOG_MODULE "slip-radio"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
#ifdef SLIP_RADIO_CONF_SENSORS
extern const struct slip_radio_sensors SLIP_RADIO_CONF_SENSORS;
#endif

/* max 16 packets at the same time??? */
uint8_t packet_ids[16];
int packet_pos;

static int slip_radio_cmd_handler(const uint8_t *data, int len);

int cmd_handler_cc2420(const uint8_t *data, int len);
/*---------------------------------------------------------------------------*/
#ifdef CMD_CONF_HANDLERS
CMD_HANDLERS(CMD_CONF_HANDLERS);
#else
CMD_HANDLERS(slip_radio_cmd_handler);
#endif

static const uint16_t mac_src_pan_id = IEEE802154_PANID;
/*---------------------------------------------------------------------------*/
static int
is_broadcast_addr(uint8_t mode, uint8_t *addr)
{
  int i = mode == FRAME802154_SHORTADDRMODE ? 2 : 8;
  while(i-- > 0) {
    if(addr[i] != 0xff) {
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
parse_frame(void)
{
  frame802154_t frame;
  int len;
  len = packetbuf_datalen();
  if(frame802154_parse(packetbuf_dataptr(), len, &frame)) {
    if(frame.fcf.dest_addr_mode) {
      if(frame.dest_pid != mac_src_pan_id &&
         frame.dest_pid != FRAME802154_BROADCASTPANDID) {
        /* Packet to another PAN */
        return 0;
      }
      if(!is_broadcast_addr(frame.fcf.dest_addr_mode, frame.dest_addr)) {
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, (linkaddr_t *)&frame.dest_addr);
      }
    }
    packetbuf_set_addr(PACKETBUF_ADDR_SENDER, (linkaddr_t *)&frame.src_addr);
    packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, frame.seq);

    return 0;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
packet_sent(void *ptr, int status, int transmissions)
{
  uint8_t buf[20];
  uint8_t sid;
  int pos;
  sid = *((uint8_t *)ptr);
  LOG_DBG("Slip-radio: packet sent! sid: %d, status: %d, tx: %d\n",
          sid, status, transmissions);
  /* packet callback from lower layers */
  /*  neighbor_info_packet_sent(status, transmissions); */
  pos = 0;
  buf[pos++] = '!';
  buf[pos++] = 'R';
  buf[pos++] = sid;
  buf[pos++] = status; /* one byte ? */
  buf[pos++] = transmissions;
  cmd_send(buf, pos);
}
/*---------------------------------------------------------------------------*/
static int
slip_radio_cmd_handler(const uint8_t *data, int len)
{
  int i;
  if(data[0] == '!') {
    /* should send out stuff to the radio - ignore it as IP */
    /* --- s e n d --- */
    if(data[1] == 'S') {
      int pos;
      packet_ids[packet_pos] = data[2];

      packetbuf_clear();
      pos = packetutils_deserialize_atts(&data[3], len - 3);
      if(pos < 0) {
        LOG_ERR("illegal packet attributes\n");
        return 1;
      }
      pos += 3;
      len -= pos;
      if(len > PACKETBUF_SIZE) {
        len = PACKETBUF_SIZE;
      }
      memcpy(packetbuf_dataptr(), &data[pos], len);
      packetbuf_set_datalen(len);

      LOG_DBG("sending %u (%d bytes)\n",
              data[2], packetbuf_datalen());

      /* parse frame before sending to get addresses, etc. */
      parse_frame();
      NETSTACK_MAC.send(packet_sent, &packet_ids[packet_pos]);

      packet_pos++;
      if(packet_pos >= sizeof(packet_ids)) {
        packet_pos = 0;
      }

      return 1;
    } else if(data[1] == 'V') {
      int type = ((uint16_t)data[2] << 8) | data[3];
      int value = ((uint16_t)data[4] << 8) | data[5];
      int param = type; /* packetutils_to_radio_param(type); */
      if(param < 0) {
        printf("radio: unknown parameter %d (can not set to %d)\n", type, value);
      } else {
        if(param == RADIO_PARAM_RX_MODE) {
          printf("radio: setting rxmode to 0x%x\n", value);
        } else if(param == RADIO_PARAM_PAN_ID) {
          printf("radio: setting pan id to 0x%04x\n", value);
        } else if(param == RADIO_PARAM_CHANNEL) {
          printf("radio: setting channel: %u\n", value);
        } else {
          printf("radio: setting param %d to %d (0x%02x)\n", param, value, value);
        }
        NETSTACK_RADIO.set_value(param, value);
      }
      return 1;
    }
  } else if(data[0] == '?') {
    LOG_DBG("Got request message of type %c\n", data[1]);
    if(data[1] == 'M') {
      /* this is just a test so far... just to see if it works */
      uip_buf[0] = '!';
      uip_buf[1] = 'M';
      for(i = 0; i < UIP_LLADDR_LEN; i++) {
        uip_buf[2 + i] = uip_lladdr.addr[i];
      }
      uip_len = 10;
      cmd_send(uip_buf, uip_len);
      return 1;
    } else if(data[1] == 'V') {
      /* ask the radio about the specific parameter and send it back... */
      int type = ((uint16_t)data[2] << 8) | data[3];
      int value;
      int param = type; /* packetutils_to_radio_param(type); */
      if(param < 0) {
        printf("radio: unknown parameter %d\n", type);
      }

      NETSTACK_RADIO.get_value(param, &value);

      uip_buf[0] = '!';
      uip_buf[1] = 'V';
      uip_buf[2] = type >> 8;
      uip_buf[3] = type & 0xff;
      uip_buf[4] = value >> 8;
      uip_buf[5] = value & 0xff;
      uip_len = 6;
      cmd_send(uip_buf, uip_len);
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
slip_input_callback(void)
{
  LOG_DBG("SR-SIN: %u '%c%c'\n", uip_len, uip_buf[0], uip_buf[1]);
  if(!cmd_input(uip_buf, uip_len)) {
    cmd_send((uint8_t *)"EUnknown command", 16);
  }
  uipbuf_clear();
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  slip_arch_init();
  process_start(&slip_process, NULL);
  slip_set_input_callback(slip_input_callback);
  packet_pos = 0;
}
/*---------------------------------------------------------------------------*/
PROCESS(slip_radio_process, "Slip radio process");
AUTOSTART_PROCESSES(&slip_radio_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(slip_radio_process, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();

  init();
#ifdef SLIP_RADIO_CONF_SENSORS
  SLIP_RADIO_CONF_SENSORS.init();
#endif
  LOG_INFO("Slip Radio started\n");

  etimer_set(&et, CLOCK_SECOND * 3);

  while(1) {
    PROCESS_YIELD();

    if(etimer_expired(&et)) {
      etimer_reset(&et);
#ifdef SLIP_RADIO_CONF_SENSORS
      SLIP_RADIO_CONF_SENSORS.send();
#endif
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
