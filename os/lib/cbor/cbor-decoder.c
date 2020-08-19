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

#include "cbor-decoder.h"

/*---------------------------------------------------------------------------*/
static int
cbor_decode_integer(struct cbor_object *data, struct cbor_token *token)
{
  token->integer.value = 0x00;

  if(token->additional_information < 24) {
    token->integer.value = token->additional_information;
    return 1;
  }

  switch(token->additional_information) {
  case 27:
    token->integer.value |= ((uint64_t)*(data->buffer + data->position++)) << 56;
    token->integer.value |= ((uint64_t)*(data->buffer + data->position++)) << 48;
    token->integer.value |= ((uint64_t)*(data->buffer + data->position++)) << 40;
    token->integer.value |= ((uint64_t)*(data->buffer + data->position++)) << 32;
  case 26:
    token->integer.value |= ((uint64_t)*(data->buffer + data->position++)) << 24;
    token->integer.value |= ((uint64_t)*(data->buffer + data->position++)) << 16;
  case 25:
    token->integer.value |= ((uint64_t)*(data->buffer + data->position++)) << 8;
  case 24:
    token->integer.value |= ((uint64_t)*(data->buffer + data->position++));
    return 1;
  default:
    return 0;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
cbor_decode_string(struct cbor_object *data, struct cbor_token *token)
{
  uint64_t length;

  if(token->additional_information < 31) {
    if(!cbor_decode_integer(data, token)) {
      return 0;
    }
    length = token->integer.value;
  } else {
    /* Indefinite string - Get length */
    for(length = 0; data->buffer[data->position + length] != 0xFF; length++) {
    }
  }

  token->string.buffer = (data->buffer + data->position);
  token->string.length = length;
  data->position += length;

  /* Indefinite string - One more position - 0xFF Skip */
  if(token->additional_information == 31) {
    data->position++;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
cbor_decode_collection(struct cbor_object *data, struct cbor_token *token)
{
  uint64_t length;

  if(token->additional_information < 31) {
    if(!cbor_decode_integer(data, token)) {
      return 0;
    }
    length = token->integer.value;
  } else {
    /* Indefinite string - Get length */
    for(length = 0; data->buffer[data->position + length] != 0xFF; length++) {
    }
  }

  token->collection.length = length;

  /* Indefinite string - One more position - 0xFF Skip */
  if(token->additional_information == 31) {
    data->position++;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
cbor_decode_tag(struct cbor_object *data, struct cbor_token *token)
{
  if(token->additional_information <= 27) {
    if(!cbor_decode_integer(data, token)) {
      return 0;
    }
    token->tag.value = token->integer.value;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
cbor_decode_primitive(struct cbor_object *data, struct cbor_token *token)
{
  token->primitive.type = token->additional_information;
  token->primitive.value = 0x00;

  switch(token->additional_information) {
  case CBOR_TOKEN_PRIMITIVE_FALSE:
  case CBOR_TOKEN_PRIMITIVE_TRUE:
  case CBOR_TOKEN_PRIMITIVE_NULL:
  case CBOR_TOKEN_PRIMITIVE_UNDEFINED:
    return 1;
  case CBOR_TOKEN_PRIMITIVE_DOUBLE_PRECISION_FLOAT:
    token->primitive.value |= ((uint64_t)*(data->buffer + data->position++)) << 56;
    token->primitive.value |= ((uint64_t)*(data->buffer + data->position++)) << 48;
    token->primitive.value |= ((uint64_t)*(data->buffer + data->position++)) << 40;
    token->primitive.value |= ((uint64_t)*(data->buffer + data->position++)) << 32;
  case CBOR_TOKEN_PRIMITIVE_SINGLE_PRECISION_FLOAT:
    token->primitive.value |= ((uint64_t)*(data->buffer + data->position++)) << 24;
    token->primitive.value |= ((uint64_t)*(data->buffer + data->position++)) << 16;
  case CBOR_TOKEN_PRIMITIVE_HALF_PRECISION_FLOAT:
    token->primitive.value |= ((uint64_t)*(data->buffer + data->position++)) << 8;
  case CBOR_TOKEN_PRIMITIVE_SIMPLE_VALUE:
    token->primitive.value |= ((uint64_t)*(data->buffer + data->position++));
    return 1;
  case CBOR_TOKEN_PRIMITIVE_BREAK:
    return 1;
  default:
    token->primitive.value = token->additional_information;
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
int
cbor_decode_token(struct cbor_object *data, struct cbor_token *token)
{
  if(data->position >= data->length) {
    return 0;
  }

  token->type = (*(data->buffer + data->position)) >> CBOR_TOKEN_MAJOR_TYPE_MASK;
  token->additional_information = (*(data->buffer + data->position)) & CBOR_TOKEN_STRUCTURE_MASK;

  data->position++;

  switch(token->type) {
  case CBOR_TOKEN_TYPE_UNSIGNED:
    return cbor_decode_integer(data, token);
  case CBOR_TOKEN_TYPE_NEGATIVE:
    /**
     * The encoding follows the rules for unsigned integers (major type 0),
     * except that the value is then -1 minus the encoded unsigned integer.
     */
    if(!cbor_decode_integer(data, token)) {
      return 0;
    }

    token->integer.value++;
    return 1;
  case CBOR_TOKEN_TYPE_BYTES:
  case CBOR_TOKEN_TYPE_TEXT:
    return cbor_decode_string(data, token);
  case CBOR_TOKEN_TYPE_MAP:
  case CBOR_TOKEN_TYPE_ARRAY:
    return cbor_decode_collection(data, token);
  case CBOR_TOKEN_TYPE_TAG:
    return cbor_decode_tag(data, token);
  case CBOR_TOKEN_TYPE_PRIMITIVE:
    return cbor_decode_primitive(data, token);
  default:
    /* Unhandled type */
    return 0;
  }

  return 0;
}
/* EOF */
