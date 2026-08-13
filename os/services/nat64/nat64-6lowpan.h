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
 *         6LoWPAN compression context for the NAT64 prefix.
 *
 *         Registers the upper 64 bits of the NAT64 prefix as IPHC
 *         context 1, saving ~8 header bytes per NAT64-bound packet.
 *         Must be included from project-conf.h on both the border
 *         router and every IoT node sending NAT64 traffic; the two
 *         sides must agree on prefix and context number, otherwise
 *         frames silently fail to decompress.
 *
 *         TODO: replace with runtime distribution once Contiki-NG
 *         implements RFC 6775 §4.2 (6CO option in RAs).
 *
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#ifndef NAT64_6LOWPAN_H_
#define NAT64_6LOWPAN_H_

/**
 * \brief Upper 64 bits of the NAT64 prefix as 8 bytes.
 *
 * Defaults to the well-known `64:ff9b::/96` (RFC 6052). Override
 * only for a non-standard prefix; both ends must match.
 */
#ifndef NAT64_6LOWPAN_PREFIX_BYTES
#define NAT64_6LOWPAN_PREFIX_BYTES 0x00, 0x64, 0xff, 0x9b, \
                                   0x00, 0x00, 0x00, 0x00
#endif

/* Reserve slot 0 for the network prefix and slot 1 for NAT64. */
#ifndef SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS 2
#elif SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS < 2
#error "NAT64 6LoWPAN context requires SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS >= 2"
#endif

/* Install the NAT64 prefix into context slot 1 from sicslowpan_init().
 * Only the upper 64 bits are elided; the embedded IPv4 in the lower
 * 64 bits is carried inline via IPHC SAM=01 (8 bytes). */
#define SICSLOWPAN_CONF_ADDR_CONTEXT_1                                 \
  do {                                                                 \
    static const uint8_t nat64_ctx_prefix[8] = {                       \
      NAT64_6LOWPAN_PREFIX_BYTES                                       \
    };                                                                 \
    memcpy(addr_contexts[1].prefix, nat64_ctx_prefix, 8);              \
  } while(0)

/** @} */

#endif /* NAT64_6LOWPAN_H_ */
