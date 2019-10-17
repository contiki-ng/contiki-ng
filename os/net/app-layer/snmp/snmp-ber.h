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

#ifndef SNMP_BER_H_
#define SNMP_BER_H_

#define BER_DATA_TYPE_INTEGER           0x02
#define BER_DATA_TYPE_OCTET_STRING      0x04
#define BER_DATA_TYPE_NULL              0x05
#define BER_DATA_TYPE_OID               0x06
#define BER_DATA_TYPE_SEQUENCE          0x30

/**
 * @brief Encodes a type
 *
 * @param out A pointer to the end of the buffer
 * @param out_len A pointer to the buffer length
 * @param type A type
 *
 * @return NULL if error or the next entry in the buffer
 */
unsigned char *
snmp_ber_encode_type(unsigned char *out, uint32_t *out_len, uint8_t type);

/**
 * @brief Encodes the length
 *
 * @param out A pointer to the end of the buffer
 * @param out_len A pointer to the buffer length
 * @param length A length
 *
 * @return NULL if error or the next entry in the buffer
 */
unsigned char *
snmp_ber_encode_length(unsigned char *out, uint32_t *out_len, uint8_t length);

/**
 * @brief Encodes an integer
 *
 * @param out A pointer to the end of the buffer
 * @param out_len A pointer to the buffer length
 * @param integer A integer
 *
 * @return NULL if error or the next entry in the buffer
 */
unsigned char *
snmp_ber_encode_integer(unsigned char *out, uint32_t *out_len, uint32_t integer);

/**
 * @brief Encodes an unsigned integer
 *
 * @param out A pointer to the end of the buffer
 * @param out_len A pointer to the buffer length
 * @param type A type that represents an unsigned integer
 * @param number A number
 *
 * @return NULL if error or the next entry in the buffer
 */
unsigned char *
snmp_ber_encode_unsigned_integer(unsigned char *out, uint32_t *out_len, uint8_t type, uint32_t number);

/**
 * @brief Encodes a string
 *
 * @param out A pointer to the end of the buffer
 * @param out_len A pointer to the buffer length
 * @param str A string
 * @param length The string length
 *
 * @return NULL if error or the next entry in the buffer
 */
unsigned char *
snmp_ber_encode_string_len(unsigned char *out, uint32_t *out_len, const char *str, uint32_t length);

/**
 * @brief Encodes a null
 *
 * @param out A pointer to the end of the buffer
 * @param out_len A pointer to the buffer length
 * @param type A type
 *
 * @return NULL if error or the next entry in the buffer
 */
unsigned char *
snmp_ber_encode_null(unsigned char *out, uint32_t *out_len, uint8_t type);

/**
 * @brief Decodes a type
 *
 * @param buff A pointer to the beginning of the buffer
 * @param buff_len A pointer to the buffer length
 * @param type A pointer to the type
 *
 * @return NULL if error or the first entry after the oid in the buffer
 */
unsigned char *
snmp_ber_decode_type(unsigned char *buff, uint32_t *buff_len, uint8_t *type);

/**
 * @brief Decodes a length
 *
 * @param buff A pointer to the beginning of the buffer
 * @param buff_len A pointer to the buffer length
 * @param length A pointer to the length
 *
 * @return NULL if error or the first entry after the oid in the buffer
 */
unsigned char *
snmp_ber_decode_length(unsigned char *buff, uint32_t *buff_len, uint8_t *length);

/**
 * @brief Decodes an integer
 *
 * @param buff A pointer to the beginning of the buffer
 * @param buff_len A pointer to the buffer length
 * @param integer A pointer to the integer
 *
 * @return NULL if error or the first entry after the oid in the buffer
 */
unsigned char *
snmp_ber_decode_integer(unsigned char *buff, uint32_t *buff_len, uint32_t *integer);

/**
 * @brief Decodes an unsigned number
 *
 * @param buff A pointer to the beginning of the buffer
 * @param buff_len A pointer to the buffer length
 * @param expected_type The expected type that represents an unsingned integer
 * @param number A pointer to the number
 *
 * @return NULL if error or the first entry after the oid in the buffer
 */
unsigned char *
snmp_ber_decode_unsigned_integer(unsigned char *buff, uint32_t *buff_len, uint8_t expected_type, uint32_t *number);

/**
 * @brief Decodes a string
 *
 * @param buff A pointer to the beginning of the buffer
 * @param buff_len A pointer to the buffer length
 * @param str A pointer to the string
 * @param length A pointer to the string length
 *
 * @return NULL if error or the first entry after the oid in the buffer
 */
unsigned char *
snmp_ber_decode_string_len_buffer(unsigned char *buff, uint32_t *buff_len, const char **str, uint32_t *length);

/**
 * @brief Decodes a null
 *
 * @param buff A pointer to the beginning of the buffer
 * @param buff_len A pointer to the buffer length
 *
 * @return NULL if error or the first entry after the oid in the buffer
 */
unsigned char *
snmp_ber_decode_null(unsigned char *buff, uint32_t *buff_len);

#endif /* SNMP_BER_H_ */
/** @} */
