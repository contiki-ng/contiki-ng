/*
 * Copyright (c) 2015, Hasso-Plattner-Institut.
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
 *         Autoconfigures the akes_mac_driver.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

/* configure Contiki */
#define CSMA_CONF_WITH_AKES 1
#define NBR_TABLE_CONF_WITH_FIND_REMOVABLE 0
#define LLSEC802154_CONF_USES_FRAME_COUNTER 1
#define CSPRNG_CONF_ENABLED 1
#define LLSEC802154_CONF_USES_AUX_HEADER 1
#define FRAMER_802154_CONF_INIT_SEQNOS 0

/* configure AKES */
#define AKES_MAC_CONF_ENABLED 1
#define NETSTACK_CONF_MAC akes_mac_driver
#define AKES_MAC_CONF_DECORATED_MAC csma_driver
#define NETSTACK_CONF_FRAMER akes_mac_framer
#define AKES_MAC_CONF_DECORATED_FRAMER framer_802154

/* configure AKES_STRATEGY */
#include "os/services/akes/akes-strategy-autoconf.h"
