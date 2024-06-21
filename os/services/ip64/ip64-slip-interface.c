/*
 * Copyright (c) 2012, Thingsquare, http://www.thingsquare.com/.
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
 *
 */
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/slip.h"
#include "ip64/ip64.h"

#include <string.h>

/*---------------------------------------------------------------------------*/

#include "sys/log.h"

#define LOG_MODULE  "IP64"
#define LOG_LEVEL   LOG_LEVEL_IP64

/*---------------------------------------------------------------------------*/

static uip_ipaddr_t last_sender;

/*---------------------------------------------------------------------------*/
void
ip64_slip_interface_input(uint8_t *packet, uint16_t len) {
    /* Dummy definition: this function is not actually called, but must
       be here to conform to the ip65-interface.h structure. */
}

/*---------------------------------------------------------------------------*/
static void
input_callback(void) {
    /*PRINTF("SIN: %u\n", uip_len);*/
    if (uip_buf[0] == '!') {
        LOG_DBG("Got configuration message of type %c\n", uip_buf[1]);
        uipbuf_clear();
#if 0
        if(uip_buf[1] == 'P') {
          uip_ipaddr_t prefix;
          /* Here we set a prefix !!! */
          memset(&prefix, 0, 16);
          memcpy(&prefix, &uip_buf[2], 8);
          LOG_DBG("Setting prefix ");
          LOG_DBG_6ADDR(&prefix);
          LOG_DBG_("\n");
          set_prefix_64(&prefix);
        }
#endif
    } else if (uip_buf[0] == '?') {
        LOG_DBG("Got request message of type %c\n", uip_buf[1]);
        if (uip_buf[1] == 'M') {
            const char *hexchar = "0123456789abcdef";
            int j;
            /* this is just a test so far... just to see if it works */
            uip_buf[0] = '!';
            for (j = 0; j < 8; j++) {
                uip_buf[2 + j * 2] = hexchar[uip_lladdr.addr[j] >> 4];
                uip_buf[3 + j * 2] = hexchar[uip_lladdr.addr[j] & 15];
            }
            uip_len = 18;
            slip_write(uip_buf, uip_len);
        }
        uipbuf_clear();
    } else {

        /* Save the last sender received over SLIP to avoid bouncing the
           packet back if no route is found */
        uip_ipaddr_copy(&last_sender, &UIP_IP_BUF->srcipaddr);

        uint16_t len = ip64_4to6(uip_buf, uip_len,
                                 ip64_packet_buffer);
        if (len > 0) {
            memcpy(uip_buf, ip64_packet_buffer, len);
            uip_len = len;
            /*      LOG_DBG("send len %d\n", len); */
        } else {
            uipbuf_clear();
        }
    }
}

/*---------------------------------------------------------------------------*/
static void
init(void) {
    LOG_INFO("ip64-slip-interface: init\n");
    process_start(&slip_process, NULL);
    slip_set_input_callback(input_callback);
}

/*---------------------------------------------------------------------------*/
static int
output(void) {
    int len;

    LOG_DBG("ip64-slip-interface: output source ");

    /*
    PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
    PRINTF(" destination ");
    PRINT6ADDR(&UIP_IP_BUF->destipaddr);
    PRINTF("\n");
    */
    if (uip_ipaddr_cmp(&last_sender, &UIP_IP_BUF->srcipaddr)) {
        LOG_WARN("ip64-interface: output, not sending bounced message\n");
    } else {
        len = ip64_6to4(uip_buf, uip_len,
                        ip64_packet_buffer);
        LOG_DBG("ip64-interface: output len %d\n", len);
        if (len > 0) {
            memcpy(uip_buf, ip64_packet_buffer, len);
            uip_len = len;
            slip_send();
            return len;
        }
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface ip64_slip_interface = {
        init, output
};
/*---------------------------------------------------------------------------*/
