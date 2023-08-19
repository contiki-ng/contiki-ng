/*
 * Copyright (c) 2018, RISE SICS.
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
  * \addtogroup sys
  * @{
  *
  * \defgroup node-id Node ID management
  * @{
  *
  * \file
  *    Node-id (simple 16-bit identifiers) handling
  * \author Simon Duquennoy <simon.duquennoy@ri.se>
  *
  */

#ifndef NODE_ID_H_
#define NODE_ID_H_

#include "contiki.h"
#include "net/linkaddr.h"
#if BUILD_WITH_DEPLOYMENT
#include "services/deployment/deployment.h"
#endif

/* A global variable that hosts the node ID */
extern uint16_t node_id;
/**
 * Initialize the node ID. Must be called after initialized of linkaddr
 */
static inline void
node_id_init(void)
{
#if BUILD_WITH_DEPLOYMENT
  deployment_init();
#else /* BUILD_WITH_DEPLOYMENT */
  /* Initialize with a default value derived from linkaddr */
  node_id = linkaddr_node_addr.u8[LINKADDR_SIZE - 1]
            + (linkaddr_node_addr.u8[LINKADDR_SIZE - 2] << 8);
#endif /* BUILD_WITH_DEPLOYMENT */
}

#endif /* NODE_ID_H_ */
/**
 * @}
 * @}
 */
