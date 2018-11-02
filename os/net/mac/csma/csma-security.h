/*
 * Copyright (c) 2018, Tiny Mesh AS
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
 *         LLSEC802154 Security related configuration
 * \author
 *         Olav Frengstad <olav@tiny-mesh.com>
 */

#ifndef CSMA_SECURITY_H_
#define CSMA_SECURITY_H_


#ifdef CSMA_CONF_LLSEC_DEFAULT_KEY0
#define CSMA_LLSEC_DEFAULT_KEY0 CSMA_CONF_LLSEC_DEFAULT_KEY0
#else
#define CSMA_LLSEC_DEFAULT_KEY0 {0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f}
#endif

#ifdef CSMA_CONF_LLSEC_SECURITY_LEVEL
#define CSMA_LLSEC_SECURITY_LEVEL   CSMA_CONF_LLSEC_SECURITY_LEVEL
#else
#define CSMA_LLSEC_SECURITY_LEVEL   5
#endif /* CSMA_CONF_LLSEC_SECURITY_LEVEL */

#ifdef CSMA_CONF_LLSEC_KEY_ID_MODE
#define CSMA_LLSEC_KEY_ID_MODE   CSMA_CONF_LLSEC_KEY_ID_MODE
#else
#define CSMA_LLSEC_KEY_ID_MODE   FRAME802154_IMPLICIT_KEY
#endif /* CSMA_CONF_LLSEC_KEY_ID_MODE */

#ifdef CSMA_CONF_LLSEC_KEY_INDEX
#define CSMA_LLSEC_KEY_INDEX   CSMA_CONF_LLSEC_KEY_INDEX
#else
#define CSMA_LLSEC_KEY_INDEX   0
#endif /* CSMA_CONF_LLSEC_KEY_INDEX */

#ifdef CSMA_CONF_LLSEC_MAXKEYS
#define CSMA_LLSEC_MAXKEYS CSMA_CONF_LLSEC_MAXKEYS
#else
#define CSMA_LLSEC_MAXKEYS 1
#endif

#endif /* CSMA_SECURITY_H_ */
