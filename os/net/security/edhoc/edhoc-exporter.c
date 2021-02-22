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
 *         edhoc-export an implementation to export keys from the EDHOC shared secret
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 */

#include "edhoc-exporter.h"
#include "contiki-lib.h"

void
edhoc_exporter_print_oscore_ctx(oscore_ctx_t *osc)
{
  LOG_PRINT("Initiator client CID: 0x%02x\n", osc->client_ID);
  LOG_PRINT("Responder server CID: 0x%02x\n", osc->server_ID);
  LOG_PRINT("OSCORE Master Secret (%d bytes):", OSCORE_KEY_SZ);
  print_buff_8_print(osc->master_secret, OSCORE_KEY_SZ);
  LOG_PRINT("OSCORE Master Salt (%d bytes):", OSCORE_SALT_SZ);
  print_buff_8_print(osc->master_salt, OSCORE_SALT_SZ);
}
static int8_t
gen_th4(edhoc_context_t *ctx)
{
  uint8_t h4[MAX_BUFFER];
  uint8_t *ptr = h4;
  uint8_t h4_sz = cbor_put_bytes(&ptr, ctx->session.th.buf, ctx->session.th.len);
  h4_sz += cbor_put_bytes(&ptr, ctx->session.ciphertex_3.buf, ctx->session.ciphertex_3.len);
  LOG_INFO("Input to calculate TH_4 (CBOR Sequence) (%d bytes)",(int)h4_sz);
  print_buff_8_info(h4, h4_sz);
  uint8_t er = compute_TH(h4, h4_sz, ctx->session.th.buf, ctx->session.th.len);
  if(er != 0) {
    LOG_ERR("ERR COMPUTED TH4\n ");
    return ERR_CODE;
  }
  LOG_INFO("TH4 (%d bytes):",(int)ctx->session.th.len);
  print_buff_8_info(ctx->session.th.buf, ctx->session.th.len);
  return er;
}
int8_t
edhoc_exporter(uint8_t *result, edhoc_context_t *ctx, char *label, uint8_t label_sz, uint8_t lenght)
{
  int8_t er = edhoc_kdf(result, ctx->eph_key.prk_4x3m, ctx->session.th, label, label_sz, lenght);
  return er;
}
int8_t
edhoc_exporter_oscore(oscore_ctx_t *osc, edhoc_context_t *ctx)
{
  if(gen_th4(ctx) < 0) {
    LOG_ERR("error code at exporter(%d) \n ", ERR_CODE);
    return ERR_CODE;
  }

  /*The oscore client is the initiator */
  if(PART == PART_I) {
    osc->client_ID = ctx->session.cid;
    osc->server_ID = ctx->session.cid_rx;
  }
  if(PART == PART_R) {
    osc->client_ID = ctx->session.cid_rx;
    osc->server_ID = ctx->session.cid;
  }
  LOG_INFO("Info for OSCORE master secret:\n");
  int er1 = edhoc_exporter(osc->master_secret, ctx, "OSCORE Master Secret", strlen("OSCORE Master Secret"), OSCORE_KEY_SZ);
  if(er1 < 0) {
    return er1;
  }
  LOG_INFO("Info for OSCORE master salt:\n");
  er1 = edhoc_exporter(osc->master_salt, ctx, "OSCORE Master Salt", strlen("OSCORE Master Salt"), OSCORE_SALT_SZ);
  return er1;
}
int8_t
edhoc_exporter_psk_chaining(psk_ctx_t *psk, edhoc_context_t *ctx)
{
  gen_th4(ctx);
  int er1 = edhoc_exporter(psk->PSK, ctx, "EDHOC Chaining PSK", strlen("EDHOC Chaining PSK"), PSK_KEY_SZ);
  if(er1 < 0) {
    return er1;
  }
  er1 = edhoc_exporter(psk->kid_PSK, ctx, "EDHOC Chaining kid_psk", strlen("EDHOC Chaining kid_psk"), PSK_KEY_ID_SZ);
  return er1;
}
