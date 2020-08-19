/*
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 *      An implementation of the Concise Binary Object Representation (CBOR) RFC 7049
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

#include "contiki.h"

#include "cbor-encoder.h"

/*---------------------------------------------------------------------------*/
static int
cbor_encoder_token(struct cbor_object *data, struct cbor_token *token)
{
  switch(token->type) {
  case CBOR_TOKEN_TYPE_UNSIGNED:
  case CBOR_TOKEN_TYPE_NEGATIVE:
    if(token->integer.value < 24) {
      token->additional_information = token->integer.value;
    } else if(token->integer.value < 256) {
      token->additional_information = 24;
    } else if(token->integer.value < 65536) {
      token->additional_information = 25;
    } else if(token->integer.value < 4294967296) {
      token->additional_information = 26;
    } else {
      token->additional_information = 27;
    }
    break;
  case CBOR_TOKEN_TYPE_BYTES:
  case CBOR_TOKEN_TYPE_TEXT:
  case CBOR_TOKEN_TYPE_MAP:
  case CBOR_TOKEN_TYPE_ARRAY:
  case CBOR_TOKEN_TYPE_TAG:
  case CBOR_TOKEN_TYPE_PRIMITIVE:
  default:
    /* Unhandled type */
    return 0;
  }

  if(!cbor_object_insert(data, (uint8_t)(token->type | token->additional_information))) {
    return 0;
  }

  switch(token->type) {
  case CBOR_TOKEN_TYPE_UNSIGNED:
  case CBOR_TOKEN_TYPE_NEGATIVE:
    if(token->additional_information < 24) {
    } else if(token->additional_information == 24) {
      if(!cbor_object_insert(data, (uint8_t)token->integer.value)) {
        return 0;
      }
    } else if(token->additional_information == 25) {
      if(!cbor_object_insert(data, (uint8_t)(token->integer.value >> 8))) {
        return 0;
      }
      if(!cbor_object_insert(data, (uint8_t)token->integer.value)) {
        return 0;
      }
    } else if(token->additional_information == 26) {
      if(!cbor_object_insert(data, (uint8_t)(token->integer.value >> 24))) {
        return 0;
      }
      if(!cbor_object_insert(data, (uint8_t)(token->integer.value >> 16))) {
        return 0;
      }
      if(!cbor_object_insert(data, (uint8_t)(token->integer.value >> 8))) {
        return 0;
      }
      if(!cbor_object_insert(data, (uint8_t)token->integer.value)) {
        return 0;
      }
    } else if(token->additional_information == 27) {
      if(!cbor_object_insert(data, (uint8_t)(token->integer.value >> 56))) {
        return 0;
      }
      if(!cbor_object_insert(data, (uint8_t)(token->integer.value >> 48))) {
        return 0;
      }
      if(!cbor_object_insert(data, (uint8_t)(token->integer.value >> 40))) {
        return 0;
      }
      if(!cbor_object_insert(data, (uint8_t)(token->integer.value >> 32))) {
        return 0;
      }
      if(!cbor_object_insert(data, (uint8_t)(token->integer.value >> 24))) {
        return 0;
      }
      if(!cbor_object_insert(data, (uint8_t)(token->integer.value >> 16))) {
        return 0;
      }
      if(!cbor_object_insert(data, (uint8_t)(token->integer.value >> 8))) {
        return 0;
      }
      if(!cbor_object_insert(data, (uint8_t)token->integer.value)) {
        return 0;
      }
    } else {
      return 0;
    }

    break;
  case CBOR_TOKEN_TYPE_BYTES:
  case CBOR_TOKEN_TYPE_TEXT:
  case CBOR_TOKEN_TYPE_MAP:
  case CBOR_TOKEN_TYPE_ARRAY:
  case CBOR_TOKEN_TYPE_TAG:
  case CBOR_TOKEN_TYPE_PRIMITIVE:
  default:
    /* Unhandled type */
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
int
cbor_encode_unsigned(struct cbor_object *data, uint64_t value)
{
  struct cbor_token token;

  token.type = CBOR_TOKEN_TYPE_UNSIGNED;
  token.integer.value = value;

  return cbor_encoder_token(data, &token);
}
/*---------------------------------------------------------------------------*/
int
cbor_encode_negative(struct cbor_object *data, uint64_t value)
{
  struct cbor_token token;

  token.type = CBOR_TOKEN_TYPE_NEGATIVE;
  token.integer.value = value;

  return cbor_encoder_token(data, &token);
}
/*---------------------------------------------------------------------------*/
int
cbor_encode_byte_string(struct cbor_object *data, uint8_t *buffer, uint64_t length)
{
  struct cbor_token token;

  token.type = CBOR_TOKEN_TYPE_BYTES;
  token.string.length = length;
  token.string.buffer = buffer;

  return cbor_encoder_token(data, &token);
}
/*---------------------------------------------------------------------------*/
int
cbor_encode_text_string(struct cbor_object *data, uint8_t *buffer, uint64_t length)
{
  struct cbor_token token;

  token.type = CBOR_TOKEN_TYPE_TEXT;
  token.string.length = length;
  token.string.buffer = buffer;

  return cbor_encoder_token(data, &token);
}
/*---------------------------------------------------------------------------*/
int
cbor_encode_array(struct cbor_object *data, uint64_t length)
{
  struct cbor_token token;

  token.type = CBOR_TOKEN_TYPE_TEXT;
  token.collection.length = length;

  return cbor_encoder_token(data, &token);
}
/*---------------------------------------------------------------------------*/
int
cbor_encode_map(struct cbor_object *data, uint64_t length)
{
  struct cbor_token token;

  token.type = CBOR_TOKEN_TYPE_TEXT;
  token.collection.length = length;

  return cbor_encoder_token(data, &token);
}
/*---------------------------------------------------------------------------*/
int
cbor_encode_tag(struct cbor_object *data, uint64_t value)
{
  struct cbor_token token;

  token.type = CBOR_TOKEN_TYPE_TEXT;
  token.tag.value = value;

  return cbor_encoder_token(data, &token);
}
/* EOF */
