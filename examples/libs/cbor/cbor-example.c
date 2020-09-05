/*
 * Copyright (C) 2019 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
#include "contiki.h"

#include "lib/cbor/cbor.h"
#include "lib/cbor/cbor-token.h"
#include "lib/cbor/cbor-decoder.h"
#include "lib/cbor/cbor-encoder.h"

#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
PROCESS(cbor_process, "Cbor Process");
AUTOSTART_PROCESSES(&cbor_process);
/*---------------------------------------------------------------------------*/
static uint8_t decoder_buffer[179] = {
  /* 19 0D7A # unsigned(3450) */
  0x19, 0x0D, 0x7A,
  /* 18 22 # unsigned(34) */
  0x18, 0x22,
  /* 1A 000541F0 # unsigned(344560) */
  0x1A, 0x00, 0x05, 0x41, 0xF0,
  /* 38 3E # negative(62) */
  0x38, 0x3F,
  /* 24 # negative(4) */
  0x24,
  /* 61    # text(1) */
  /*    61 # "a" */
  0x61, 0x61,
  /* 78 1A                                   # text(26) */
  /*   5468697320697320612076657279206C6F6E6720737472696E67 # "This is a very long string" */
  0x78, 0x1A, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x76, 0x65, 0x72,
  0x79, 0x20, 0x6C, 0x6F, 0x6E, 0x67, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67,
  /* 7F                                      # text(INDEF) */
  /*   5468697320697320612076657279206C6F6E6720737472696E67 # "This is a very long string" */
  0x7F, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x76, 0x65, 0x72,
  0x79, 0x20, 0x6C, 0x6F, 0x6E, 0x67, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0xFF,
  /* 25 # negative(5) */
  0x25,
  /* 88                                      # array(8) */
  /*    19 0992                              # unsigned(2450) */
  /*    18 18                                # unsigned(24) */
  /*    1A 0003BB50                          # unsigned(244560) */
  /*    38 34                                # negative(52) */
  /*    25                                   # negative(5) */
  /*    78 19                                # text(25) */
  /*       54686973206973207465787420696E73696465206172726179 # "This is text inside array" */
  /*    7F                                   # text(INDEF) */
  /*       54686973206973206120737472696E672077697468206120627265616BFF # "This is a string with a break" */
  /*    27                                   # negative(7) */
  0x88, 0x19, 0x09, 0x92, 0x18, 0x18, 0x1A, 0x00, 0x03, 0xBB, 0x50, 0x38, 0x34, 0x25, 0x78, 0x19, 0x54,
  0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x65, 0x78, 0x74, 0x20, 0x69, 0x6E, 0x73, 0x69, 0x64,
  0x65, 0x20, 0x61, 0x72, 0x72, 0x61, 0x79, 0x7F, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61,
  0x20, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x61, 0x20, 0x62, 0x72,
  0x65, 0x61, 0x6B, 0xFF, 0x27,
  /* 25 # negative(5) */
  0x25,
  /* A1           # map(1) */
  /*    63        # text(3) */
  /*       4B6579 # "Key" */
  /*    0A        # unsigned(10) */
  0xA1, 0x63, 0x4B, 0x65, 0x79, 0x0A,
  /* F4 # primitive(20) */
  0xF4,
  /* F5 # primitive(21) */
  0xF5,
  /* F6 # primitive(22) */
  0xF6,
  /* F7 # primitive(23) */
  0xF7,
  /* F9 3100 # primitive(12544) */
  0xF9, 0x31, 0x00,
  /* FB 3FF0000000000002 # primitive(4607182418800017410) */
  0xFB, 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  /* FA 47C35000 # primitive(1203982336) */
  0xFA, 0x47, 0xC3, 0x50, 0x00,
  /* F0 # primitive(16) */
  0xF0,
  /* 82       # array(2) */
  /*    01    # unsigned(1) */
  /*    82    # array(2) */
  /*       02 # unsigned(2) */
  /*       03 # unsigned(3) */
  0x82, 0x01, 0x82, 0x02, 0x03,
};
/*---------------------------------------------------------------------------*/
void
cbor_dump(struct cbor_object *cbor, uint8_t depth)
{
  uint64_t i;
  struct cbor_token token;

  cbor_token_init(&token);

  while(cbor_decode_token(cbor, &token)) {

    switch(token.type) {
    case CBOR_TOKEN_TYPE_UNSIGNED:
      printf("Unsigned (%" PRIu64 ")", token.integer.value);
      break;
    case CBOR_TOKEN_TYPE_NEGATIVE:
      printf("Negative (%" PRIu64 ")", token.integer.value);
      break;
    case CBOR_TOKEN_TYPE_TEXT:
      printf("Text (%" PRIu64 ") ", token.string.length);
      i = 0;
      printf("\"");
      for(i = 0; i < token.string.length; i++) {
        printf("%c", token.string.buffer[i]);
      }
      printf("\"");
      break;
    case CBOR_TOKEN_TYPE_MAP:
      printf("Map (%" PRIu64 ")", token.collection.length);
      break;
    case CBOR_TOKEN_TYPE_ARRAY:
      printf("Array (%" PRIu64 ")", token.collection.length);
      break;
    case CBOR_TOKEN_TYPE_PRIMITIVE:
      switch(token.primitive.type) {
      case CBOR_TOKEN_PRIMITIVE_FALSE:
        printf("Primitive (false)");
        break;
      case CBOR_TOKEN_PRIMITIVE_TRUE:
        printf("Primitive (true)");
        break;
      case CBOR_TOKEN_PRIMITIVE_NULL:
        printf("Primitive (null)");
        break;
      case CBOR_TOKEN_PRIMITIVE_UNDEFINED:
        printf("Primitive (undefined)");
        break;
      case CBOR_TOKEN_PRIMITIVE_DOUBLE_PRECISION_FLOAT:
      case CBOR_TOKEN_PRIMITIVE_SINGLE_PRECISION_FLOAT:
      case CBOR_TOKEN_PRIMITIVE_HALF_PRECISION_FLOAT:
      case CBOR_TOKEN_PRIMITIVE_SIMPLE_VALUE:
        printf("Primitive (%" PRIu64 ")", token.primitive.value);
        break;
      case CBOR_TOKEN_PRIMITIVE_BREAK:
        printf("Primitive (break)");
        break;
      default:
        printf("Primitive (%" PRIu64 ")", token.primitive.value);
        break;
      }
      break;
    default:
      printf("No type");
      break;
    }
    printf("\n");
  }
}
/*---------------------------------------------------------------------------*/
void
cbor_decoder_example()
{
  struct cbor_object cbor;

  cbor_object_init(&cbor, decoder_buffer, 179);

  cbor_object_dump_hex(&cbor);
  cbor_dump(&cbor, 0);
}
/*---------------------------------------------------------------------------*/
void
cbor_encoder_example()
{
  uint8_t buffer[256];
  struct cbor_object cbor;

  cbor_object_init(&cbor, buffer, 256);

  /* 19 0D7A # unsigned(3450) */
  cbor_encode_unsigned(&cbor, 3450);
  /* 18 22 # unsigned(34) */
  /*cbor_encode_unsigned(&cbor, 34); */
  /* 1A 000541F0 # unsigned(344560) */
  /*cbor_encode_unsigned(&cbor, 344560); */
  /* 38 3E # negative(62) */
  /*cbor_encode_negative(&cbor, 62); */
  /* 24 # negative(4) */
  /*cbor_encode_negative(&cbor, 4); */

  /* 61    # text(1) */
  /*    61 # "a" */
  /*char *text_1 = "a"; */
  /*cbor_encode_text_string(&cbor, (uint8_t *)text_1, strlen(text_1)); */
  /*/ * 78 1A                                   # text(26) * / */
  /*/ *   5468697320697320612076657279206C6F6E6720737472696E67 # "This is a very long string" * / */
  /*0x78, 0x1A, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x76, 0x65, 0x72, */
  /*0x79, 0x20, 0x6C, 0x6F, 0x6E, 0x67, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, */
  /*/ * 7F                                      # text(INDEF) * / */
  /*/ *   5468697320697320612076657279206C6F6E6720737472696E67 # "This is a very long string" * / */
  /*0x7F, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x76, 0x65, 0x72, */
  /*0x79, 0x20, 0x6C, 0x6F, 0x6E, 0x67, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0xFF, */
  /*/ * 25 # negative(5) * / */
  /*0x25, */
  /*/ * 88                                      # array(8) * / */
  /*/ *    19 0992                              # unsigned(2450) * / */
  /*/ *    18 18                                # unsigned(24) * / */
  /*/ *    1A 0003BB50                          # unsigned(244560) * / */
  /*/ *    38 34                                # negative(52) * / */
  /*/ *    25                                   # negative(5) * / */
  /*/ *    78 19                                # text(25) * / */
  /*/ *       54686973206973207465787420696E73696465206172726179 # "This is text inside array" * / */
  /*/ *    7F                                   # text(INDEF) * / */
  /*/ *       54686973206973206120737472696E672077697468206120627265616BFF # "This is a string with a break" * / */
  /*/ *    27                                   # negative(7) * / */
  /*0x88, 0x19, 0x09, 0x92, 0x18, 0x18, 0x1A, 0x00, 0x03, 0xBB, 0x50, 0x38, 0x34, 0x25, 0x78, 0x19, 0x54, */
  /*0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x65, 0x78, 0x74, 0x20, 0x69, 0x6E, 0x73, 0x69, 0x64, */
  /*0x65, 0x20, 0x61, 0x72, 0x72, 0x61, 0x79, 0x7F, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, */
  /*0x20, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x61, 0x20, 0x62, 0x72, */
  /*0x65, 0x61, 0x6B, 0xFF, 0x27, */
  /*/ * 25 # negative(5) * / */
  /*0x25, */
  /*/ * A1           # map(1) * / */
  /*/ *    63        # text(3) * / */
  /*/ *       4B6579 # "Key" * / */
  /*/ *    0A        # unsigned(10) * / */
  /*0xA1, 0x63, 0x4B, 0x65, 0x79, 0x0A, */
  /*/ * F4 # primitive(20) * / */
  /*0xF4, */
  /*/ * F5 # primitive(21) * / */
  /*0xF5, */
  /*/ * F6 # primitive(22) * / */
  /*0xF6, */
  /*/ * F7 # primitive(23) * / */
  /*0xF7, */
  /*/ * F9 3100 # primitive(12544) * / */
  /*0xF9, 0x31, 0x00, */
  /*/ * FB 3FF0000000000002 # primitive(4607182418800017410) * / */
  /*0xFB, 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, */
  /*/ * FA 47C35000 # primitive(1203982336) * / */
  /*0xFA, 0x47, 0xC3, 0x50, 0x00, */
  /*/ * F0 # primitive(16) * / */
  /*0xF0, */
  /*/ * 82       # array(2) * / */
  /*/ *    01    # unsigned(1) * / */
  /*/ *    82    # array(2) * / */
  /*/ *       02 # unsigned(2) * / */
  /*/ *       03 # unsigned(3) * / */
  /*0x82, 0x01, 0x82, 0x02, 0x03, */

  cbor_object_dump_hex(&cbor);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cbor_process, ev, data)
{
  PROCESS_BEGIN();

  /*cbor_decoder_example(); */

  cbor_encoder_example();

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/* EOF */
