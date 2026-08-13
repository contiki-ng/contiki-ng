/*
 * Copyright (c) 2025, RISE Research Institutes of Sweden AB.
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
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
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
 *      EDHOC tracing and logging header.
 * \author
 *      Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#ifndef EDHOC_TRACE_H_
#define EDHOC_TRACE_H_

#include "edhoc.h"
#include "sys/log.h"

/* Enhanced tracing macros aligned with RFC 9529 format */

/**
 * \brief Print protocol step header in RFC 9529 style
 * \param step_name The name of the protocol step (e.g., "message_1", "message_2")
 */
#define EDHOC_TRACE_STEP(step_name) \
  LOG_DBG("=== EDHOC %s ===\n", step_name)

/**
 * \brief Print cryptographic value in RFC 9529 trace format
 * \param label Descriptive label for the value
 * \param data Pointer to the data
 * \param len Length of the data in bytes
 */
#define EDHOC_TRACE_VALUE(label, data, len) do { \
  if(LOG_LEVEL >= LOG_LEVEL_DBG) { \
    LOG_DBG("%s (%zu bytes): ", label, (size_t)(len)); \
    if((len) > 0) { \
      LOG_DBG_BYTES(data, len); \
    } \
    LOG_DBG_("\n"); \
  } \
} while(0)

/**
 * \brief Print protocol state transition
 * \param from_state Previous state
 * \param to_state New state
 */
#define EDHOC_TRACE_STATE(from_state, to_state) \
  LOG_DBG("State transition: %s -> %s\n", from_state, to_state)

/**
 * \brief Print computation step description
 * \param description Brief description of the computation
 */
#define EDHOC_TRACE_COMPUTE(description) \
  LOG_DBG("Computing: %s\n", description)

/**
 * \brief Print detailed debug value (only at DBG level)
 * \param label Descriptive label
 * \param data Pointer to the data
 * \param len Length of the data
 */
#define EDHOC_DBG_VALUE(label, data, len) do { \
  if(LOG_LEVEL >= LOG_LEVEL_DBG) { \
    LOG_DBG("%s (%d bytes): ", label, (int)(len)); \
    if((len) > 0) { \
      LOG_DBG_BYTES(data, len); \
      LOG_DBG_("\n"); \
    } else { \
      LOG_DBG_("(empty)\n"); \
    } \
  } \
} while(0)

/* Function prototypes for enhanced tracing */

/**
 * \brief Print EDHOC message in RFC 9529 trace format
 * \param msg_num Message number (1, 2, 3, or 4)
 * \param msg_data Message data
 * \param msg_len Message length
 * \param is_tx True if transmitting, false if receiving
 */
void edhoc_trace_message(uint8_t msg_num, const uint8_t *msg_data, size_t msg_len, bool is_tx);

/**
 * \brief Print ephemeral key generation step
 * \param role_label Role label ("Initiator" or "Responder")
 * \param pub_x Public key X coordinate
 * \param pub_y Public key Y coordinate (optional, can be NULL)
 * \param priv Private key (optional for security, can be NULL)
 */
void edhoc_trace_ephemeral_key(const char *role_label,
                              const uint8_t *pub_x,
                              const uint8_t *pub_y,
                              const uint8_t *priv);

/**
 * \brief Print transcript hash computation
 * \param th_label TH label ("TH_2", "TH_3", "TH_4")
 * \param th_data Transcript hash data
 * \param input_data Input data for hash computation (optional)
 * \param input_len Input data length
 */
void edhoc_trace_transcript_hash(const char *th_label,
                                const uint8_t *th_data,
                                const uint8_t *input_data,
                                size_t input_len);

/**
 * \brief Print PRK derivation step
 * \param prk_label PRK label ("PRK_2e", "PRK_3e2m", "PRK_4e3m")
 * \param prk_data PRK data
 * \param salt_data Salt used (optional)
 * \param ikm_data Input keying material (optional)
 */
void edhoc_trace_prk_derivation(const char *prk_label,
                               const uint8_t *prk_data,
                               const uint8_t *salt_data,
                               const uint8_t *ikm_data);

/**
 * \brief Print MAC computation step
 * \param mac_label MAC label ("MAC_2", "MAC_3")
 * \param mac_data MAC data
 * \param context_data Context data used for MAC computation (optional)
 * \param context_len Context data length
 */
void edhoc_trace_mac_computation(const char *mac_label,
                                const uint8_t *mac_data,
                                const uint8_t *context_data,
                                size_t context_len);

/**
 * \brief Print credential information
 * \param cred_label Credential label ("CRED_I", "CRED_R")
 * \param cred_data Credential data
 * \param cred_len Credential length
 * \param id_cred_data ID_CRED data (optional)
 * \param id_cred_len ID_CRED length
 */
void edhoc_trace_credential(const char *cred_label,
                           const uint8_t *cred_data, size_t cred_len,
                           const uint8_t *id_cred_data, size_t id_cred_len);

/**
 * \brief Print session summary at protocol completion
 * \param ctx EDHOC context
 */
void edhoc_trace_session_summary(const edhoc_context_t *ctx);

#endif /* EDHOC_TRACE_H_ */
