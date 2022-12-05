/*
 * Copyright (C) 2021 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-usb USB driver
 * @{
 *
 * \file
 *         USB header file for the nRF.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#ifndef USB_H_
#define USB_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
/**
 * @brief Initialize the USB driver
 *
 */
void usb_init(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Writes to the USB driver
 *
 * @param buffer data to be transferred
 * @param buffer_size size of data
 *
 * @pre @ref usb_init must have been called
 */
void usb_write(uint8_t *buffer, uint32_t buffer_size);
/*---------------------------------------------------------------------------*/
/**
 * @brief Flush USB buffer
 * 
 * @pre @ref usb_init must have been called
 * @pre Data must be written by @ref usb_write prior
 */
void usb_flush(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Sets the input handler called in the event handler
 *
 * @param input character that has been read
 */
void usb_set_input(int (*input)(unsigned char c));
/*---------------------------------------------------------------------------*/
/**
 * @brief Handles the interrupt
 * 
 * @remarks Must be called from the arch interrupt handler
 * 
 */
void usb_interrupt_handler(void);
/*---------------------------------------------------------------------------*/
/* Arch specific interface                                                   */
/*---------------------------------------------------------------------------*/
/**
 * @brief Initialize the architecture specific USB driver
 *
 */
void usb_arch_init(void);
/*---------------------------------------------------------------------------*/
#endif /* USB_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
