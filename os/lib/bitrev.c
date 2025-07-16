/*
 * Copyright (c) 2023, RISE Research Institutes of Sweden AB
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

/**
 * \addtogroup bitrev
 * @{
 */

/**
 * \file
 *         Bit reversal library implementation
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

#include "bitrev.h"
/*---------------------------------------------------------------------------*/
/*
 * Lookup table for bit reversal
 *
 * This precomputed lookup table allows O(1) bit reversal for each byte.
 * The table is generated using the bit manipulation technique where:
 * - R2(n) generates 4 entries: n, n+128, n+64, n+192  
 * - R4(n) generates 16 entries by applying R2 to 4 different bases
 * - R6(n) generates 64 entries by applying R4 to 4 different bases
 * - Final call generates all 256 entries
 */
static const uint8_t bitrev_lookup_table[256] = {
  #define R2(n)   n,     n + 2*64,   n + 1*64,   n + 3*64
  #define R4(n)   R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
  #define R6(n)   R4(n), R4(n + 2*4),  R4(n + 1*4),  R4(n + 3*4)
  R6(0), R6(2), R6(1), R6(3)
  #undef R2
  #undef R4
  #undef R6
};
/*---------------------------------------------------------------------------*/
uint8_t
bitrev_byte(uint8_t byte)
{
  return bitrev_lookup_table[byte];
}
/*---------------------------------------------------------------------------*/
void
bitrev_array(uint8_t *data, size_t len)
{
  size_t i;
  for(i = 0; i < len; i++) {
    data[i] = bitrev_lookup_table[data[i]];
  }
}
/*---------------------------------------------------------------------------*/
void
bitrev_array_copy(const uint8_t *input, uint8_t *output, size_t len)
{
  size_t i;
  for(i = 0; i < len; i++) {
    output[i] = bitrev_lookup_table[input[i]];
  }
}
/*---------------------------------------------------------------------------*/
/** @} */