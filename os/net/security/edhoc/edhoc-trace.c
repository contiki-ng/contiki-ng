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
 *      EDHOC tracing and logging module.
 * \author
 *      Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "edhoc-trace.h"
#include "edhoc-config.h"
#include "ecc-common.h"

#include "sys/log.h"
#define LOG_MODULE "EDHOC-Trace"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/*---------------------------------------------------------------------------*/
void
edhoc_trace_message(uint8_t msg_num,
                    const uint8_t *msg_data,
                    size_t msg_len,
                    bool is_tx)
{
  if(LOG_LEVEL >= LOG_LEVEL_DBG) {
    const char *direction = is_tx ? "TX" : "RX";
    LOG_DBG_("%s message_%d CBOR (%zu bytes): ", direction, msg_num, msg_len);
    if(msg_len > 0) {
      LOG_DBG_BYTES(msg_data, msg_len);
    }
    LOG_DBG_("\n");
  }
}
/*---------------------------------------------------------------------------*/
void
edhoc_trace_ephemeral_key(const char *role_label,
                         const uint8_t *pub_x,
                         const uint8_t *pub_y,
                         const uint8_t *priv)
{
  if(LOG_LEVEL >= LOG_LEVEL_DBG) {
    LOG_DBG("=== %s Ephemeral Key Generation ===\n", role_label);

    if(pub_x) {
      EDHOC_TRACE_VALUE("Ephemeral public key (x coordinate)",
                        pub_x, ECC_KEY_LEN);
    }

    if(pub_y) {
      EDHOC_TRACE_VALUE("Ephemeral public key (y coordinate)",
                        pub_y, ECC_KEY_LEN);
    }

    /* Only show the private key in RFC 9529 test-vector builds (EDHOC_TEST_VECTOR_TRACE_DH); hide it otherwise */
#if EDHOC_TEST == EDHOC_TEST_VECTOR_TRACE_DH
    if(priv) {
      EDHOC_TRACE_VALUE("Ephemeral private key (test vector)",
                        priv, ECC_KEY_LEN);
    }
#else
    if(priv) {
      LOG_DBG("Ephemeral private key: [hidden for security]\n");
    }
#endif
  }
}
/*---------------------------------------------------------------------------*/
void
edhoc_trace_transcript_hash(const char *th_label,
                           const uint8_t *th_data,
                           const uint8_t *input_data,
                           size_t input_len)
{
  if(LOG_LEVEL >= LOG_LEVEL_DBG) {
    LOG_DBG("=== Computing %s ===\n", th_label);

    if(input_data && input_len > 0) {
      EDHOC_TRACE_VALUE("Input to transcript hash (CBOR)",
                        input_data, input_len);
    }

    if(th_data) {
      EDHOC_TRACE_VALUE(th_label, th_data, HASH_LEN);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
edhoc_trace_prk_derivation(const char *prk_label,
                          const uint8_t *prk_data,
                          const uint8_t *salt_data,
                          const uint8_t *ikm_data)
{
  if(LOG_LEVEL >= LOG_LEVEL_DBG) {
    LOG_DBG("=== Deriving %s ===\n", prk_label);

    if(salt_data) {
      EDHOC_TRACE_VALUE("Salt", salt_data, HASH_LEN);
    }

    if(ikm_data) {
      EDHOC_TRACE_VALUE("Input Keying Material (IKM)", ikm_data, ECC_KEY_LEN);
    }

    if(prk_data) {
      EDHOC_TRACE_VALUE(prk_label, prk_data, HASH_LEN);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
edhoc_trace_mac_computation(const char *mac_label,
                           const uint8_t *mac_data,
                           const uint8_t *context_data,
                           size_t context_len)
{
  if(LOG_LEVEL >= LOG_LEVEL_DBG) {
    LOG_DBG("=== Computing %s ===\n", mac_label);

    if(context_data && context_len > 0) {
      EDHOC_TRACE_VALUE("MAC computation context", context_data, context_len);
    }

    if(mac_data) {
      EDHOC_TRACE_VALUE(mac_label, mac_data, HASH_LEN);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
edhoc_trace_credential(const char *cred_label,
                      const uint8_t *cred_data,
                      size_t cred_len,
                      const uint8_t *id_cred_data,
                      size_t id_cred_len)
{
  if(LOG_LEVEL >= LOG_LEVEL_DBG) {
    LOG_DBG("=== %s Authentication ===\n", cred_label);

    if(id_cred_data && id_cred_len > 0) {
      char id_label[32];
      snprintf(id_label, sizeof(id_label), "ID_%s", cred_label);
      EDHOC_TRACE_VALUE(id_label, id_cred_data, id_cred_len);
    }

    if(cred_data && cred_len > 0) {
      EDHOC_TRACE_VALUE(cred_label, cred_data, cred_len);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
edhoc_trace_session_summary(const edhoc_context_t *ctx)
{
  if(!ctx || LOG_LEVEL < LOG_LEVEL_DBG) {
    return;
  }

  LOG_DBG("=== EDHOC Session Summary ===\n");
  LOG_DBG("Protocol Role: %s\n",
           ctx->config.role == EDHOC_INITIATOR ? "Initiator" : "Responder");
  LOG_DBG("Method: %d\n", ctx->config.method);
  LOG_DBG("Cipher Suite: %d\n", ctx->state.suite_selected);

  if(ctx->state.cid_len > 0) {
    EDHOC_TRACE_VALUE("Local Connection ID",
                      ctx->state.cid, ctx->state.cid_len);
  }

  if(ctx->state.cid_rx_len > 0) {
    EDHOC_TRACE_VALUE("Peer Connection ID",
                      ctx->state.cid_rx, ctx->state.cid_rx_len);
  }

  LOG_DBG("Test Vector Mode: %s\n",
           EDHOC_TEST == EDHOC_TEST_VECTOR_TRACE_DH ? "RFC 9529" : "Production");

  LOG_DBG("Session Status: Ready for key export\n");
}
