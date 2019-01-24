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

#ifndef PACKETUTILS_H_
#define PACKETUTILS_H_

#include "os/sys/log.h"

/* Log level for packetutil module */
#ifdef LOG_CONF_LEVEL_PACKETUTILS
#define LOG_LEVEL_PACKETUTILS LOG_CONF_LEVEL_PACKETUTILS
#else /* PACKET_UTILS_CONF_LOG_LEVEL */
#define LOG_LEVEL_PACKETUTILS LOG_LEVEL_NONE
#endif /* LOG_CONF_LEVEL_PACKETUTILS */

/**
 * Package the all packetbuf_attr data.
 *
 * Caveat,
 *   This function reads global variable `packetbuf`
 *
 * For examples,
 *   If already set packetbuf_attr(PACKETBUF_ATTR_RSSI, -80),
 *   data buffer is following.
 *   +------+------+-------------+
 *   | 0x01 | 0x04 | 0xff | 0xb0 |
 *   +------+------+-------------+
 *
 *   The 1st octet shows number of attributes.
 *   After the 2nd octet, 3 octets group shows 1 attribute.
 *   The 1st octet of group shows attribute type, see packetbuf.h
 *   The 2nd and 3rd octets of group show attribute value,
 *   2nd shows upper 8bits, and 3rd shows lower 8bits.
 *
 *   If packetbuf had more attribute, 3 octets group continues.
 *
 * \param data The buffer stores packaged packetbuf_attr data.
 * \param size Length of the `data` buffer.
 * \return The length of packaged data if success, -1 if falure.
 */
int packetutils_serialize_atts(uint8_t *data, int size);
/**
 * Unpackage the all packetbuf_attr data.
 *
 * Caveat,
 *   This function overwrites global variable `packetbuf`
 *
 * \param data The buffer stores unpackaged packetbuf_attr data.
 * \param size Length of the `data` buffer.
 * \return The length of unpackaged data if success, -1 if falure.
 */
int packetutils_deserialize_atts(const uint8_t *data, int size);
/**
 * Package the all packetbuf_addr data.
 *
 * Caveat,
 *   This function reads global variable `packetbuf`
 *
 * \param data The buffer stores packaged packetbuf_addr data.
 * \param size Length of the `data` buffer.
 * \return The length of packaged data if success, -1 if falure.
 */
int packetutils_serialize_addrs(uint8_t *data, int size);
/**
 * Package the all packetbuf_addr data.
 *
 * Caveat,
 *   This function overwrites global variable `packetbuf`
 *
 * \param data The buffer stores unpackaged packetbuf_addr data.
 * \param size Length of the `data` buffer.
 * \return The length of unpackaged data if success, -1 if falure.
 */
int packetutils_deserialize_addrs(const uint8_t *data, int size);

#endif /* PACKETUTILS_H_ */
