/*
 * Copyright (c) 2023, ComLab, Jozef Stefan Institute - https://e6.ijs.si/
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
 * \file
 *        Header file for the at86rf215-arch.c - architecture dependent code.
 * \author
 *        Grega Morano <grega.morano@ijs.si>
 */
/*---------------------------------------------------------------------------*/
#ifndef AT86RF215_ARCH_H_
#define AT86RF215_ARCH_H_

#include <stdint.h>

/**
 * \brief Initialize the radio's I/O periphery
 *
 * The function has to accomplish the following tasks:
 * - Configure RST pin
 * - Configure IRQ pin and setup the interrupt
 * - Configure CSn pin
 * - Enable and configure SPI
 */
void at86rf215_arch_init(void);

/**
 * \brief Reset the radio
 *
 * The radio Reset is triggered by pulling the pin RSTN to low, keeping it
 * low for 625ns and than release it to high.
 */
void at86rf215_arch_set_RSTN(void);

/**
 * \brief Release the radio from the reset mode
 */
void at86rf215_arch_clear_RSTN(void);

/**
 * \brief Enable the radio's IRQ line
 */
void at86rf215_arch_enable_EXTI(void);

/**
 * \brief Disable the radio's IRQ line
 */
void at86rf215_arch_disable_EXTI(void);

/**
 * \brief Select the radio's SPI chip select
 */
void at86rf215_arch_spi_select(void);

/**
 * \brief Deselect the radio's SPI chip select
 */
void at86rf215_arch_spi_deselect(void);

/**
 * \brief   Transfer and receive a single byte over SPI
 * \param b Byte to be sent
 * \return  Byte received
 */
uint8_t at86rf215_arch_spi_txrx(uint8_t b);

/**
 * \brief Interrupt routine
 *
 * To be called by the hardware interrupt handler - part of at86rf215-arch.c
 */
void at86rf215_isr(void);

#endif /* AT86RF215_ARCH_H */
