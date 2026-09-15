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
 * \file
 *         Project configuration for the ip64-router example.
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */
/*---------------------------------------------------------------------------*/
#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_
/*---------------------------------------------------------------------------*/
/* Use the platform-independent ENC28J60 arch layer on top of the SPI HAL. */
#define ENC28J60_CONF_USE_SPI_HAL 1
/*---------------------------------------------------------------------------*/
/*
 * Wiring for the nRF54L15 DK expansion header, marked "PORT P1" on the board.
 * Only P1.04 through P1.14 are brought out, and of those the console UART
 * (P1.04/P1.05), the buttons (P1.08/P1.09/P1.13) and LED2/LED4
 * (P1.10/P1.14) are already spoken for. That leaves the three SPI22 signals
 * and P1.12, which is why CS is on P1.12 and not on P1.10 as Zephyr's
 * devicetree has it.
 *
 * RESET is deliberately not wired: the module pulls it up, and the driver
 * issues a soft reset over SPI. Define ETH_RESET_PORT/ETH_RESET_PIN if you
 * do connect it.
 */
#define ETH_SPI_CONTROLLER   0    /* logical controller 0 -> SPIM22 */

#define ETH_SPI_CLK_PORT     1
#define ETH_SPI_CLK_PIN     11
#define ETH_SPI_MOSI_PORT    1
#define ETH_SPI_MOSI_PIN     6
#define ETH_SPI_MISO_PORT    1
#define ETH_SPI_MISO_PIN     7
#define ETH_SPI_CSN_PORT     1
#define ETH_SPI_CSN_PIN     12

/* The ENC28J60 is rated for 20 MHz; start slow while bringing the wiring up. */
#define ETH_SPI_BIT_RATE     4000000
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/* IP64 translates TCP as well as UDP, so uIP needs TCP compiled in. */
#define UIP_CONF_TCP 1
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*
 * Optional NAT64 self-test: with NAT64_TEST_ADDR defined, the router sends a
 * UDP datagram every few seconds to that IPv4 host through its own NAT64
 * prefix. Off by default; enable it by defining the target as four
 * comma-separated octets and running a listener there with  nc -u -l 7777
 *
 *   #define NAT64_TEST_ADDR  192, 168, 1, 10
 *   #define NAT64_TEST_PORT  7777
 */
/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */
