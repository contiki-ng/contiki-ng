/*
 * Copyright (c) 2021, Uppsala universitet.
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
 * \file
 *         Interface for fixed-point arithmetic.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef UFIX_H_
#define UFIX_H_

#include <stdint.h>

#define UFIX_LOG_2_E_INV(mantissa_bits) (0xb17217f779166c04 >> (64 - mantissa_bits))
#define UFIX_FROM_UINT(a, x) ((a) * ((uint32_t)1 << x))

typedef uint32_t ufix16_t;
#define UFIX16_FROM_UINT(a) UFIX_FROM_UINT(a, 16)
#define UFIX16_ONE UFIX16_FROM_UINT(1)
#define UFIX16_LOG_2_E_INV UFIX_LOG_2_E_INV(16)
#define UFIX16_MAX 0xFFFFFFFF
extern ufix16_t ufix16_from_uint(unsigned a);
extern ufix16_t ufix16_multiply(ufix16_t a, ufix16_t b);
extern ufix16_t ufix16_divide(ufix16_t a, ufix16_t b);
extern ufix16_t ufix16_sqrt(ufix16_t a);
extern ufix16_t ufix16_log2(ufix16_t a);

typedef uint32_t ufix22_t;
#define UFIX22_FROM_UINT(a) UFIX_FROM_UINT(a, 22)
#define UFIX22_ONE UFIX22_FROM_UINT(1)
#define UFIX22_LOG_2_E_INV UFIX_LOG_2_E_INV(22)
#define UFIX22_MAX 0xFFFFFFFF
extern ufix22_t ufix22_from_uint(unsigned a);
extern ufix22_t ufix22_multiply(ufix22_t a, ufix22_t b);
extern ufix22_t ufix22_divide(ufix22_t a, ufix22_t b);
extern ufix22_t ufix22_sqrt(ufix22_t a);
extern ufix22_t ufix22_log2(ufix22_t a);

#endif /* UFIX_H_ */
