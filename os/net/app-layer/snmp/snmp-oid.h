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

/**
 * \addtogroup snmp
 * @{
 */

#ifndef SNMP_OID_H_
#define SNMP_OID_H_

#include "snmp.h"

#define SNMP_DATA_TYPE_OBJECT                   0x06

/**
 * @brief Compares to oids
 *
 * @param oid1 First Oid
 * @param oid2 Second Oid
 *
 * @return < 0 if oid1 < oid2, > 0 if oid1 > oid2 and 0 if they are equal
 */
int
snmp_oid_cmp_oid(uint32_t *oid1, uint32_t *oid2);

/**
 * @brief Encodes a Oid
 *
 * @param out A pointer to the end of the buffer
 * @param out_len A pointer to the buffer length
 * @param oid The Oid
 *
 * @return NULL if error or the next entry in the buffer
 */
unsigned char *
snmp_oid_encode_oid(unsigned char *out, uint32_t *out_len, uint32_t *oid);

/**
 * @brief Decodes a Oid
 *
 * @param buf A pointer to the beginning of the buffer
 * @param buf_len A pointer to the buffer length
 * @param oid A pointer to the oid array
 * @param oid_len A pointer to the oid length
 *
 * @return NULL if error or the first entry after the oid in the buffer
 */
unsigned char *
snmp_oid_decode_oid(unsigned char *buf, uint32_t *buf_len, uint32_t *oid, uint32_t *oid_len);

/**
 * @brief Copies a Oid
 *
 * @param dst A pointer to the destination array
 * @param src A pointer to the source array
 */
void
snmp_oid_copy(uint32_t *dst, uint32_t *src);

#if LOG_LEVEL == LOG_LEVEL_DBG
/**
 * @brief Prints a oid
 *
 * @param oid A oid
 */
void
snmp_oid_print(uint32_t *oid);
#endif /* LOG_LEVEL == LOG_LEVEL_DBG */

#endif /* SNMP_OID_H_ */
/** @} */
