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

#include "cbor.h"

/*---------------------------------------------------------------------------*/
void
cbor_object_init(struct cbor_object *object, uint8_t *buffer, uint32_t length)
{
  object->buffer = buffer;
  object->length = length;
  object->position = 0;
}
/*---------------------------------------------------------------------------*/
int
cbor_object_insert(struct cbor_object *object, uint8_t value)
{
  if(object->position == object->length) {
    return 0;
  }

  *(object->buffer + object->position++) = value;
  return 1;
}
/*---------------------------------------------------------------------------*/
uint8_t *
cbor_object_get(struct cbor_object *object)
{
  if(object->position == object->length) {
    return NULL;
  }

  return object->buffer + object->position++;
}
/*---------------------------------------------------------------------------*/
void
cbor_object_dump_hex(struct cbor_object *object)
{
  uint32_t i;

  for(i = 0; i < object->position; i++) {
    PRINTF("0x%.2X ", *(object->buffer + i));
    if(i != 0 && (i % 16) == 0) {
      PRINTF("\n");
    }
  }
  PRINTF("\n");
}
/*---------------------------------------------------------------------------*/
void
cbor_token_init(struct cbor_token *token)
{
}
/*---------------------------------------------------------------------------*/
/* EOF */
