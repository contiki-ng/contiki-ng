/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
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
 * \addtogroup cc26xx
 * @{
 *
 * \defgroup cc26xx-uart CC13xx/CC26xx UARTs
 *
 * Driver for the CC13xx/CC26xx UART controller
 * @{
 *
 * \file
 * Header file for the CC13xx/CC26xx UART driver
 */
#ifndef UART0_ARCH_H_
#define UART0_ARCH_H_

#include <stdint.h>

typedef int (*uart0_input_cb)(unsigned char);

/*---------------------------------------------------------------------------*/
/** \name UART functions
 * @{
 */

/**
 * \brief Initializes the UART driver
 */
void uart0_init(void);

/**
 * \brief       Writes data from a memory buffer to the UART interface.
 * \param buffer A pointer to the data buffer.
 * \param size  Size of the data buffer.
 * \return      Number of bytes that has been written to the UART. If an
 *              error occurs, a negative value is returned.
 */
int_fast32_t uart0_write(const void *buffer, size_t size);

/**
 * \brief Reads data from the UART interface to a memory buffer.
 * \param buffer A pointer to the data buffer.
 * \param size  Number of bytes to read
 * \return      Number of bytes that has been written to the buffer. If an
 *              error occurs, a negative value is returned.
 */
void uart0_set_callback(uart0_input_cb input_cb);

/** @} */
/*---------------------------------------------------------------------------*/
#endif /* UART0_ARCH_H_ */

/**
 * @}
 * @}
 */
