/*
 * Copyright (c) 2019, Inria.
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

#include <contiki.h>
#include <contiki-net.h>

#include "net/routing/routing.h"
#include "net/mac/tsch/sixtop/sixtop.h"
#include "services/msf/msf.h"

PROCESS(msf_root_process, "MSF root");
AUTOSTART_PROCESSES(&msf_root_process);

PROCESS_THREAD(msf_root_process, ev, data)
{
  static struct udp_socket s;
  PROCESS_BEGIN();

  if(udp_socket_register(&s, NULL, NULL) < 0 ||
     udp_socket_bind(&s, APP_UDP_PORT) < 0) {
    printf("CRITICAL ERROR: socket initialization failed\n");
    /*
     * we don't need to process received packets, but we need to have
     * a socket so as to prevent sending back ICMP error packets
     */
  } else {
    NETSTACK_ROUTING.root_start();
    sixtop_add_sf(&msf);
  }

  PROCESS_END();
}
