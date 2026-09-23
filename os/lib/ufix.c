/*
 * libfixmath is Copyright (c) 2011-2021 Flatmush <Flatmush@gmail.com>,
 * Petteri Aimonen <Petteri.Aimonen@gmail.com>, & libfixmath AUTHORS
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * \file
 *         Adapted version of libfixmath.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "ufix.h"
#include <stdbool.h>
#include <assert.h>

/*---------------------------------------------------------------------------*/
static uint32_t
generic_from_uint(unsigned a, uint32_t one)
{
  return a * one;
}
/*---------------------------------------------------------------------------*/
ufix16_t
ufix16_from_uint(unsigned a)
{
  return generic_from_uint(a, UFIX16_ONE);
}
/*---------------------------------------------------------------------------*/
ufix22_t
ufix22_from_uint(unsigned a)
{
  return generic_from_uint(a, UFIX22_ONE);
}
/*---------------------------------------------------------------------------*/
static uint32_t
generic_multiply(uint32_t a, uint32_t b, uint8_t mantissa_bits)
{
  uint64_t product = (uint64_t)a * b;
  return product >> mantissa_bits;
}
/*---------------------------------------------------------------------------*/
ufix16_t
ufix16_multiply(ufix16_t a, ufix16_t b)
{
  return generic_multiply(a, b, 16);
}
/*---------------------------------------------------------------------------*/
ufix22_t
ufix22_multiply(ufix22_t a, ufix22_t b)
{
  return generic_multiply(a, b, 22);
}
/*---------------------------------------------------------------------------*/
static uint32_t
generic_divide(uint32_t a, uint32_t b, uint8_t mantissa_bits)
{
  uint64_t dividend = (uint64_t)a << mantissa_bits;
  return (uint32_t)(dividend / b);
}
/*---------------------------------------------------------------------------*/
ufix16_t
ufix16_divide(ufix16_t a, ufix16_t b)
{
  return generic_divide(a, b, 16);
}
/*---------------------------------------------------------------------------*/
ufix22_t
ufix22_divide(ufix22_t a, ufix22_t b)
{
  return generic_divide(a, b, 22);
}
/*---------------------------------------------------------------------------*/
static uint32_t
generic_sqrt(uint32_t a, uint8_t mantissa_bits)
{
  uint32_t result = 0;

  uint32_t bit;
  if(a & 0xFFF00000) {
    bit = (uint32_t)1 << 30;
  } else {
    bit = (uint32_t)1 << 18;
  }

  while(bit > a) {
    bit >>= 2;
  }

  for(uint_fast8_t n = 0; n < 2; n++) {
    while(bit) {
      if(a >= result + bit) {
        a -= result + bit;
        result = (result >> 1) + bit;
      } else {
        result >>= 1;
      }
      bit >>= 2;
    }

    if(!n) {
      if(a >= ((uint32_t)1 << 16)) {
        a -= result;
        a = (a << 16) - (1 << (16 - 1));
        result = (result << 16) + (1 << (16 - 1));
      } else {
        a <<= 16;
        result <<= 16;
      }

      bit = 1 << (16 - 2 - (mantissa_bits - 16));
    }
  }

#ifndef FIXMATH_NO_ROUNDING
  if(a > result) {
    result++;
  }
#endif

  return (ufix16_t)result;
}
/*---------------------------------------------------------------------------*/
ufix16_t
ufix16_sqrt(ufix16_t a)
{
  return generic_sqrt(a, 16);
}
/*---------------------------------------------------------------------------*/
ufix22_t
ufix22_sqrt(ufix22_t a)
{
  return generic_sqrt(a, 22);
}
/*---------------------------------------------------------------------------*/
static uint32_t
right_shift_rounded(uint32_t x)
{
#ifdef FIXMATH_NO_ROUNDING
  return x >> 1;
#else
  return (x >> 1) + (x & 1);
#endif
}
/*---------------------------------------------------------------------------*/
static uint32_t
generic_log2(uint32_t a, uint_fast8_t mantissa_bits, uint32_t two)
{
  uint32_t result = 0;

  while(a >= two) {
    result++;
    a = right_shift_rounded(a);
  }

  if(!a) {
    return result << mantissa_bits;
  }

  for(uint_fast8_t i = mantissa_bits; i > 0; i--) {
    a = generic_multiply(a, a, mantissa_bits);
    result <<= 1;
    if(a >= two) {
      result |= 1;
      a = right_shift_rounded(a);
    }
  }
#ifndef FIXMATH_NO_ROUNDING
  a = generic_multiply(a, a, mantissa_bits);
  if(a >= two) {
    result++;
  }
#endif
  return result;
}
/*---------------------------------------------------------------------------*/
ufix16_t
ufix16_log2(ufix16_t a)
{
  assert(a >= UFIX16_ONE);
  return generic_log2(a, 16, UFIX_FROM_UINT(2, 16));
}
/*---------------------------------------------------------------------------*/
ufix22_t
ufix22_log2(ufix22_t a)
{
  assert(a >= UFIX22_ONE);
  return (ufix22_t)generic_log2(a, 22, UFIX_FROM_UINT(2, 22));
}
/*---------------------------------------------------------------------------*/
