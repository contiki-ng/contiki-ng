/*
 * Copyright (c) 2016, University of Bristol - http://www.bristol.ac.uk
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
 */
/**
 * \addtogroup cc26xx
 * @{
 *
 * \defgroup cc26xx-aes CC26x0/CC13x0 AES-128
 *
 * AES-128 driver for the CC26x0/CC13x0 SoC
 * @{
 *
 * \file
 *         Header file of the AES-128 driver for the CC26xx SoC
 * \author
 *         Atis Elsts <atis.elsts@gmail.com>
 */
#ifndef CC2538_AES_H_
#define CC2538_AES_H_

#include "lib/aes-128.h"

/**
 * \brief Set a key to use in subsequent encryption & decryption operations.
 * \param key The key to use
 *
 * The size of the key must be AES_128_KEY_LENGTH.
 */
void cc26xx_aes_set_key(const uint8_t *key);

/**
 * \brief Encrypt a message using the SoC AES-128 hardware implementation
 * \param plaintext_and_result In: message to encrypt, out: the encrypted message.
 *
 * The size of the message must be AES_128_BLOCK_SIZE.
 * The key to use in the encryption must be set before calling this function.
 */
void cc26xx_aes_encrypt(uint8_t *plaintext_and_result);

/**
 * \brief Decrypt a message using the SoC AES-128 hardware implementation
 * \param cyphertext_and_result In: message to decrypt, out: the decrypted message.
 *
 * The size of the message must be AES_128_BLOCK_SIZE.
 * The key to use in the decryption must be set before calling this function.
 */
void cc26xx_aes_decrypt(uint8_t *cyphertext_and_result);

extern const struct aes_128_driver cc26xx_aes_128_driver;

#endif /* CC2538_AES_H_ */
/**
 * @}
 * @}
 */
