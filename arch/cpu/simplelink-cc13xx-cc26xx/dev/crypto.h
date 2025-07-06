/*
 * Copyright (c) 2025, Konrad-Felix Krentz
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
 * \addtogroup cc13xx-cc26xx-cpu
 * @{
 *
 * \defgroup cc13xx-cc26xx-crypto AES and Hash Cryptoprocessor
 *
 * Driver for the AES and Hash Cryptoprocessor.
 * @{
 *
 * \file
 *         General functions of the AES and Hash Cryptoprocessor.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CRYPTO_H_
#define CRYPTO_H_

#include <stdbool.h>

/**
 * \brief Resets the cryptoprocessor.
 */
void crypto_init(void);

/**
 * \brief Enables the cryptoprocessor.
 */
void crypto_enable(void);

/**
 * \brief Disables the cryptoprocessor.
 */
void crypto_disable(void);

/**
 * \brief  Checks if the cryptoprocessor is on.
 * \return \c true if the cryptoprocessor is on and \c false otherwise.
 */
bool crypto_is_enabled(void);

#endif /* CRYPTO_H_ */

/**
 * @}
 * @}
 */
