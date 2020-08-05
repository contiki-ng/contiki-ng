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
 *         EDHOC, an implementation of Ephimeral Diffie-Hellman Over COSE (EDHOC) (IETF draft-ietf-lake-edhoc-01)
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 */
#include "edhoc.h"
#include "contiki-lib.h"
#include "edhoc-config.h"

static uint8_t buf[MAX_BUFFER];
static uint8_t inf[MAX_BUFFER];
static uint8_t cred_x[128];
static uint8_t id_cred_x[128];
static uint8_t mac[MAC_LEN];

MEMB(edhoc_context_storage, edhoc_context_t, 1);

static inline edhoc_context_t *
context_new()
{
  return (edhoc_context_t *)memb_alloc(&edhoc_context_storage);
}
static inline void
context_free(edhoc_context_t *ctx)
{
  memb_free(&edhoc_context_storage, ctx);
}
void
edhoc_storage_init(void)
{
  memb_init(&edhoc_context_storage);
}
static void
init(edhoc_context_t *ctx)
{
  /*generate original ephimeral key */
  LOG_DBG("generate key\n");
  ctx->curve.curve = uECC_secp256r1();
  watchdog_periodic();
  generate_key(&ctx->ephimeral_key, ctx->curve);
  watchdog_periodic();
  /*TO DO : check that the key is belown to the curve */

  LOG_DBG("generate session cid\n");
  /*generate session id and update */
  ctx->session.cid = (uint8_t)random_rand() % ((64 + 1) - 25) + 25; /* the cid number can  be between 25 and 64 */
  ctx->session.suit = SUIT;
  LOG_DBG("Set part\n");
  ctx->session.part = PART;  /*iniciator I (U) or receiver (V) */
  ctx->session.method = (4 * METHOD) + CORR; /*the method we use is: 4*METHOD+CORR */
}
edhoc_context_t *
edhoc_new()
{
  edhoc_context_t *ctx;
  ctx = context_new();
  if(ctx) {
    init(ctx);
  }
  return ctx;
}
void
edhoc_finalize(edhoc_context_t *ctx)
{
  context_free(ctx);
}
static size_t
generate_cred_x(cose_key *cose, uint8_t *cred)
{
  size_t size = 0;
  if(cose->crv == 1) {
    size += cbor_put_map(&cred, 5);
  } else {
    size += cbor_put_map(&cred, 4);
  }

  size += cbor_put_unsigned(&cred, 1);
  size += cbor_put_unsigned(&cred, cose->kty);
  size += cbor_put_negative(&cred, 1);
  size += cbor_put_unsigned(&cred, cose->crv);
  size += cbor_put_negative(&cred, 2);
  size += cbor_put_bytes(&cred, cose->x.buf, cose->x.len);
  if(cose->crv == 1) {
    size += cbor_put_negative(&cred, 3);
    size += cbor_put_bytes(&cred, cose->y.buf, cose->y.len);
  }
  size += cbor_put_text(&cred, "subject name", strlen("subject name"));
  size += cbor_put_text(&cred, cose->identity.buf, cose->identity.len);
  return size;
}
/*TODO : Check the correct id_cred selection */
static size_t
generate_id_cred_x(cose_key *cose, uint8_t *cred) /*TO DO. Not sure of the cred id formation for our case */
{
  size_t size = 0;
  if(AUTHENT_TYPE == PRKI) {
    size += cbor_put_map(&cred, 2);
    size += cbor_put_unsigned(&cred, 32);
    size += cbor_put_bytes(&cred, cose->x.buf, cose->x.len);
    size += cbor_put_text(&cred, "subject name", strlen("subject name"));
    size += cbor_put_text(&cred, cose->identity.buf, cose->identity.len);
  }
  if(AUTHENT_TYPE == PRK_ID) {
    size += cbor_put_map(&cred, 1);
    size += cbor_put_unsigned(&cred, 4);
    size += cbor_put_bytes(&cred, cose->kid.buf, cose->kid.len);
  }

  return size;
}
static size_t
generate_info(uint8_t *info, uint8_t *th, uint8_t th_sz, char *label, uint8_t label_sz, uint8_t lenght)
{
  size_t size = 0;
  size = cbor_put_array(&info, 4);
  size += cbor_put_unsigned(&info, ALGORITHM_ID);
  size += cbor_put_bytes(&info, th, th_sz);
  size += cbor_put_text(&info, label, label_sz);
  size += cbor_put_unsigned(&info, lenght);
  return size;
}
static int8_t
set_rx_cid(edhoc_context_t *ctx, uint8_t *cidrx, uint8_t cidrx_sz)
{
  /*set conector id from rx */
  if(cidrx[0] == ctx->session.cid) {
    LOG_ERR("error code (%d)\n ", ERR_CID_NOT_VALID);
    while(ctx->session.cid == cidrx[0])
      ctx->session.cid = (uint8_t)random_rand() % ((64 + 1) - 25) + 25; /* the cid number can  be between 25 and 64 */
    LOG_DBG("set a new CID different from the rx value");
  }
  if(cidrx_sz == 1) {
    ctx->session.cid_rx = cidrx[0] + 24;

  } else {
    LOG_ERR("error code (%d)\n ", ERR_MSG_MALFORMED);
    return ERR_MSG_MALFORMED;
  }
  return 0;
}
static int8_t
check_rx_suit(edhoc_context_t *ctx, uint8_t suitrx)
{
  /*Verify the selected cipher suit */
  if(suitrx != SUIT) {
    LOG_ERR("error code (%d)\n ", ERR_SUIT_NON_SUPPORT);
    return ERR_SUIT_NON_SUPPORT;
  }
  ctx->session.suit_rx = suitrx;
  return 0;
}
static void
set_rx_gx(edhoc_context_t *ctx, uint8_t *gx)
{
  memcpy(ctx->eph_key.gx, gx, ECC_KEY_BYTE_LENGHT + 1);
  ctx->session.Gx = (bstr){(uint8_t *)ctx->eph_key.gx, ECC_KEY_BYTE_LENGHT + 1 };
}
static int8_t
set_rx_method(edhoc_context_t *ctx, uint8_t method)
{
  /*TO DO: Support more than one method */
  if(method != ((4 * METHOD) + CORR)) {
    LOG_ERR("error code (%d)\n ", ERR_REJECT_METHOD);
    return ERR_REJECT_METHOD;
  }
  ctx->session.method = method;
  return 0;
}
static void
set_rx_msg(edhoc_context_t *ctx, uint8_t *msg, uint8_t msg_sz)
{
  memcpy(ctx->msg_rx, msg, msg_sz);
  ctx->rx_sz = msg_sz;
}
static void
print_connection(edhoc_session *con)
{
  LOG_DBG("Conection print\n");
  LOG_DBG("connectin part: %d\n", con->part);
  LOG_DBG("connectin method: %d\n", con->method);
  LOG_DBG("My suit: %d\n", con->suit);
  LOG_DBG("Other part suit: %d\n", con->suit_rx);
  LOG_DBG("My cID: %x\n", con->cid);
  LOG_DBG("Other part cID: %x\n", con->cid_rx);
  LOG_DBG("Gx:");
  print_buff_8_dbg(con->Gx.buf, con->Gx.len);
}
static size_t
set_data_2(edhoc_context_t *ctx)
{
  /* Genereta data for MSG2 */
  edhoc_data_2 data;
  uint8_t var = TYPE_CON % 4;
  if((var == 3) || (var == 1)) {
    data.Ci = (bstr){NULL, 0 };
  } else {
    data.Ci = (bstr){&ctx->session.cid_rx, 1 };
  }
  /*Put X cordenate at data */
  data.Gy = (bstr){(uint8_t *)&ctx->ephimeral_key.public.x, ECC_KEY_BYTE_LENGHT + 1 };
  data.Cr = (bstr){&ctx->session.cid, 1 };
  size_t data_buff_sz = edhoc_serialize_data_2(&data, ctx->msg_tx);
  return data_buff_sz;
}
static size_t
set_data_3(edhoc_context_t *ctx)
{
  edhoc_data_3 data;
  uint8_t var = TYPE_CON % 4;
  if((var == 2) || (var == 3)) {
    data.Cr = (bstr){NULL, 0 };
  } else {
    data.Cr = (bstr){&ctx->session.cid_rx, 1 };
  }
  size_t data_buff_sz = edhoc_serialize_data_3(&data, ctx->msg_tx);
  return data_buff_sz;
}
static uint8_t
gen_th2(edhoc_context_t *ctx, uint8_t *data, uint16_t data_sz, uint8_t *msg, uint16_t msg_sz)
{
  /*Create the input for TH2 = H(msg1,data_2) msg1 is in msg_rx */
  uint8_t h2_sz = msg_sz + data_sz;
  uint8_t h2[h2_sz];
  memcpy(h2, msg, msg_sz);
  memcpy((h2 + msg_sz), data, data_sz);

  /*Compute TH */
  uint8_t er = compute_TH(h2, h2_sz, ctx->session.th.buf, ctx->session.th.len);
  if(er != 0) {
    LOG_ERR("ERR COMPUTED TH2\n ");
    return ERR_CODE;
  }
  return 0;
}
static uint8_t
gen_th3(edhoc_context_t *ctx, uint8_t *data, uint16_t data_sz, uint8_t *ciphertext, uint16_t ciphertext_sz)
{
  uint8_t h[MAX_BUFFER];
  uint8_t *ptr = h;
  uint16_t h_sz = cbor_put_bytes(&ptr, ctx->session.th.buf, ctx->session.th.len);
  h_sz += cbor_put_bytes(&ptr, ciphertext, ciphertext_sz);
  memcpy(h + h_sz, data, data_sz);
  h_sz += data_sz;

  /*Compute TH */
  uint8_t er = compute_TH(h, h_sz, ctx->session.th.buf, ctx->session.th.len);
  if(er != 0) {
    LOG_ERR("ERR COMPUTED TH3\n ");
    return ERR_CODE;
  }
  return 0;
}
int16_t
edhoc_kdf(uint8_t *result, uint8_t *key, bstr th, char *label, uint16_t label_sz, uint16_t lenght)
{
  /* generate info for K */
  uint16_t info_sz = generate_info(inf, th.buf, th.len, label, label_sz, lenght);
  int16_t er = hkdf_expand(key, ECC_KEY_BYTE_LENGHT, inf, info_sz, result, lenght);
  if (er < 0)
    return er;
  return lenght;
}
static uint8_t
set_mac(cose_encrypt0 *cose, edhoc_context_t *ctx, uint8_t *ad, uint16_t ad_sz, uint8_t mac_num)
{
  /*CBOR The TH2 */
  cose_encrypt0_set_content(cose, NULL, 0, NULL, 0);
  uint8_t th_cbor[ECC_KEY_BYTE_LENGHT + 2];
  uint8_t *th_ptr = th_cbor;
  size_t th_cbor_sz = cbor_put_bytes(&th_ptr, ctx->session.th.buf, ctx->session.th.len);
  /*COSE encrypt0 set external aad */
  cose->external_aad_sz = th_cbor_sz + ctx->session.cred_x.len + ad_sz;
  memcpy(cose->external_aad, th_cbor, th_cbor_sz);
  memcpy((cose->external_aad + th_cbor_sz), ctx->session.cred_x.buf, ctx->session.cred_x.len);
  memcpy((cose->external_aad + th_cbor_sz + ctx->session.cred_x.len), ad, ad_sz);

  if(mac_num == 2) {
    edhoc_kdf(cose->key, ctx->eph_key.prk_3e2m, ctx->session.th, "K_2m", strlen("K_2m"), KEY_DATA_LENGHT);
    edhoc_kdf(cose->nonce, ctx->eph_key.prk_3e2m, ctx->session.th, "IV_2m", strlen("IV_2m"), IV_LENGHT);
  } else if(mac_num == 3) {
    edhoc_kdf(cose->key, ctx->eph_key.prk_4x3m, ctx->session.th, "K_3m", strlen("K_3m"), KEY_DATA_LENGHT);
    edhoc_kdf(cose->nonce, ctx->eph_key.prk_4x3m, ctx->session.th, "IV_3m", strlen("IV_3m"), IV_LENGHT);
  } else {
    LOG_ERR("Wrong MAC number\n");
    return 0;
  }

  cose->key_sz = KEY_DATA_LENGHT;
  cose->nonce_sz = IV_LENGHT;

  /* COSE encrypt0 set header */
  cose_encrypt0_set_header(cose, ctx->session.id_cred_x.buf, ctx->session.id_cred_x.len, NULL, 0);
  return 1;
}
static uint8_t
gen_mac_dh(edhoc_context_t *ctx, uint8_t *ad, uint16_t ad_sz, uint8_t *mac, uint8_t mac_num)
{
  cose_encrypt0 *cose = cose_encrypt0_new();
  if(!set_mac(cose, ctx, ad, ad_sz, mac_num)) {
    LOG_ERR("Set MAC error\n");
    return 0;
  }
  uint8_t mac_sz = cose_encrypt(cose);
  for(int i = 0; i < mac_sz; i++) {
    mac[i] = cose->ciphertext[i];
  }
  cose_encrypt0_finalize(cose);
  return mac_sz;
}
static uint16_t
check_mac_dh(edhoc_context_t *ctx, uint8_t *ad, uint16_t ad_sz, uint8_t *cipher, uint16_t cipher_sz, uint8_t mac_num)
{
  LOG_DBG("AD (%d):", ad_sz);
  print_char_8_dbg((char *)ad, ad_sz);
  cose_encrypt0 *cose = cose_encrypt0_new();
  if(!set_mac(cose, ctx, ad, ad_sz, mac_num)) {
    LOG_ERR("Set MAC error\n");
    return 0;
  }
  cose_encrypt0_set_ciphertext(cose, cipher, cipher_sz);
  uint16_t mac_sz = cose_decrypt(cose);
  if(mac_sz == 0) {
    LOG_ERR("error code in check mac (%d)\n ", ERR_AUTHENTICATION);
    return 0;
  }
  cose_encrypt0_finalize(cose);
  return mac_sz;
}

static uint8_t
gen_gxy(edhoc_context_t *ctx)
{
  uint8_t er = generate_IKM(ctx->eph_key.gx, ctx->ephimeral_key.private_key, buf, ctx->curve);
  LOG_DBG("gen_IKM\n");
  if(er == 0) {
    LOG_ERR("error in generate shared secret\n ");
    return 0;
  }
  LOG_DBG("GXY: ");
  print_buff_8_dbg(buf, ECC_KEY_BYTE_LENGHT);
  return 1;
}
static uint8_t
gen_prk_2e(edhoc_context_t *ctx)
{
  uint8_t er = 0;
  LOG_DBG("gen_gxy\n");
  watchdog_periodic();
  er = gen_gxy(ctx);
  watchdog_periodic();
  if(er == 0) {
    return 0;
  }
  LOG_DBG("hkdf_extract\n");
  er = hkdf_extrac(NULL, 0, buf, ECC_KEY_BYTE_LENGHT, ctx->eph_key.prk_2e);
  if(er < 1) {
    LOG_ERR("Error in extarct prk_2e\n");
    return 0;
  }
  LOG_DBG("PRK_2e: ");
  print_buff_8_dbg(ctx->eph_key.prk_2e, ECC_KEY_BYTE_LENGHT);
  return 1;
}
/*use PRK_2e */
static uint8_t
gen_k_2e(edhoc_context_t *ctx, uint16_t lenght)
{
  uint16_t info_sz = generate_info(inf, ctx->session.th.buf, ctx->session.th.len, "K_2e", strlen("K_2e"), lenght);
  if(info_sz == 0) {
    LOG_ERR("Error in generate info for k_2e\n");
    return 0;
  }
  LOG_DBG("gen_k_2e info: ");
  print_buff_8_dbg(inf, info_sz);
  int8_t er = hkdf_expand(ctx->eph_key.prk_2e, ECC_KEY_BYTE_LENGHT, inf, info_sz, ctx->eph_key.k2_e, lenght);
  if(er < 1) {
    LOG_ERR("Error in expand k_2e\n");
    return 0;
  }
  LOG_DBG("gen_k_2e: ");
  print_buff_8_dbg(ctx->eph_key.k2_e, lenght);

  return 1;
}
/*TO DO: change the gen with the PART: Initiator U: gen = 0; Responder V: gen = 1; */
static uint8_t
gen_prk_3e2m(edhoc_context_t *ctx, ecc_key *key_authenticate, uint8_t gen)
{
  uint8_t grx[ECC_KEY_BYTE_LENGHT];
  int8_t er = 0;
  if(gen) {
    er = generate_IKM(ctx->eph_key.gx, key_authenticate->private_key, grx, ctx->curve);
  } else {
    er = generate_IKM(key_authenticate->public.x, ctx->ephimeral_key.private_key, grx, ctx->curve);
  }
  if(er == 0) {
    LOG_ERR("error in generate shared secret for prk_3e2m\n ");
    return 0;
  }
  LOG_DBG("GRX: ");
  print_buff_8_dbg(grx, ECC_KEY_BYTE_LENGHT);
  er = hkdf_extrac(ctx->eph_key.prk_2e, ECC_KEY_BYTE_LENGHT, grx, ECC_KEY_BYTE_LENGHT, ctx->eph_key.prk_3e2m);
  if(er < 1) {
    LOG_ERR("error in extact for prk_3e2m\n");
    return 0;
  }
  LOG_DBG("PRK_3e2m: ");
  print_buff_8_dbg(ctx->eph_key.prk_3e2m, ECC_KEY_BYTE_LENGHT);
  return 1;
}
static uint8_t
gen_prk_4x3m(edhoc_context_t *ctx, ecc_key *key_authenticate, uint8_t gen)
{
  uint8_t giy[ECC_KEY_BYTE_LENGHT];
  int8_t er = 0;
  if(gen) {
    er = generate_IKM(key_authenticate->public.x, ctx->ephimeral_key.private_key, giy, ctx->curve);
  } else {
    er = generate_IKM(ctx->eph_key.gx, key_authenticate->private_key, giy, ctx->curve); /*G_IY = G_Y and I //Initiator (U):  //Initiator U */
  }
  if(er == 0) {
    LOG_ERR("error in generate shared secret for prk_4x3m\n ");
    return 0;
  }
  LOG_DBG("G_IY: ");
  print_buff_8_dbg(giy, ECC_KEY_BYTE_LENGHT);
  er = hkdf_extrac(ctx->eph_key.prk_3e2m, ECC_KEY_BYTE_LENGHT, giy, ECC_KEY_BYTE_LENGHT, ctx->eph_key.prk_4x3m);
  if(er < 1) {
    LOG_ERR("error in extact for prk_3e2m\n");
    return 0;
  }
  LOG_DBG("PRK_4x3m: ");
  print_buff_8_dbg(ctx->eph_key.prk_4x3m, ECC_KEY_BYTE_LENGHT);
  return 1;
}
static void
gen_ciphertext_2(edhoc_context_t *ctx, uint8_t *plaintext, uint16_t plaintext_sz)
{
  for(int i = 0; i < plaintext_sz; i++) {
    plaintext[i] = plaintext[i] ^ ctx->eph_key.k2_e[i];
  }
}
static uint16_t
decrypt_ciphertext_3(edhoc_context_t *ctx, uint8_t *ciphertext, uint16_t ciphertext_sz, uint8_t *plaintext)
{
  cose_encrypt0 *cose = cose_encrypt0_new();
  /*set external aad in cose */
  cose_encrypt0_set_content(cose, NULL, 0, NULL, 0);
  uint8_t *th3_ptr = cose->external_aad;
  cose->external_aad_sz = cbor_put_bytes(&th3_ptr, ctx->session.th.buf, ctx->session.th.len);
  cose_encrypt0_set_ciphertext(cose, ciphertext, ciphertext_sz);
  /* COSE encrypt0 set header */
  cose_encrypt0_set_header(cose, NULL, 0, NULL, 0);
  /* generate info for K */
  uint16_t info_sz = generate_info(inf, ctx->session.th.buf, ctx->session.th.len, "K_3ea", strlen("K_3ea"), KEY_DATA_LENGHT);
  if(info_sz == 0) {
    LOG_ERR("error in info for decrypt ciphertext 3\n");
    return 0;
  }

  /*Set K in cose */
  int8_t er = hkdf_expand(ctx->eph_key.prk_3e2m, ECC_KEY_BYTE_LENGHT, inf, info_sz, cose->key, KEY_DATA_LENGHT);
  if(er < 1) {
    LOG_ERR("error in expand for decrypt ciphertext 3\n");
    return 0;
  }
  cose->key_sz = KEY_DATA_LENGHT;

  /* generate info for IV */
  info_sz = generate_info(inf, ctx->session.th.buf, ctx->session.th.len, "IV_3ae", strlen("IV_3ae"), IV_LENGHT);
  if(info_sz == 0) {
    LOG_ERR("error in info for decrypt ciphertext 3\n");
    return 0;
  }
  /*Set IV in cose */
  er = hkdf_expand(ctx->eph_key.prk_3e2m, ECC_KEY_BYTE_LENGHT, inf, info_sz, cose->nonce, IV_LENGHT);
  if(er < 1) {
    LOG_ERR("error in expand for decrypt ciphertext 3\n");
    return 0;
  }
  cose->nonce_sz = IV_LENGHT;

  /*Encrypt cose */
  if(!cose_decrypt(cose)) {
    LOG_ERR("ciphertext 3 decrypt error \n");
    return 0;
  }

  for(int i = 0; i < cose->plaintext_sz; i++) {
    plaintext[i] = cose->plaintext[i];
  }

  /*Empty memory */
  cose_encrypt0_finalize(cose);
  return cose->plaintext_sz;
}
static uint16_t
gen_ciphertext_3(edhoc_context_t *ctx, uint8_t *ad, uint16_t ad_sz, uint8_t *mac, uint16_t mac_sz, uint8_t *ciphertext)
{
  int8_t er = 0;
  cose_encrypt0 *cose = cose_encrypt0_new();
  /*set external aad in cose */
  uint8_t *th3_ptr = cose->external_aad;
  cose->external_aad_sz = cbor_put_bytes(&th3_ptr, ctx->session.th.buf, ctx->session.th.len);

  /*generate plaintext and set it in cose */
  uint8_t *ptr_p = &cose->plaintext[ctx->session.id_cred_x.len];
  memcpy(cose->plaintext, ctx->session.id_cred_x.buf, ctx->session.id_cred_x.len);
  uint16_t sz = cbor_put_bytes(&ptr_p, mac, mac_sz);
  sz += cbor_put_bytes(&ptr_p, ad, ad_sz);
  cose->plaintext_sz = ctx->session.id_cred_x.len + sz;

  /* generate info for K */
  uint16_t info_sz = generate_info(inf, ctx->session.th.buf, ctx->session.th.len, "K_3ea", strlen("K_3ea"), KEY_DATA_LENGHT);
  if(info_sz == 0) {
    LOG_ERR("error in info for encrypt ciphertext 3\n");
    return 0;
  }
  /*Set K in cose */
  er = hkdf_expand(ctx->eph_key.prk_3e2m, ECC_KEY_BYTE_LENGHT, inf, info_sz, cose->key, KEY_DATA_LENGHT);
  if(er < 1) {
    LOG_ERR("error in expand for decrypt ciphertext 3\n");
    return 0;
  }
  cose->key_sz = KEY_DATA_LENGHT;

  /* generate info for IV */
  info_sz = generate_info(inf, ctx->session.th.buf, ctx->session.th.len, "IV_3ae", strlen("IV_3ae"), IV_LENGHT);
  if(info_sz == 0) {
    LOG_ERR("error in info for decrypt ciphertext 3\n");
    return 0;
  }
  /*Set IV in cose */
  er = hkdf_expand(ctx->eph_key.prk_3e2m, ECC_KEY_BYTE_LENGHT, inf, info_sz, cose->nonce, IV_LENGHT);
  if(er < 1) {
    LOG_ERR("error in expand for decrypt ciphertext 3\n");
    return 0;
  }
  cose->nonce_sz = IV_LENGHT;

  /* COSE encrypt0 set header */
  cose_encrypt0_set_header(cose, NULL, 0, NULL, 0);

  /*Encrypt cose */
  cose_encrypt(cose);

  uint8_t *ptr = ciphertext;
  uint16_t ext = cbor_put_bytes(&ptr, cose->ciphertext, cose->ciphertext_sz);

  /*Empty memory */
  cose_encrypt0_finalize(cose);
  return ext;
}
uint8_t
edhoc_get_authentication_key(edhoc_context_t *ctx)
{
  LOG_DBG("Check for authentication key\n");
  cose_key_t *key;
  if(edhoc_check_key_list_identity(AUTH_KEY_IDENTITY, strlen(AUTH_KEY_IDENTITY), &key)) {
    memcpy(ctx->authen_key.private_key, key->private, ECC_KEY_BYTE_LENGHT);
    memcpy(ctx->authen_key.public.x, key->x, ECC_KEY_BYTE_LENGHT + 1);
    memcpy(ctx->authen_key.public.y, key->y, ECC_KEY_BYTE_LENGHT);
    memcpy(ctx->authen_key.kid, key->kid, key->kid_sz);
    ctx->authen_key.kid_sz = key->kid_sz;
    LOG_DBG("Authentication key has been set from the storage\n");
    return 1;
  }
  LOG_ERR("Not key for the specific AUTH_KEY_IDENTITY in the storage\n");
  return 0;
}
void
edhoc_gen_msg_1(edhoc_context_t *ctx, uint8_t *ad, size_t ad_sz)
{
  /*Generate message 1 */
  edhoc_msg_1 msg1 = { 0, 0, { NULL, 0 }, { NULL, 0 }, { NULL, 0 } };
  msg1.method = ctx->session.method;
  msg1.suit_U = ctx->session.suit;
  msg1.Gx = (bstr){(uint8_t *)&ctx->ephimeral_key.public.x, ECC_KEY_BYTE_LENGHT + 1 };
  msg1.Ci = (bstr){&ctx->session.cid, 1 };
  msg1.uad = (bstr){ad, ad_sz };

  /*cbor encode message on the buffer */
  size_t size = edhoc_serialize_msg_1(&msg1, ctx->msg_tx);
  ctx->tx_sz = size;

  LOG_DBG("AAD1 (%d):", (int)ad_sz);
  print_char_8_dbg((char *)ad, ad_sz);
  LOG_DBG("MSG1 (%d):", ctx->tx_sz);
  print_buff_8_dbg(ctx->msg_tx, ctx->tx_sz);
}
void
edhoc_gen_msg_2(edhoc_context_t *ctx, uint8_t *ad, size_t ad_sz)
{
  size_t data_sz = set_data_2(ctx);
  ctx->session.th.buf = ctx->eph_key.th;
  ctx->session.th.len = ECC_KEY_BYTE_LENGHT;
  gen_th2(ctx, ctx->msg_tx, data_sz, ctx->msg_rx, ctx->rx_sz);
  LOG_DBG("TH2(%d):", (int)ctx->session.th.len);
  print_buff_8_dbg(ctx->session.th.buf, ctx->session.th.len);
  /*Generate MAC */
  /*generate id_cred_x and cred_x */
  /*The cose key include the authentication key */
  cose_key cose;
  LOG_DBG("PART R (Receiver): cose key of R for Authenticate MSG2 \n");
  generate_cose_key(&ctx->authen_key, &cose, AUTH_KEY_IDENTITY, strlen(AUTH_KEY_IDENTITY));
  cose_print_key(&cose);

  ctx->session.cred_x.buf = cred_x;
  ctx->session.cred_x.len = generate_cred_x(&cose, ctx->session.cred_x.buf);
  LOG_DBG("CRED_R:");
  print_buff_8_dbg(ctx->session.cred_x.buf, ctx->session.cred_x.len);

  ctx->session.id_cred_x.buf = id_cred_x;
  ctx->session.id_cred_x.len = generate_id_cred_x(&cose, ctx->session.id_cred_x.buf);
  LOG_DBG("ID_CRED_R:");
  print_buff_8_dbg(ctx->session.id_cred_x.buf, ctx->session.id_cred_x.len);

  LOG_DBG("prk_2e\n");
  gen_prk_2e(ctx);

  /*generate prk_3e2m */
  gen_prk_3e2m(ctx, &ctx->authen_key, 1);

#if ((METHOD == METH0) || (METHOD == METH2))

#endif

#if ((METHOD == METH1) || (METHOD == METH3))
  gen_mac_dh(ctx, ad, ad_sz, mac, 2);
  LOG_DBG("MAC 2:");
  print_buff_8_dbg(mac, MAC_LEN);
#endif

  memcpy(buf, ctx->session.id_cred_x.buf, ctx->session.id_cred_x.len);
  uint8_t *ptr = buf + ctx->session.id_cred_x.len;
  uint16_t cbor_sz = cbor_put_bytes(&ptr, mac, MAC_LEN);
  if(ad_sz != 0) {
    cbor_sz += cbor_put_bytes(&ptr, ad, ad_sz);
  }
  gen_k_2e(ctx, ctx->session.id_cred_x.len + cbor_sz);
  LOG_DBG("K2_e (%d): ", (int)(ctx->session.id_cred_x.len + cbor_sz));
  print_buff_8_dbg(ctx->eph_key.k2_e, ctx->session.id_cred_x.len + cbor_sz);
  LOG_DBG("PLAINTEXT_2 (%d):", (int)(ctx->session.id_cred_x.len + cbor_sz));
  print_buff_8_dbg(buf, ctx->session.id_cred_x.len + cbor_sz);
  gen_ciphertext_2(ctx, buf, ctx->session.id_cred_x.len + cbor_sz);
  print_buff_8_dbg(buf, ctx->session.id_cred_x.len + cbor_sz);
  LOG_DBG("CIPHERTEXT_2 (%d):", (int)(ctx->session.id_cred_x.len + cbor_sz));
  print_buff_8_dbg(buf, ctx->session.id_cred_x.len + cbor_sz);
  /*set ciphertext on msg tx */
  ctx->tx_sz = data_sz;
  ptr = ctx->msg_tx + data_sz;
  ctx->session.ciphertex_2.len = cbor_put_bytes(&ptr, buf, ctx->session.id_cred_x.len + cbor_sz);
  ctx->tx_sz += ctx->session.ciphertex_2.len;

  uint8_t head = 1;
  if(*(ctx->msg_tx + data_sz) == 0x58) {
    head++;
  }
  ctx->session.ciphertex_2.len -= head;
  ctx->session.ciphertex_2.buf = ctx->msg_tx + data_sz + head;
}
void
edhoc_gen_msg_3(edhoc_context_t *ctx, uint8_t *ad, size_t ad_sz)
{
  /*generate data_3 an put it on the msg_tx */
  LOG_DBG("MSG3\n");
  size_t data_3_sz = set_data_3(ctx);

  /*gen TH3 */
  LOG_DBG("TH3\n");
  /*Set the pointer to th2 */
  ctx->session.th.buf = ctx->eph_key.th;
  ctx->session.th.len = ECC_KEY_BYTE_LENGHT;
  gen_th3(ctx, ctx->msg_tx, data_3_sz, ctx->session.ciphertex_2.buf, ctx->session.ciphertex_2.len);

  LOG_DBG("cose key\n");
  /*Generate cose authentication key */
  cose_key cose;
  LOG_DBG("PART I (Initiator): cose key of I for Authenticate MSG3\n");
  generate_cose_key(&ctx->authen_key, &cose, AUTH_KEY_IDENTITY, strlen(AUTH_KEY_IDENTITY));
  cose_print_key(&cose);

  /*generate cred_x */
  ctx->session.cred_x.buf = cred_x;
  ctx->session.cred_x.len = generate_cred_x(&cose, ctx->session.cred_x.buf);
  LOG_DBG("CRED_I:");
  print_buff_8_dbg(ctx->session.cred_x.buf, ctx->session.cred_x.len);

  /*generate id_cred_x */
  ctx->session.id_cred_x.buf = id_cred_x;
  ctx->session.id_cred_x.len = generate_id_cred_x(&cose, ctx->session.id_cred_x.buf);
  LOG_DBG("ID_CRED_I:");
  print_buff_8_dbg(ctx->session.id_cred_x.buf, ctx->session.id_cred_x.len);

  /*Genetrate prk_4x3m */
  gen_prk_4x3m(ctx, &ctx->authen_key, 0);

#if ((METHOD == METH0) || (METHOD == METH2))

#endif

  /*Generate Authentication MAC */
#if ((METHOD == METH1) || (METHOD == METH3))
  gen_mac_dh(ctx, ad, ad_sz, mac, 3);

  LOG_DBG("MAC 3:");
  print_buff_8_dbg(mac, MAC_LEN);
#endif

  /*Gen ciphertex_3 */
  uint16_t ciphertext_sz = gen_ciphertext_3(ctx, ad, ad_sz, mac, MAC_LEN, &ctx->msg_tx[data_3_sz]);
  ctx->tx_sz = data_3_sz + ciphertext_sz;

  /*Point ciphertext_3 for the exporter */
  uint8_t *ptr_c = &ctx->msg_tx[data_3_sz];
  ctx->session.ciphertex_3.len = get_bytes(&ptr_c, &ctx->session.ciphertex_3.buf);
}
uint8_t
edhoc_gen_msg_error(uint8_t *msg_er, edhoc_context_t *ctx, int8_t err)
{
  edhoc_msg_error msg;
  uint8_t var = ctx->session.method % 4;

  if((PART == PART_R) && ((var == 0) || (var == 2))) {
    msg.Cx = (bstr){&ctx->session.cid_rx, 1 };
  } else if((PART == PART_I) && ((var == 0) || (var == 1))) {
    msg.Cx = (bstr){&ctx->session.cid_rx, 1 };
  } else {
    msg.Cx = (bstr){NULL, 0 };
  }
  switch(err * (-1)) {
  case (ERR_SUIT_NON_SUPPORT * (-1)):
    msg.err = (sstr){"ERR_SUIT_NON_SUPPORT", strlen("ERR_SUIT_NON_SUPPORT") };
    break;
  case (ERR_MSG_MALFORMED * (-1)):
    msg.err = (sstr){"ERR_MSG_MALFORMED", strlen("ERR_MSG_MALFORMED") };
    break;
  case (ERR_REJECT_METHOD * (-1)):
    msg.err = (sstr){"ERR_REJECT_METHOD", strlen("ERR_REJECT_METHOD") };
    break;
  case (ERR_CID_NOT_VALID * (-1)):
    msg.err = (sstr){"ERR_CID_NOT_VALID", strlen("ERR_CID_NOT_VALID") };
    break;
  case (ERR_WRONG_CID_RX * (-1)):
    msg.err = (sstr){"ERR_WRONG_CID_RX", strlen("ERR_WRONG_CID_RX") };
    break;
  case (ERR_ID_CRED_X_MALFORMED * (-1)):
    msg.err = (sstr){"ERR_ID_CRED_X_MALFORMED", strlen("ERR_ID_CRED_X_MALFORMED") };
    break;
  case (ERR_AUTHENTICATION * (-1)):
    msg.err = (sstr){"ERR_AUTHENTICATION", strlen("ERR_AUTHENTICATION") };
    break;
  case (ERR_DECRYPT * (-1)):
    msg.err = (sstr){"ERR_DECRYPT", strlen("ERR_DECRYPT") };
    break;
  case (ERR_CODE * (-1)):
    msg.err = (sstr){"ERR_CODE", strlen("ERR_CODE") };
    break;
  }

  if(err == ERR_SUIT_NON_SUPPORT) {
    msg.suit = (bstr){&ctx->session.suit, 1 };
  } else {
    msg.suit = (bstr){NULL, 0 };
  }
  LOG_DBG("ERR MSG:");
  print_char_8_dbg(msg.err.buf, msg.err.len);

  size_t err_sz = edhoc_serialize_err(&msg, msg_er);
  LOG_DBG("ERR MSG cbor:");
  print_buff_8_dbg((uint8_t *)msg_er, err_sz);
  return err_sz;
}
int8_t
edhoc_check_rx_msg(uint8_t *buffer, uint8_t buff_sz)
{
  /*Check if the rx msg is an msg_err */
  uint8_t *msg_err = buffer;
  edhoc_msg_error err;
  uint8_t msg_err_sz = 0;
  msg_err_sz = edhoc_deserialize_err(&err, msg_err, buff_sz);
  if(msg_err_sz > 0) {
    LOG_ERR("RX MSG_ERR:");
    print_char_8_err(err.err.buf, err.err.len);
    return RX_ERR_MSG;
  }
  return 0;
}
int
edhoc_handler_msg_1(edhoc_context_t *ctx, uint8_t *buffer, size_t buff_sz, uint8_t *ad)
{

  edhoc_msg_1 msg1 = { 0, 0, { NULL, 0 }, { NULL, 0 }, { NULL, 0 } };
  int er = 0;
  /*Decode MSG1 */
  set_rx_msg(ctx, buffer, buff_sz);

  /*Check if the rx msg is an msg_err */
  if(edhoc_check_rx_msg(buffer, buff_sz) < 0) {
    return RX_ERR_MSG;
  }

  LOG_DBG("MSG1:");
  print_buff_8_dbg(buffer, buff_sz);
  er = edhoc_deserialize_msg_1(&msg1, ctx->msg_rx, ctx->rx_sz);
  if(er < 0) {
    LOG_ERR("MSG1 malformed");
    return er;
  }
  LOG_DBG("PART R: Rx MSG1\n");
  print_msg_1(&msg1);

  /*check rx suit and set id connection of the other part */
  er = check_rx_suit(ctx, msg1.suit_U);
  if(er < 0) {
    LOG_ERR("Rx Suit not suported\n");
    return er;
  }

  /*Check to not have the same cid */
  er = set_rx_cid(ctx, msg1.Ci.buf, msg1.Ci.len);
  if(er < 0) {
    LOG_ERR("Not support cid rx\n");
    return er;
  }

  /*Set edhoc method */
  er = set_rx_method(ctx, msg1.method);
  if(er < 0) {
    LOG_ERR("Rx Method not suported\n");
    return er;
  }

  /*Set GX */
  set_rx_gx(ctx, msg1.Gx.buf);

  LOG_DBG("PART R :Session context:\n");
  print_connection(&ctx->session);

  LOG_DBG("MSG UAD (%d)", (int)msg1.uad.len);
  print_char_8_dbg((char *)msg1.uad.buf, msg1.uad.len);

  /*validate that is a solution to the curve fot the x-cordenate */
  /*TO DO: New REF does not make it.It is a function to make it at ecc.c */

  if(msg1.uad.len != 0) {
    memcpy(ad, msg1.uad.buf, msg1.uad.len);
  } else {
    ad = NULL;
  }
  LOG_DBG("MSG UAD (%d)", (int)msg1.uad.len);
  print_char_8_dbg((char *)ad, msg1.uad.len);
  return msg1.uad.len;
}
int
edhoc_handler_msg_2(edhoc_context_t *ctx, uint8_t *buffer, size_t buff_sz, uint8_t *ad)
{
  edhoc_msg_2 msg2;
  int er = 0;
  LOG_DBG("PART I:Recived  MSG2\n");
  set_rx_msg(ctx, buffer, buff_sz);

  /*Check if the rx msg is an msg_err */
  if(edhoc_check_rx_msg(buffer, buff_sz) < 0) {
    return RX_ERR_MSG;
  }

  er = edhoc_deserialize_msg_2(&msg2, ctx->msg_rx, ctx->rx_sz);
  if(er < 0) {
    LOG_ERR("MSG2 malformed\n");
    return er;
  }
  print_msg_2(&msg2);
  /*not need to check rx suit and set id connection of the other part */

  er = set_rx_cid(ctx, msg2.data.Cr.buf, msg2.data.Cr.len);
  if(er < 0) {
    return er;
  }
  /* TODO: retrive protocol state */
  /* if the correlation is 0 or 2 check that the received msg have the correct CU */
  if((CORR == NON_EXTERNAL_CORR) || (CORR == EXTERNAL_CORR_V)) {
    if((msg2.data.Ci.buf[0] + 24) != ctx->session.cid) {
      LOG_ERR("Wrong Cid rx: %d - %d \n", msg2.data.Ci.buf[0] + 24, ctx->session.cid);
      LOG_ERR("error code (%d)\n ", ERR_WRONG_CID_RX);
      return ERR_WRONG_CID_RX;
    }
  }

  /*Set GX */
  set_rx_gx(ctx, msg2.data.Gy.buf);

  LOG_DBG("PART U :Session context:\n");
  print_connection(&ctx->session);

  gen_prk_2e(ctx);

  ctx->session.th.buf = ctx->eph_key.th;
  ctx->session.th.len = ECC_KEY_BYTE_LENGHT;

  gen_th2(ctx, msg2.data_2.buf, msg2.data_2.len, ctx->msg_tx, ctx->tx_sz);

  LOG_DBG("TH2 (%d):", (int)ctx->session.th.len);
  print_buff_8_dbg(ctx->session.th.buf, ctx->session.th.len);

  LOG_DBG("CIPHER_2 (%d):", (int)msg2.cipher.len);
  print_buff_8_dbg(msg2.cipher.buf, msg2.cipher.len);
  /*Gen K_2e */
  gen_k_2e(ctx, msg2.cipher.len);
  LOG_DBG("K2_e (%d): ", (int)msg2.cipher.len);
  print_buff_8_dbg(ctx->eph_key.k2_e, msg2.cipher.len);

  /*Set ciphertext */
  ctx->session.ciphertex_2.buf = msg2.cipher.buf;
  ctx->session.ciphertex_2.len = msg2.cipher.len;

  /*Decripted cipher text */
  memcpy(buf, msg2.cipher.buf, msg2.cipher.len);
  LOG_DBG("PLAINTEXT_2 (%d):", (int)msg2.cipher.len);
  print_buff_8_dbg(buf, msg2.cipher.len);
  gen_ciphertext_2(ctx, buf, msg2.cipher.len);
  LOG_DBG("CIPHERTEXT_2 (%d):", (int)msg2.cipher.len);
  print_buff_8_dbg(buf, msg2.cipher.len);
  /*ID_CRED_R */
  uint8_t *sign_r = NULL;
  uint8_t *ptr = buf;
  cose_key_t key;

  ctx->session.id_cred_x.len = edhoc_get_id_cred_x(&ptr, msg2.cipher.len, &ctx->session.id_cred_x.buf, &key);
  LOG_DBG("ID_CRED_X:");
  print_buff_8_dbg(ctx->session.id_cred_x.buf, ctx->session.id_cred_x.len);
  if(ctx->session.id_cred_x.len == 0) {
    LOG_ERR("error code (%d)\n ", ERR_ID_CRED_X_MALFORMED);
    return ERR_ID_CRED_X_MALFORMED;
  }
  /* verify that the identity of the responder is among the allowed */
  else if(ctx->session.id_cred_x.len < 0) {
    LOG_ERR("error code (%d)\n ", ERR_CID_NOT_VALID);
    return ERR_CID_NOT_VALID;
  }

  /*Get MAC_2 from the decript msg*/
  uint16_t sign_r_sz = edhoc_get_sign(&ptr, &sign_r);
  uint16_t rest_sz = msg2.cipher.len - (ptr - buf);

  /*Get the ad from the decript msg*/
  if(rest_sz) {
    rest_sz = edhoc_get_ad(&ptr, ad);
  } else {
    ad = NULL;
    rest_sz = 0;
  }
  cose_key cose;
  ecc_key authenticate_R;
  /*ecc_key authnticate_R; */
  LOG_DBG("PART I: get cose authentication key of V\n");

  LOG_DBG("key_sz: %d", key.kid_sz);
  print_buff_8_dbg(key.kid, key.kid_sz);
  set_cose_key(&authenticate_R, &cose, &key, ctx->curve);

  cose_print_key(&cose);

  ctx->session.cred_x.buf = inf;
  ctx->session.cred_x.len = generate_cred_x(&cose, ctx->session.cred_x.buf);

  LOG_DBG("CRED_R:");
  print_buff_8_dbg(ctx->session.cred_x.buf, ctx->session.cred_x.len);

  gen_prk_3e2m(ctx, &authenticate_R, 0);

#if ((METHOD == METH0) || (METHOD == METH2))

#endif

#if ((METHOD == METH1) || (METHOD == METH3))
  if(check_mac_dh(ctx, ad, rest_sz, sign_r, sign_r_sz, 2) == 0) {
    LOG_ERR("error code in handeler (%d)\n ", ERR_AUTHENTICATION);
    return ERR_AUTHENTICATION;
  }
#endif
  LOG_INFO("PART U: V AUTHENTICAITION success\n");
  return rest_sz;
}
int
edhoc_handler_msg_3(edhoc_context_t *ctx, uint8_t *buffer, size_t buff_sz, uint8_t *ad)
{
  edhoc_msg_3 msg3;
  /*Decode MSG3 */
  LOG_DBG("PART R :Recived message 3\n");
  set_rx_msg(ctx, buffer, buff_sz);

  /*Check if the rx msg is an msg_err */
  if(edhoc_check_rx_msg(buffer, buff_sz) < 0) {
    return RX_ERR_MSG;
  }

  int8_t er = edhoc_deserialize_msg_3(&msg3, ctx->msg_rx, ctx->rx_sz);
  if(er < 0) {
    LOG_DBG("MSG3 malformed\n");
    return er;
  }
  print_msg_3(&msg3);

  /*Set the ciphertex_3 for the key exporter */
  ctx->session.ciphertex_3.buf = msg3.cipher.buf;
  ctx->session.ciphertex_3.len = msg3.cipher.len;

  /*TO DO: retrive protocol state */
  /* if the correlation is 0 or 2 check that the received msg have the correct CV */
  if((CORR == NON_EXTERNAL_CORR) || (CORR == EXTERNAL_CORR_U)) {
    if((msg3.data.Cr.buf[0] + 24) != ctx->session.cid) {
      LOG_ERR("Wrong Cid rx: %d - %d \n", msg3.data.Cr.buf[0] + 24, ctx->session.cid);
      return ERR_WRONG_CID_RX;
    }
  }

  /*generate TH3 */
  gen_th3(ctx, msg3.data_3.buf, msg3.data_3.len, ctx->session.ciphertex_2.buf, ctx->session.ciphertex_2.len);

  /*decrypt msg3 and check the TAG for verify the outer */
  uint16_t plaintext_sz = decrypt_ciphertext_3(ctx, msg3.cipher.buf, msg3.cipher.len, buf);
  if(plaintext_sz == 0) {
    LOG_ERR("Error in decrypt ciphertext 3\n");
    return ERR_DECRYPT;
  }

  /*TODO: verify the identity of the Inititor (U) */

  /*ID_CRED_R */
  uint8_t *sign_r = NULL;
  uint8_t *ptr = buf;

  cose_key_t key;
  ctx->session.id_cred_x.len = (uint8_t)edhoc_get_id_cred_x(&ptr, msg3.cipher.len, &ctx->session.id_cred_x.buf, &key);
  LOG_DBG("ID_CRED_X (%d):", (int)ctx->session.id_cred_x.len);
  print_buff_8_dbg(ctx->session.id_cred_x.buf, ctx->session.id_cred_x.len);
  if(ctx->session.id_cred_x.len == 0) {
    LOG_ERR("error code (%d)\n ", ERR_ID_CRED_X_MALFORMED);
    return ERR_ID_CRED_X_MALFORMED;
  } else if(ctx->session.id_cred_x.len < 0) {
    LOG_ERR("error code (%d)\n ", ERR_CID_NOT_VALID);
    return ERR_CID_NOT_VALID;
  }

  uint16_t sign_r_sz = edhoc_get_sign(&ptr, &sign_r);
  uint16_t rest_sz = msg3.cipher.len - (ptr + sign_r_sz - buf);

  if(rest_sz) {
    rest_sz = edhoc_get_ad(&ptr, ad);
  } else {
    ad = NULL;
    rest_sz = 0;
  }

  cose_key cose;
  ecc_key authenticate_I;
  set_cose_key(&authenticate_I, &cose, &key, ctx->curve);

  cose_print_key(&cose);

  ctx->session.cred_x.buf = inf;
  ctx->session.cred_x.len = generate_cred_x(&cose, ctx->session.cred_x.buf);

  LOG_DBG("CRED_I:");
  print_buff_8_dbg(ctx->session.cred_x.buf, ctx->session.cred_x.len);

  gen_prk_4x3m(ctx, &authenticate_I, 1);

#if ((METHOD == METH0) || (METHOD == METH2))

#endif

#if ((METHOD == METH1) || (METHOD == METH3))
  if(check_mac_dh(ctx, ad, rest_sz, sign_r, sign_r_sz, 3) == 0) {
    LOG_ERR("error code (%d)\n ", ERR_AUTHENTICATION);
    return ERR_AUTHENTICATION;
  }

#endif
  LOG_INFO("PART R: I AUTHENTICAION success\n");
  return rest_sz;
}
