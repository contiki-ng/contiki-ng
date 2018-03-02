/*
 * Copyright (c) 2013, University of Michigan.
 *
 * Copyright (c) 2015, Weptech elektronik GmbH
 * Author: Ulf Knoblich, ulf.knoblich@weptech.de
 *
 * Copyright (c) 2018, University of Bristol.
 *
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
 * 3. Neither the name of the University nor the names of its contributors
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
/*---------------------------------------------------------------------------*/
#ifndef SPI_ARCH_H_
#define SPI_ARCH_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#define BOARD_SPI_CONTROLLERS               2
/*---------------------------------------------------------------------------*/
#define BOARD_SPI_CONTROLLER_SPI0           0
#define BOARD_SPI_CONTROLLER_SPI1           1
/*---------------------------------------------------------------------------*/
/* Default values for the clock rate divider */
#ifdef SPI0_CONF_CPRS_CPSDVSR
#define SPI0_CPRS_CPSDVSR               SPI0_CONF_CPRS_CPSDVSR
#else
#define SPI0_CPRS_CPSDVSR               2
#endif

#ifdef SPI1_CONF_CPRS_CPSDVSR
#define SPI1_CPRS_CPSDVSR               SPI1_CONF_CPRS_CPSDVSR
#else
#define SPI1_CPRS_CPSDVSR               2
#endif
/*---------------------------------------------------------------------------*/
/* New API macros */
#define SPIX_WAITFORTxREADY(spi) do { \
    while(!(REG(SSI_BASE(spi) + SSI_SR) & SSI_SR_TNF)) ; \
} while(0)
#define SPIX_BUF(spi)                   REG(SSI_BASE(spi) + SSI_DR)
#define SPIX_WAITFOREOTx(spi) do { \
    while(REG(SSI_BASE(spi) + SSI_SR) & SSI_SR_BSY) ; \
} while(0)
#define SPIX_WAITFOREORx(spi) do { \
    while(!(REG(SSI_BASE(spi) + SSI_SR) & SSI_SR_RNE)) ; \
} while(0)
#define SPIX_FLUSH(spi) do { \
    while(REG(SSI_BASE(spi) + SSI_SR) & SSI_SR_RNE) { \
        SPIX_BUF(spi);                                           \
    } \
} while(0)
#define SPIX_CS_CLR(port, pin) do { \
    GPIO_CLR_PIN(GPIO_PORT_TO_BASE(port), GPIO_PIN_MASK(pin)); \
} while(0)
#define SPIX_CS_SET(port, pin) do { \
    GPIO_SET_PIN(GPIO_PORT_TO_BASE(port), GPIO_PIN_MASK(pin)); \
} while(0)

#endif /* SPI_ARCH_H_ */

/**
 * @}
 */
