/*
 * Copyright (c) 2020, Industrial Systems Institute (ISI), Patras, Greece
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
 */

/**
 * \file
 *         An implementation of Ephemeral Diffie-Hellman Over COSE (EDHOC)
 *         (draft-ietf-lake-edhoc-01)
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 *         Christos Koulamas <cklm@isi.gr>
 */

/**
 * \defgroup edhoc An EDHOC implementation (draft-selander-lake-edhoc-01)
 * @{
 *
 * This is an implementation of the  Ephemeral Diffie-Hellman Over COSE (EDHOC)
 * a very compact, and lightweight authenticated Diffie-Hellman key exchange with
 * ephemeral keys that provide mutual authentication, perfect forward secrecy,
 * and identity protection as described in (draft-selander-lake-edhoc-01)
 *
 **/

#ifndef _EDHOC_H_
#define _EDHOC_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "ecdh.h"
#include "edhoc-msgs.h"

/**
 * \brief The max size of the buffers
 */
#define MAX_BUFFER 256

/**
 * \brief The max size of the EDHOC msg, as CoAP payload
 */
/*#ifdef EDHOC_CONF_MAX_PAYLOAD
 #define MAX_PAYLOAD EDHOC_CONF_MAX_PAYLOAD
 #else
 #define MAX_PAYLOAD 64
 #endif*/
#ifndef EDHOC_CID
#define EDHOC_CID 0x1
#endif
/**
 * \brief MAC length
 */
#define MAC_LEN 8

/**
 * \brief EDHOC session struct
 */
typedef struct edhoc_session {
  uint8_t part;
  uint8_t method;
  uint8_t suit[5];
  uint8_t suit_num;
  uint8_t suit_rx;
  bstr Gx;
  int cid;
  int cid_rx;
  bstr id_cred_x;
  bstr cred_x;
  bstr th;
  bstr ciphertex_2;
  bstr ciphertex_3;
} edhoc_session;

/**
 * \brief EDHOC context struct
 */
typedef struct edhoc_context_t {
  ecc_key authen_key;
  ecc_key ephimeral_key;
  session_key eph_key;
  edhoc_session session;
  ecc_curve_t curve;
  uint8_t msg_rx[MAX_PAYLOAD];
  uint8_t msg_tx[MAX_PAYLOAD];
  uint16_t rx_sz;
  uint16_t tx_sz;
} edhoc_context_t;

/**
 * \brief Reserve memory for the edhoc context struct
 *
 * Used by both Initiator and Responder EDHOC parts to reserve memory
 */
void  edhoc_storage_init(void);

/**
 * \brief Create a new edhoc context
 * \relates edhoc_storage_init
 * \return edhoc_ctx_t EDHOC context struct
 *
 * Used by both Initiator and Responder EDHOC parts to create a news edhoc context
 * and allocate at the memory reserved before with the edhoc_storage_init function
 */
edhoc_context_t *edhoc_new();

/**
 * \brief Initialize the EDHOC ctx with the define edhoc parameters
 * \return edhoc_ctx_t EDHOC context struct
 *
 * Used in the edho_new to set the default protocol definitions and in the Responder to
 * reset the initial values to prepare for a newd edhoc connection
 */

void edhoc_init(edhoc_context_t *ctx);

/**
 * \brief Close the edhoc context
 * \param ctx EDHOC context struct
 *
 * Used by both Initiator and Responder EDHOC parts to de-allocate the memory reserved
 * for the EDHOC context when EDHOC protocol finalize.
 */
void edhoc_finalize(edhoc_context_t *ctx);

/**
 * \brief Generate the EDHOC Message 1 and set it on the EDHOC context
 * \param ctx EDHOC Context struct
 * \param ad Application data to include in MSG1
 * \param ad_sz Application data lenght
 * \param suit_array If true the msg1 include an array of suits if have more than one suit if 0 msg1 inclue an 
 * unique unsigned suit independently of the number of suits supported by the initiator 
 *
 * Generate an ephemeral ECDH key pair, determinate the cipher suite to use and the
 * connection identifier. Compose the EDHOC Message 1 as describe the (draft-selander-lake-edhoc-01) reference
 * for EHDOC Authentication with Asymmetric Keys and encoded as a CBOR sequence in the MSG1 elemnent of the
 * ctx struct.
 * It is used by Initiator EDHOC part.
 *
 * - ctx->MSG1 = (METHOD_CORR:unsigned, SUITES_I:unisgned, G_X:bstr, C_I:bstr_identifier)
 *
 */
void edhoc_gen_msg_1(edhoc_context_t *ctx, uint8_t *ad, size_t ad_sz,bool suit_array);

/**
 * \brief Generate the EDHOC Message 2 and set it on the EDHOC ctx
 * \param ctx EDHOC Context struct
 * \param ad Aplication data to include in MSG2
 * \param ad_sz Aplication data lenght
 *
 * It is used by EDHOC Responder part to processing the message 2
 * Generate an ephemeral ECDH key pair
 * Choose a connection identifier,
 * Compute the trancript hash 2 TH2 = H(ctx->MSG1, data_2)
 * Compute MAC_2 (Message Authentictaion Code)
 * Compute CIPHERTEXT_2
 * Compose the EDHOC Message 2 as describe the (draft-selander-lake-edhoc-01) reference
 * for EHDOC Authentication with Asymetric Keys and encoded as a CBOR sequence in the MSG2 elemnent of the
 * ctx struct.
 * - ctx->MSG2 = (data_2, CIPHERTEXT_2:bstr)
 * - where: data_2 = (?C_I:bstr_identifier, G_Y:bstr)
 */
void edhoc_gen_msg_2(edhoc_context_t *ctx, uint8_t *ad, size_t ad_sz);

/**
 * \brief Generate the EDHOC Message 3 and set it on the EDHOC ctx
 * \param ctx EDHOC Context struct
 * \param ad Aplication data to include in MSG3
 * \param ad_sz Aplication data lenght
 *
 * It is used by EDHOC Initiator part to processing the message 3.
 * Compute the transcript hash 3 TH3 = H(TH_2, CIPHERTEXT_2, data_3)
 * Compute MAC_3 (Message Authentictaion Code)
 * Compute CIPHERTEXT_3
 * Compose the EDHOC Message 3 as describe the (draft-selander-lake-edhoc-01) reference
 * for EHDOC Authentication with Asymmetric Keys and encoded as a CBOR sequence in the MSG3 elemnent of the
 * ctx struct.
 *
 * - ctx->MSG3 = (data_3, CIPHERTEXT_3:bstr)
 * - where: data_3 = (?C_R:bstr_identifier)
 */
void edhoc_gen_msg_3(edhoc_context_t *ctx, uint8_t *ad, size_t ad_sz);

/**
 * \brief Generate the EDHOC ERROR Message
 * \param msg_er A pointer to a buffer to copy the generated CBOR message error
 * \param ctx EDHOC Context struct
 * \param err EDHOC error number
 * \return err_sz CBOR Message Error size
 *
 * An EDHOC error message can be sent by both parties as a reply to any non-error
 * EDHOC message. If any verification step fails on the EDHOC protocol the Initiator
 * or Responder must send an EDHOC error message back that contains a brief human-readable
 * diagnostic message.
 * - msg_er = (?C_x:bstr_identifier, ERR_MSG:tstr)
 */
uint8_t edhoc_gen_msg_error(uint8_t *msg_er, edhoc_context_t *ctx, int8_t err);

/**
 * \brief Get the authnetication key from the rx msg
 * \param ctx EDHOC Context struct
 * \param pt A pointer to the ID_CRED_X on the Rx msg buffer.
 * \param key A pointer to a cose key struct
 * \retval ERR_CODE when an EDHOC ERROR is detected return a negative number correspondig to the specific error code
 * \retval 1 when EDHOC get success the authentication key
 *
 * Used by Initiator and Responder EDHOC part to get the authentication key from the received msg.
 *
 * If any verification step fails to return an EDHOC ERROR code and, if all the steps success return 1.
 */
int edhoc_get_auth_key(edhoc_context_t *ctx, uint8_t **pt, cose_key_t *key);

/**
 * \brief Authneticate the rx message
 * \param ctx EDHOC Context struct
 * \param pt A pointer to the SIGN on the Rx msg buffer.
 * \param cipher_len Lenght of the cipher msg
 * \param ad A pointer to a buffer to copy the Application Data of the rx message
 * \param key The other party authentication key, used for authentication
 * \retval ERR_CODE when an EDHOC ERROR is detected return a negative number correspondig to the specific error code
 * \retval ad_sz The length of the Application Data received in Message 2, when EDHOC success
 *
 * Used by Initiator and Responder EDHOC part to Authenticate the other party
 * - Verify that the EDHOC Responder part identity is among the allower if it is necessary
 * - Verify MAC
 * - Pass Application data AD
 *
 * If any verification step fails to return an EDHOC ERROR code and, if all the steps success
 * the length of the Application Data receive on the Message is returned.
 */
int edhoc_authenticate_msg(edhoc_context_t *ctx, uint8_t **pt, uint8_t cipher_len, uint8_t *ad, cose_key_t *key);

/**
 * \brief Handle the EDHOC Message 1 received
 * \param ctx EDHOC Context struct
 * \param buffer A pointer to the buffer containing the EDHOC message received
 * \param buff_sz Size of the EDHOC message received
 * \param ad A pointer to a buffer to copy the Application Data received in Message 1
 * \retval negative number (EDHOC ERROR CODES) when an EDHOC ERROR is detected
 * \retval ad_sz The length of the Application Data received in Message 1, when EDHOC success
 *
 * Used by Responder EDHOC part to process the Message 1 receive
 * - Decode the message 1
 * - Verify that cypher suite
 * - Pass Application data AD_1
 * - If any verification step fails to return an EDHOC ERROR code and, if all the steps success
 * - the length of the Application Data receive on the Message 1 is returned.
 */
int edhoc_handler_msg_1(edhoc_context_t *ctx, uint8_t *buffer, size_t buff_sz, uint8_t *ad);

/**
 * \brief Handle the EDHOC Message 2 received
 * \param msg2 A pointer to the buffer containing the received EDHOC message 2
 * \param ctx EDHOC Context struct
 * \param buffer A pointer to the buffer containing the EDHOC message received
 * \param buff_sz Size of the EDHOC message received
 * \retval ERR_CODE when an EDHOC ERROR is detected return a negative number correspondig to the specific error code
 * \retval 1 when EDHOC decode and verify success
 *
 * Used by Initiator EDHOC part to process the Message 2 receive
 * - Decode the message 2
 * - Verify the other par trough 5-tuple IP address and/or connection identifier C_I
 *
 * If any verification step fails to return an EDHOC ERROR code and, if all the steps success return 1.
 */
int edhoc_handler_msg_2(edhoc_msg_2 *msg2, edhoc_context_t *ctx, uint8_t *buffer, size_t buff_sz);

/**
 * \brief Handle the EDHOC Message 3 received
 * \param msg3 A pointer to the buffer containing the received EDHOC message 3
 * \param ctx EDHOC Context struct
 * \param buffer A pointer to the buffer containing the EDHOC message received
 * \param buff_sz Size of the EDHOC message received
 * \retval negative number (EDHOC ERROR CODE) when an EDHOC ERROR is detected
 * \retval 1 when EDHOC decode and verify success
 *
 * Used by Responder EDHOC part to process the Message 3 receive
 * - Decode the message 3
 * - Verify the other par trough 5-tuple IP address and/or connection identifier C_R
 *
 * If any verification step fails to return an EDHOC ERROR code and,if all the steps success return 1.
 */
int edhoc_handler_msg_3(edhoc_msg_3 *msg3, edhoc_context_t *ctx, uint8_t *buffer, size_t buff_sz);

/**
 * \brief HMAC-based Expand Key derivation function (RFC 5869) on the EDHOC context
 * \param result OKM (Output Keying Material)
 * \param key PRK A pseudorandom key of at least HAS_LENGHT bites
 * \param th Transcription Hash to generate the CBOR info input of hmac_expand
 * \param label Label to generate the CBOR info input of hmac_expand
 * \param label_sz Label length to generate the info input of hmac_expand
 * \param lenght of OKM in bites
 * \return 1 if HKDF-expand finish successfully
 *
 * Used by both Initiator and Responder EDHOC parts to generate the info parameter
 * of the HKDF-Expand function and the HKDF-Expand function as well.
 *  - OKM = HKDF-Expand(PRK, info, lenght)
 *  - where info: info = [ALGORITHM_ID:unsigned, th:bstr, label:tstr, length:uint]
 */
int16_t edhoc_kdf(uint8_t *result, uint8_t *key, bstr th, char *label, uint16_t label_sz, uint16_t lenght);

/**
 * \brief Get the SH-Static authentication pair key from the storage and set in the EDHOC context
 * \param ctx EDHOC Context struct
 *
 * Used by both Initiator and Responder EDHOC parts to set the EDHOC context with their
 * own authentication key from the EDHOC key storage. The authentication keys must be
 * established at the EDHOC key storage before running the EDHOC protocol.
 */
uint8_t edhoc_get_authentication_key(edhoc_context_t *ctx);

#endif /* _EDHOC_H_ */
/** @} */