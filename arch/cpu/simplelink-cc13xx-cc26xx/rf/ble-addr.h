/*
 * Copyright (c) 2016, Michael Spoerk
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
 * \addtogroup cc13xx-cc26xx-rf
 * @{
 *
 * \defgroup cc13xx-cc26xx-rf-ble-addr Driver for CC13xx/CC26xx BLE addresses
 *
 * @{
 *
 * \file
 *        Header file for the CC13xx/CC26xx BLE address driver.
 * \author
 *        Michael Spoerk <mi.spoerk@gmail.com>
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef BLE_ADDR_H_
#define BLE_ADDR_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
/**
 * \brief Retrieve the pointer to where the BLE address is stored.
 *
 * This function will return the primary address from info page, unless a
 * valid address is found in the secondary address from CCFG.
 */
uint8_t *ble_addr_ptr(void);
/*---------------------------------------------------------------------------*/
/**
 * \brief Copy the node's factory BLE address to a destination memory area
 *        in big-endian (be) order.
 * \param dst A pointer to the destination area where the BLE address is to be
 *            written
 * \return  0 : Returned successfully
 *         -1 : Returned with error
 *
 * This function will copy 6 bytes and it will invert byte order in
 * the process. The factory address on devices is normally little-endian,
 * therefore you should expect dst to store the address in a big-endian order.
 */
int ble_addr_be_cpy(uint8_t *dst);
/*---------------------------------------------------------------------------*/
/**
 * \brief Copy the node's factory BLE address to a destination memory area
 *        in little-endian (le) order.
 * \param dst A pointer to the destination area where the BLE address is to be
 *            written
 * \return  0 : Returned successfully
 *         -1 : Returned with error
 *
 * This function will copy 6 bytes, but will **not** invert the byte order.
 * This is usefull for the RF core which assumes the BLE MAC address in
 * little-endian order.
 */
int ble_addr_le_cpy(uint8_t *dst);
/*---------------------------------------------------------------------------*/
/**
 * \brief Copy the node's BLE address to a destination memory area and converts
 *      it into a EUI64 address in the process
 * \param dst A pointer to the destination area where the EUI64 address is to be
 *            written
 * \param src A pointer to the BLE address that is to be copied
 * \return  0 : Returned successfully
 *         -1 : Returned with error
 */
int ble_addr_to_eui64(uint8_t *dst, uint8_t *src);
/*---------------------------------------------------------------------------*/
/**
 * \brief Copy the node's EUI64 address that is based on its factory BLE address
 *      to a destination memory area
 * \param dst A pointer to the destination area where the EUI64 address is to be
 *            written
 * \return  0 : Returned successfully
 *         -1 : Returned with error
 */
int ble_addr_to_eui64_cpy(uint8_t *dst);
/*---------------------------------------------------------------------------*/
#endif /* BLE_ADDR_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */