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
 * \addtogroup lib
 * @{
 *
 * \defgroup bitrev Bit Reversal Library
 *
 * This library provides functions for reversing bits in bytes and byte arrays.
 * It's commonly used by radio drivers for protocol compliance (e.g., 802.15.4g).
 *
 * @{
 */

/**
 * \file
 *         Bit reversal library header
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

#ifndef BITREV_H_
#define BITREV_H_

#include "contiki.h"

/**
 * \brief Reverse the bits in a single byte
 * \param byte The input byte
 * \return The byte with bits reversed
 *
 * Example: bitrev_byte(0xF0) returns 0x0F
 */
uint8_t bitrev_byte(uint8_t byte);

/**
 * \brief Reverse bits in all bytes of an array (in-place)
 * \param data Pointer to the data array
 * \param len Length of the array in bytes
 *
 * This function modifies the input array in-place, reversing the bit
 * order within each byte. Commonly used for protocol compliance.
 */
void bitrev_array(uint8_t *data, size_t len);

/**
 * \brief Reverse bits in all bytes of an array (copy to output)
 * \param input Pointer to the input data array
 * \param output Pointer to the output data array
 * \param len Length of the arrays in bytes
 *
 * This function copies data from input to output while reversing
 * the bit order within each byte. Input and output arrays must not overlap.
 */
void bitrev_array_copy(const uint8_t *input, uint8_t *output, size_t len);

#endif /* BITREV_H_ */

/** @} */
/** @} */