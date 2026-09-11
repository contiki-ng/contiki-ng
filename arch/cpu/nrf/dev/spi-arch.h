/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
 * \addtogroup nrf-dev Device drivers
 * @{
 *
 * \addtogroup nrf-spi SPI driver
 * @{
 *
 * \file
 *         SPI HAL arch header for the nRF, on top of nrfx_spim.
 *
 * Logical SPI controllers (the spi_controller field of spi_device_t) are
 * indices 0..SPI_CONTROLLER_COUNT-1. Each index is mapped to an nRF SPIM
 * instance id by NRF_SPI_CONF_CONTROLLERn_ID. The instance id is the
 * number in the peripheral name, so SPIM00 is 00 and SPIM22 is 22.
 *
 * The corresponding NRFX_SPIMxx_ENABLED must also be defined, otherwise
 * nrfx does not emit the instance and the build fails on a missing
 * NRFX_SPIMxx_INST_IDX.
 *
 * \note Choosing an instance is not free. On SoCs where peripherals are
 * grouped into SERIAL slots (nRF5340, nRF54L), SPIMn and UARTEn in the same
 * slot share one interrupt vector, so only one of them can be built. Picking
 * the instance that collides with the console UART fails at link time with a
 * duplicate SERIALn_IRQHandler rather than at runtime. Known-good choices:
 *
 * - nRF54L15: SPIM00 or SPIM22. SPIM20 collides with the DK console
 *   (UARTE20), and SPIM21 would collide with UARTE21.
 * - nRF5340 application core: SPIM1 and up. SPIM0 collides with UARTE0.
 * - nRF52840: no such grouping; SPIM0 coexists with UARTE0.
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */
/*---------------------------------------------------------------------------*/
#ifndef SPI_ARCH_H_
#define SPI_ARCH_H_
/*---------------------------------------------------------------------------*/
/**
 * \brief Chunk size used when a transfer has to be staged through a RAM
 * bounce buffer.
 *
 * SPIM uses EasyDMA, which can only reach the Data RAM region. A transfer
 * whose write buffer is in flash, or which has to clock out bytes that are
 * read and discarded, is split into chunks of this size and staged through
 * static buffers. Transfers that need no staging bypass this entirely and
 * are handed to nrfx in one piece, so this size does not cap throughput on
 * the common path.
 */
#ifdef NRF_SPI_CONF_CHUNK_SIZE
#define NRF_SPI_CHUNK_SIZE NRF_SPI_CONF_CHUNK_SIZE
#else
#define NRF_SPI_CHUNK_SIZE 64
#endif
/*---------------------------------------------------------------------------*/
#endif /* SPI_ARCH_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
