/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB
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
 *         Declarations for EDHOC message generators.
 * \author
 *         Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund, Marco Tiloca
 *         Christos Koulamas <cklm@isi.gr>, Niclas Finne <niclas.finne@ri.se>,
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

/**
 * \addtogroup edhoc
 * @{
 */

#ifndef EDHOC_MSG_GENERATORS_H_
#define EDHOC_MSG_GENERATORS_H_

#include "edhoc.h"
#include "edhoc-error.h"

/**
 * \brief Generate the EDHOC Message 1 and set it in the EDHOC context
 * \param ctx EDHOC Context struct
 * \param ad Application data to include in MSG1
 * \param ad_sz Application data length
 * \param suite_array If true, MSG1 includes an array of cipher suites when more than one is supported.
 *                    If false, MSG1 includes a single unsigned suite value regardless of the number
 *                    of suites supported by the initiator.
 * \return EDHOC_SUCCESS on success, error code on failure
 *
 * Composes EDHOC Message 1 as described in RFC9528 for EDHOC authentication with
 * asymmetric keys, encoded as a CBOR sequence in the MSG1 element of the context struct.
 * Uses the ephemeral key, cipher suite, and connection identifier already set in the
 * context (they are established by the caller, not by this function). Used by the
 * Initiator EDHOC role.
 *
 * - ctx->MSG1 = (METHOD:unsigned, SUITES_I, G_X, C_I_identifier)
 *
 */
edhoc_error_t edhoc_generate_message_1(edhoc_context_t *ctx, uint8_t *ad, size_t ad_sz, bool suite_array);

/**
 * \brief Generate the EDHOC Message 2 and set it in the EDHOC context
 * \param ctx EDHOC Context struct
 * \param auth_data Application data to include in MSG2
 * \param auth_data_size Application data length
 * \return EDHOC_SUCCESS on success, error code on failure
 *
 * Used by the EDHOC Responder role to generate message 2.
 * Computes the transcript hash TH_2 = H(ctx->MSG1, data_2),
 * computes MAC_2 (Message Authentication Code), computes CIPHERTEXT_2,
 * and composes EDHOC Message 2 as described in RFC9528
 * for EDHOC authentication with asymmetric keys, encoded as a CBOR sequence
 * in the MSG2 element of the context struct. Uses the ephemeral key and
 * connection identifier already set in the context.
 *
 * - ctx->MSG2 = a single CBOR byte string containing G_Y || CIPHERTEXT_2
 * - where: data_2 = G_Y (C_R is carried inside the plaintext of CIPHERTEXT_2, not in data_2)
 */
edhoc_error_t edhoc_generate_message_2(edhoc_context_t *ctx, const uint8_t *auth_data, size_t auth_data_size);

/**
 * \brief Generate the EDHOC Message 3 and set it in the EDHOC context
 * \param ctx EDHOC Context struct
 * \param auth_data Application data to include in MSG3
 * \param auth_data_size Application data length
 * \return EDHOC_SUCCESS on success, error code on failure
 *
 * Used by the EDHOC Initiator role to generate message 3.
 * Computes the transcript hash TH_3 = H(TH_2, PLAINTEXT_2, data_3),
 * computes MAC_3 (Message Authentication Code), computes CIPHERTEXT_3,
 * and composes EDHOC Message 3 as described in RFC9528
 * for EDHOC authentication with asymmetric keys, encoded as a CBOR sequence
 * in the MSG3 element of the context struct.
 *
 * - ctx->MSG3 = (data_3, CIPHERTEXT_3)
 * - where: data_3 = (?C_R_identifier)
 */
edhoc_error_t edhoc_generate_message_3(edhoc_context_t *ctx, const uint8_t *auth_data, size_t auth_data_size);

#endif /* EDHOC_MSG_GENERATORS_H_ */
/** @} */
