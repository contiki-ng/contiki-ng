/*
 * Copyright (c) 2006, Swedish Institute of Computer Science
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
 *
 */

/**
 * \file
 *        Header file to the external flash memory (XMem) API.
 */

/**
 * \addtogroup dev
 * @{
 */

/**
 * \defgroup xmem XMem API
 *
 * The Xmem API  module contains functionality to use external flash memories
 * of NOR or NAND type. It us typically used as a back-end for the CFS module.
 *
 * @{
 */

#ifndef XMEM_H_
#define XMEM_H_

/**
 * Initialize the external memory.
 */
void xmem_init(void);

/**
 * Read a number of bytes from an offset in the external memory.
 *
 * \param buf The buffer where the read data is stored.
 * \param nbytes The number of bytes to read.
 * \param offset The offset to read from in the flash memory.
 * \return The number of read bytes, or -1 if the operation failed.
 */
int xmem_pread(void *buf, int nbytes, unsigned long offset);

/**
 *
 * \param buf The buffer that contains the data to write.
 * \param nbytes The number of bytes to write.
 * \param offset The offset to write to in the flash memory.
 * \return The number of written bytes, or -1 if the operation failed.
 */
int xmem_pwrite(const void *buf, int nbytes, unsigned long offset);

/**
 * Erase a sector in the flash memory.
 *
 * \param nbytes The number of bytes to erase.
 * \param offset The offset in the flash memory at which to start erasing.
 * \return The number of bytes that got erased, or -1 if the operation failed.
 *
 * Both parameters may have to be a multiple of a value that depends
 * on the particular flash memory used.
 */
int xmem_erase(long nbytes, unsigned long offset);

/*---------------------------------------------------------------------------*/
#endif /* XMEM_H_ */
/*---------------------------------------------------------------------------*/
/** @} */
/** @} */
