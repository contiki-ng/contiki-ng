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

/**
 * \addtogroup nat64
 * @{
 *
 * \file
 *         NAT64 DNS64 translation (RFC 6147).
 *
 *         Rewrites outgoing DNS messages so that an IPv4 resolver
 *         sees A queries and the IoT node sees AAAA responses with
 *         IPv4 addresses synthesized into the NAT64 prefix.  All
 *         translation happens in-place or single-pass into a caller-
 *         supplied buffer; no DNS state is kept across calls.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#ifndef NAT64_DNS64_H_
#define NAT64_DNS64_H_

#include <stdint.h>

/**
 * \brief Rewrite an outgoing DNS query from AAAA to A.
 * \param data DNS payload buffer (modified in place).
 * \param len  Length of the DNS payload.
 *
 * Modifies QTYPE fields in-place so that the upstream IPv4 DNS server
 * receives an A query instead of AAAA (RFC 6147).
 */
void nat64_dns64_6to4(uint8_t *data, uint16_t len);

/**
 * \brief Rewrite an incoming DNS response from A to AAAA.
 * \param ipv4data   Original DNS response from the IPv4 server (read-only).
 * \param ipv4len    Length of the original response.
 * \param ipv6data   Output buffer for the rewritten response.
 * \param ipv6len    Length of the data already copied into ipv6data.
 * \param ipv6bufsiz Total size of the ipv6data output buffer.
 * \return New payload length (>= ipv6len due to 4-to-16 byte address growth).
 *
 * Converts A records to AAAA records by synthesizing IPv6 addresses
 * using the NAT64 prefix.  Each A record grows by 12 bytes.  Stops
 * translating if the output would exceed ipv6bufsiz.
 */
uint16_t nat64_dns64_4to6(const uint8_t *ipv4data, uint16_t ipv4len,
                          uint8_t *ipv6data, uint16_t ipv6len,
                          uint16_t ipv6bufsiz);

/** @} */

#endif /* NAT64_DNS64_H_ */
