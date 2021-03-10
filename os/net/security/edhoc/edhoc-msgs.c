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
 *         edhoc-msg serialize and desirialize the edhoc msgs ussing the cbor library
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 */
#include "contiki-lib.h"
#include "edhoc-msgs.h"
#include "lib/random.h"

void
print_msg_1(edhoc_msg_1 *msg)
{
  LOG_DBG("Type: %d\n", msg->method);
  LOG_DBG("Suit I:");
  print_buff_8_dbg(msg->suit_I.buf,msg->suit_I.len);
  LOG_DBG("Gx: ");
  print_buff_8_dbg(msg->Gx.buf, msg->Gx.len);
  LOG_DBG("Ci: ");
  print_buff_8_dbg(msg->Ci.buf, msg->Ci.len);
  LOG_DBG("Uad: ");
  print_buff_8_dbg(msg->uad.buf, msg->uad.len);
}
void
print_msg_2(edhoc_msg_2 *msg)
{
  LOG_DBG("Ci: ");
  print_buff_8_dbg(msg->data.Ci.buf, msg->data.Ci.len);
  LOG_DBG("GY:");
  print_buff_8_dbg(msg->data.Gy.buf, msg->data.Gy.len);
  LOG_DBG("Cr:");
  print_buff_8_dbg(msg->data.Cr.buf, msg->data.Cr.len);
  LOG_DBG("ciphertext: ");
  print_buff_8_dbg(msg->cipher.buf, msg->cipher.len);
  LOG_DBG("data_2: ");
  print_buff_8_dbg(msg->data_2.buf, msg->data_2.len);
}
void
print_msg_3(edhoc_msg_3 *msg)
{
  LOG_DBG("Cr: ");
  print_buff_8_dbg(msg->data.Cr.buf, msg->data.Cr.len);
  LOG_DBG("ciphertext: ");
  print_buff_8_dbg(msg->cipher.buf, msg->cipher.len);
}
static uint8_t
get_byte(uint8_t **in)
{
  uint8_t out = **in;
  (*in)++;
  return out;
}
int16_t
edhoc_get_unsigned(uint8_t **in)
{
  uint8_t byte = get_byte(in);

  if(byte < 0x18) {

    return byte;
  } else if(byte == 0x18) {
    return get_byte(in);
  } else {
    LOG_ERR("get not unsigned\n ");
     (*in)--;
    return -1;
  }
}
static int64_t
get_negative(uint8_t **in)
{
  uint8_t byte = get_byte(in);

  int64_t num = 0;
  if(byte < 0x38) {

    num = byte ^ 0x20;
  } else if(byte == 0x38) {
    byte = get_byte(in);
    num = byte ^ 0x20;
  } else {
    LOG_ERR("get not negative\n ");
    return 0;
  }
  num++;
  return num;
}
static uint8_t *
point_byte(uint8_t **in)
{
  uint8_t *out = *in;
  (*in)++;
  return out;
}
size_t
edhoc_get_bytes(uint8_t **in, uint8_t **out)
{
  uint8_t byte = get_byte(in);
  size_t size = 0;
  if(byte == 0x58) {
    size = get_byte(in);
    *out = *in;
    *in = (*in + size);
    return size;
  } else if((0x40 <= byte) && (byte < 0x58)) {
    size = byte ^ 0x40;
    *out = *in;
    *in = (*in + size);
    return size;
  } else {
    (*in)--;
    return 0;
  }
}
uint8_t
edhoc_get_maps_num(uint8_t **in)
{
  uint8_t byte = get_byte(in);
  uint8_t num = 0;
  if((byte >= 0xa0) && (byte <= 0xaf)) { /*max of 15 maps */
    num = byte ^ 0xa0;
    return num;
  } else {
    (*in)--;
    return 0;
  }
}
uint8_t
edhoc_get_array_num(uint8_t **in)
{
  uint8_t byte = get_byte(in);
  uint8_t num = 0;
  if((byte >= 0x80) && (byte <= 0x8f)) { /*max of 15 maps */
    num = byte ^ 0x80;
    return num;
  } else {
    (*in)--;
    return 0;
  }
}
static int16_t
get_text(uint8_t **in, char **out)
{
  uint8_t byte = get_byte(in);
  size_t size = 0;
  if(byte == 0x78) {
    size = get_byte(in);
    *out = (char *)*in;
    *in = (*in + size);
    return size;
  } else if((0x60 <= byte) && (byte < 0x78)) {
    size = byte ^ 0x60;
    *out = (char *)*in;
    *in = (*in + size);
    return size;
  } else {
    (*in)--;
    return -1;
  }
}
uint8_t
edhoc_get_byte_identifier(uint8_t **in)
{
  uint8_t out = **in;
  if((0 < out) && (out < 0x18)) {
    out = (uint8_t)edhoc_get_unsigned(in) + 24;
  } else if((0x18 < out) && (out < 0x38)) {
    out = get_negative(in) * (-1) + 24;
  } else {
    return 0;
  }

  return out;
}
size_t
edhoc_serialize_msg_1(edhoc_msg_1 *msg, unsigned char *buffer, bool suit_array)
{
  size_t size = 0;
  size += cbor_put_unsigned(&buffer, msg->method);
  /* uint8_t suit_num = 0;
 while((msg->suit_U[suit_num] < 0) && (suit_num < EDHOC_MAX_SUITS)){
    suit_num++;
  }*/
  if((msg->suit_I.len > 1) && suit_array){
    size += cbor_put_array(&buffer,msg->suit_I.len);
    uint8_t i = 0;
    while(i < msg->suit_I.len){
      size += cbor_put_unsigned(&buffer, msg->suit_I.buf[i]);
      i++;
    }
  }
  else{
    size += cbor_put_unsigned(&buffer, msg->suit_I.buf[0]);
  }

  size += cbor_put_bytes(&buffer, msg->Gx.buf, msg->Gx.len);
  size += cbor_put_bytes_identifier(&buffer, msg->Ci.buf, msg->Ci.len);
  if(msg->uad.len > 0) {
    size += cbor_put_bytes(&buffer, msg->uad.buf, msg->uad.len);
  }
  return size;
}
size_t
edhoc_serialize_data_2(edhoc_data_2 *msg, unsigned char *buffer)
{
  int size = 0;
  if(msg->Ci.len != 0) {
    size += cbor_put_bytes_identifier(&buffer, msg->Ci.buf, msg->Ci.len);
  }
  size += cbor_put_bytes(&buffer, msg->Gy.buf, msg->Gy.len);
  size += cbor_put_bytes_identifier(&buffer, msg->Cr.buf, msg->Cr.len);
  return size;
}
size_t
edhoc_serialize_data_3(edhoc_data_3 *msg, unsigned char *buffer)
{
  int size = 0;
  if(msg->Cr.len != 0) {
    size += cbor_put_bytes_identifier(&buffer, msg->Cr.buf, msg->Cr.len);
  }
  return size;
}
size_t
edhoc_serialize_err(edhoc_msg_error *msg, unsigned char *buffer)
{
  int size = 0;
  if(msg->Cx.len != 0) {
    size += cbor_put_bytes_identifier(&buffer, msg->Cx.buf, msg->Cx.len);
  }
  size += cbor_put_text(&buffer, msg->err.buf, msg->err.len);
  if(msg->suit.len == 1) {
    size += cbor_put_unsigned(&buffer, msg->suit.buf[0]);
  }
  else if(msg->suit.len > 1){
    size += cbor_put_array(&buffer,msg->suit.len);
    uint8_t i = 0;
    while(i < msg->suit.len){
      size += cbor_put_unsigned(&buffer, msg->suit.buf[i]);
      i++;
    }
  }
  return size;
}
int8_t
edhoc_deserialize_err(edhoc_msg_error *msg, unsigned char *buffer, uint8_t buff_sz)
{
  uint8_t *buff_f = buffer + buff_sz;
  uint8_t var = ((4 * METHOD) + CORR) % 4;
  if(buffer < buff_f) {
    if((PART == PART_I) && ((var == 0) || (var == 2))) {
      msg->Cx.len = edhoc_get_bytes(&buffer, &msg->Cx.buf);
      if(msg->Cx.len == 0) {
        msg->Cx.buf = point_byte(&buffer);
        msg->Cx.len = 1;
      }
    } else if((PART == PART_R) && ((var == 0) || (var == 1))) {
      msg->Cx.len = edhoc_get_bytes(&buffer, &msg->Cx.buf);
      if(msg->Cx.len == 0) {
        msg->Cx.buf = point_byte(&buffer);
        msg->Cx.len = 1;
      }
    } else {
      msg->Cx = (bstr){ NULL, 0 };
    }
  }
  if(buffer < buff_f) {
    int16_t len = get_text(&buffer, &msg->err.buf);
    if(len > 0) {
      msg->err.len = len;
      LOG_ERR("Is an error msgs\n");
      return RX_ERR_MSG;
    }
    else if(len == -1){
      return 0;      
    }
    else{
      msg->err.len = (size_t)len;
    }
  }
  
  if(buffer < buff_f) {
    msg->suit.buf = buffer;
    int16_t len = edhoc_get_unsigned(&buffer);
    if(len == -1){
       msg->suit.len = edhoc_get_array_num(&buffer); 
       msg->suit.buf = buffer;
    }
    else{
      msg->suit.len = 1;
    }
    if(msg->suit.len > 0){
      LOG_WARN("The error message include a new suit proposal\n");
      return 2;
    } 
  } 

  return 0;
}
int8_t
edhoc_deserialize_msg_1(edhoc_msg_1 *msg, unsigned char *buffer, size_t buff_sz)
{
  /*Get the METHOD */
  int8_t unint = 0;
  uint8_t *p_out = NULL;
  size_t out_sz = 0;
  uint8_t *buff_f = buffer + buff_sz;

  if(buffer < buff_f) {
    unint = (int8_t)edhoc_get_unsigned(&buffer);
    msg->method = unint;
  }
  /* Get the suit */
  if(buffer < buff_f) {
    msg->suit_I.buf = (uint8_t*) buffer;
    unint = (int8_t)edhoc_get_unsigned(&buffer);
    if(unint < 0){
      unint = edhoc_get_array_num(&buffer);
      msg->suit_I.buf = (uint8_t*)buffer;
      while(msg->suit_I.len < unint){
        edhoc_get_unsigned(&buffer);
        msg->suit_I.len++;     
      }
    }
    else{
      msg->suit_I.len = 1;
    }
  }
  /*Get Gx */
  if(buffer < buff_f) {
    out_sz = edhoc_get_bytes(&buffer, &p_out);
    if(out_sz == 0) {
      LOG_ERR("error code (%d)\n ", ERR_MSG_MALFORMED);
      return ERR_MSG_MALFORMED;
    }
    msg->Gx.buf = p_out;
    msg->Gx.len = out_sz;
  }
  /* Get the session_id (Ci) */
  if(buffer < buff_f) {
    msg->Ci.len = edhoc_get_bytes(&buffer, &msg->Ci.buf);
    if(msg->Ci.len == 0) {
      msg->Ci.buf = point_byte(&buffer);
      msg->Ci.len = 1;
    }
  }
  /* Get the uncripted msg */
  if(buffer < buff_f) {
    out_sz = edhoc_get_bytes(&buffer, &p_out);
    if(out_sz == 0) {
      LOG_ERR("error code (%d)\n ", ERR_MSG_MALFORMED);
      return ERR_MSG_MALFORMED;
    }
    msg->uad.buf = p_out;
    msg->uad.len = out_sz;
  }
  return 1;
}
int8_t
edhoc_deserialize_msg_2(edhoc_msg_2 *msg, unsigned char *buffer, size_t buff_sz)
{
  uint8_t data_sz = 0;
  msg->data_2.buf = buffer;

  uint8_t var = ((4 * METHOD) + CORR) % 4;
  if((var == 3) || (var == 1)) {
    msg->data.Ci.buf = NULL;
    msg->data.Ci.len = 0;
  } else {
    msg->data.Ci.len = edhoc_get_bytes(&buffer, &msg->data.Ci.buf);
    if(msg->data.Ci.len == 0) {
      msg->data.Ci.buf = point_byte(&buffer);
      msg->data.Ci.len = 1;
    }
  }

  msg->data.Gy.len = edhoc_get_bytes(&buffer, &msg->data.Gy.buf);
  LOG_DBG("GY:");
  print_buff_8_dbg(msg->data.Gy.buf, msg->data.Gy.len);
  if(msg->data.Gy.len == 0) {
    LOG_ERR("error code (%d)\n ", ERR_MSG_MALFORMED);
    return ERR_MSG_MALFORMED;
  }

  msg->data.Cr.len = edhoc_get_bytes(&buffer, &msg->data.Cr.buf);
  if(msg->data.Cr.len == 0) {
    msg->data.Cr.buf = point_byte(&buffer);
    msg->data.Cr.len = 1;
  }

  data_sz = buffer - msg->data_2.buf;
  msg->data_2.len = data_sz;

  /*Set the data and cipher pointers */
  msg->cipher.len = edhoc_get_bytes(&buffer, &msg->cipher.buf);
  if(msg->cipher.len == 0) {
    LOG_ERR("error code (%d)\n ", ERR_MSG_MALFORMED);
    return ERR_MSG_MALFORMED;
  }
  return 1;
}
int8_t
edhoc_deserialize_msg_3(edhoc_msg_3 *msg, unsigned char *buffer, size_t buff_sz)
{
  msg->data_3.buf = buffer;
  uint8_t var = ((4 * METHOD) + CORR) % 4;
  if((var == 2) || (var == 3)) {
    msg->data.Cr = (bstr){ NULL, 0 };
  } else {
    msg->data.Cr.len = edhoc_get_bytes(&buffer, &msg->data.Cr.buf);
    msg->data_3.len = msg->data.Cr.len + 1;
    if(msg->data.Cr.len == 0) {
      msg->data.Cr.buf = point_byte(&buffer);
      msg->data.Cr.len = 1;
      msg->data_3.len = msg->data.Cr.len;
    }
  }

  msg->cipher.len = edhoc_get_bytes(&buffer, &msg->cipher.buf);
  if(msg->cipher.len == 0) {
    LOG_ERR("error code (%d)\n ", ERR_MSG_MALFORMED);
    return ERR_MSG_MALFORMED;
  }
  return 1;
}
uint8_t
edhoc_get_cred_x_from_kid(uint8_t *kid, uint8_t kid_sz, cose_key_t **key)
{
  cose_key_t *auth_key;
  if(edhoc_check_key_list_kid(kid, kid_sz, &auth_key) == 0) {
    LOG_ERR("The authentication key id is not in the list\n");
    return ERR_NOT_ALLOWED_IDENTITY;
  }
  *key = auth_key;
  return ECC_KEY_BYTE_LENGHT;
}
uint8_t
edhoc_get_id_cred_x(uint8_t **p, uint8_t **id_cred_x, cose_key_t *key)
{
  *id_cred_x = *p;
  uint8_t num = edhoc_get_maps_num(p);
  uint8_t label;
  uint8_t key_sz = 0;
  uint8_t key_id_sz = 0;
  uint8_t *ptr = NULL;
  char* ch = NULL;

  cose_key_t *hkey;

  if(num > 0) {
    label = (uint8_t) edhoc_get_unsigned(p);
  } else {
    key->kid[0] = edhoc_get_byte_identifier(p);
    key->kid_sz = 1;
    ptr = key->kid;
    if(key->kid[0] == 0) {
      key->kid_sz = edhoc_get_bytes(p, &ptr);
      memcpy(key->kid, ptr, key->kid_sz);
    }
    label = 0;
  }

  switch(label) {
  /*(PRK_ID) ID_CRED_R = KID byte identifier (KID 1 Byte)*/
  case 0:
    key_sz = edhoc_get_cred_x_from_kid(key->kid, key->kid_sz, &hkey);
    memcpy(key, hkey, sizeof(cose_key_t));
    if(key_sz == 0) {
      return 0;
    } else if(key_sz < 0) {
      return key_sz;
    }
    break;

  /*TODO: include cases for each different support authtication case */
  /*case 32:

     key_sz = edhoc_get_bytes(p, &ptr);
     memcpy(key->x, ptr, ECC_KEY_BYTE_LENGHT);
     get_text(p, &sn);
     key->kid_sz = 0;
     if(memcmp(sn, "subject name", strlen("subject name")) == 0) {
      key_id_sz = get_text(p, &sn);
      memcpy(key->identity, sn, key_id_sz);
      key->identity_sz = key_id_sz;
     } else {
      return 0;
      LOG_ERR("missing subject name");
      break;
     }
     break;*/
  /*(PRK_ID) ID_CRED_R = map(4:KID bstr)  (KID 4 Byte)*/
  case 4:
    key_id_sz = edhoc_get_bytes(p, &ptr);
    key_sz = edhoc_get_cred_x_from_kid(ptr, key_id_sz, &hkey);
    memcpy(key, hkey, sizeof(cose_key_t));
    if(key_sz == 0) {
      return 0;
    } else if(key_sz < 0) {
      return key_sz;
    }
    break;

  /*(PRKI_2) ID_CRED_R = CRED_R */
  case 1:
    key->kty = edhoc_get_unsigned(p);
    int param = get_negative(p);
    if(param != 1) {
      break;
    }
    key->crv = (uint8_t) edhoc_get_unsigned(p);

    param = get_negative(p);
    if(param != 2) {
      break;
    }
    key_sz = edhoc_get_bytes(p, &ptr);
    memcpy(key->x, ptr, ECC_KEY_BYTE_LENGHT);

    param = get_negative(p);
    if(param != 3) {
      break;
    }
    key_sz = edhoc_get_bytes(p, &ptr);
    memcpy(key->y, ptr, ECC_KEY_BYTE_LENGHT);

    //char *ch = key->identity;
    key->identity_sz = get_text(p, &ch);
    memcpy(key->identity,ch,key->identity_sz);
    ch = NULL;
    if(!memcmp(key->identity, "subject name", strlen("subject name"))) {
      key->identity_sz = get_text(p, &ch);
      memcpy(key->identity,ch,key->identity_sz);
    }

    if(key_sz == 0) {
      return 0;
    } else if(key_sz < 0) {
      return key_sz;
    }
    break;
  }
  if(key_sz != ECC_KEY_BYTE_LENGHT) {
    LOG_ERR("wrong key size\n ");
    return 0;
  }
  uint8_t id_cred_x_sz = *p - *id_cred_x;
  return id_cred_x_sz;
}
uint8_t
edhoc_get_sign(uint8_t **p, uint8_t **sign)
{
  uint8_t sign_sz = edhoc_get_bytes(p, sign);
  return sign_sz;
}
uint8_t
edhoc_get_ad(uint8_t **p, uint8_t *ad)
{
  uint8_t *ptr;
  uint8_t ad_sz = edhoc_get_bytes(p, &ptr);
  memcpy(ad, ptr, ad_sz);
  return ad_sz;
}
