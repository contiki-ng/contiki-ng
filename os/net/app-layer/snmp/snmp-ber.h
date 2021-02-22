/*
 * Copyright (C) 2019-2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 *      SNMP Implementation of the BER encoding
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

/**
 * \addtogroup snmp
 * @{
 */

#ifndef SNMP_BER_H_
#define SNMP_BER_H_

/**
 * \addtogroup SNMPInternal SNMP Internal API
 * @{
 *
 * This group contains all the functions that can be used inside the OS level.
 */

/**
 * \addtogroup SNMPBER SNMP BER
 * @{
 *
 * This group contains the BER implementation
 */

/**
 * @brief End-of-Content
 *
 */
#define BER_DATA_TYPE_EOC                   0x00

/**
 * @brief Integer
 *
 */
#define BER_DATA_TYPE_INTEGER               0x02

/**
 * @brief Octet String
 *
 */
#define BER_DATA_TYPE_OCTET_STRING          0x04

/**
 * @brief Null
 *
 */
#define BER_DATA_TYPE_NULL                  0x05

/**
 * @brief Object Identifier
 *
 */
#define BER_DATA_TYPE_OBJECT_IDENTIFIER     0x06

/**
 * @brief Sequence
 *
 */
#define BER_DATA_TYPE_SEQUENCE              0x30

/**
 * @brief TimeTicks
 *
 */
#define BER_DATA_TYPE_TIMETICKS             0x43

/**
 * @brief No Such Instance
 *
 */
#define BER_DATA_TYPE_NO_SUCH_INSTANCE      0x81

/**
 * @brief End of MIB View
 *
 */
#define BER_DATA_TYPE_END_OF_MIB_VIEW       0x82

/**
 * @brief PDU Get Request
 *
 */
#define BER_DATA_TYPE_PDU_GET_REQUEST       0xA0

/**
 * @brief PDU Get Next Request
 *
 */
#define BER_DATA_TYPE_PDU_GET_NEXT_REQUEST  0xA1

/**
 * @brief PDU Get Reponse
 *
 */
#define BER_DATA_TYPE_PDU_GET_RESPONSE      0xA2

/**
 * @brief PDU Set Request
 *
 */
#define BER_DATA_TYPE_PDU_SET_REQUEST       0xA3

/**
 * @brief PDU Trap
 *
 */
#define BER_DATA_TYPE_PDU_TRAP              0xA4

/**
 * @brief PDU Get Bulk
 *
 */
#define BER_DATA_TYPE_PDU_GET_BULK          0xA5

/**
 * @brief Encodes a type
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param type A type
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_encode_type(snmp_packet_t *snmp_packet, uint8_t type);

/**
 * @brief Encodes the length
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param length A length
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_encode_length(snmp_packet_t *snmp_packet, uint16_t length);

/**
 * @brief Encodes an integer
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param integer A integer
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_encode_integer(snmp_packet_t *snmp_packet, uint32_t integer);

/**
 * @brief Encodes a timeticks
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param timeticks A TimeTicks
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_encode_timeticks(snmp_packet_t *snmp_packet, uint32_t timeticks);

/**
 * @brief Encodes a string
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param str A string
 * @param length The string length
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_encode_string_len(snmp_packet_t *snmp_packet, const char *str, uint32_t length);

/**
 * @brief Encodes a Oid
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param oid A OID
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_encode_oid(snmp_packet_t *snmp_packet, snmp_oid_t *oid);

/**
 * @brief Encodes a null
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param type A type
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_encode_null(snmp_packet_t *snmp_packet, uint8_t type);

/**
 * @brief Decodes a type
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param type A pointer to the type
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_decode_type(snmp_packet_t *snmp_packet, uint8_t *type);

/**
 * @brief Decodes a length
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param length A pointer to the length
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_decode_length(snmp_packet_t *snmp_packet, uint8_t *length);

/**
 * @brief Decodes an integer
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param integer A pointer to the integer
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_decode_integer(snmp_packet_t *snmp_packet, uint32_t *integer);

/**
 * @brief Decodes a timeticks
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param timeticks A pointer to the timeticks
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_decode_timeticks(snmp_packet_t *snmp_packet, uint32_t *timeticks);

/**
 * @brief Decodes a string
 *
 * @param snmp_packet A pointer to the snmp packet
 * @param str A pointer to the string
 * @param length A pointer to the string length
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_decode_string_len_buffer(snmp_packet_t *snmp_packet, const char **str, uint32_t *length);

/**
 * @brief Decodes a null
 *
 * @param snmp_packet A pointer to the snmp packet
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_decode_null(snmp_packet_t *snmp_packet);

/**
 * @brief Decodes an OID
 *
 * @param snmp_packet  pointer to the snmp packet
 * @param oid A pointer to the OID
 *
 * @return 0 if error or 1 if success
 */
int
snmp_ber_decode_oid(snmp_packet_t *snmp_packet, snmp_oid_t *oid);

/** @} */

/** @} */

#endif /* SNMP_BER_H_ */

/** @} */
