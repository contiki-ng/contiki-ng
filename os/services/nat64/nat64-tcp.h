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
 *         NAT64 TCP splice proxy.
 *
 *         Terminates TCP on both the IPv6 and IPv4 sides and splices
 *         the data streams.  Per-session sequence number state lets
 *         the proxy generate IoT-side ACKs and RFC 6528-compliant
 *         ISNs without translating headers across address families.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#ifndef NAT64_TCP_H_
#define NAT64_TCP_H_

#include <stdbool.h>
#include <stdint.h>
#include "nat64-platform.h"

/**
 * \brief Initialize the TCP splice proxy.
 *
 * Clears the per-session sequence number state table.
 */
void nat64_tcp_init(void);

/**
 * \brief Set the 128-bit secret key for TCP ISN generation.
 * \param key 16 bytes of cryptographically random data.
 *
 * Must be called before any TCP sessions are created.
 * The key is used as input to HMAC-SHA-256 per RFC 6528.
 */
void nat64_tcp_set_isn_secret(const uint8_t key[16]);

/**
 * \brief Process an outgoing IPv6+TCP packet from an IoT node.
 * \param pkt Pointer to the raw IPv6 packet.
 * \param len Total packet length in bytes.
 * \return 1 if the packet was handled, 0 otherwise.
 *
 * Handles SYN (initiates connect), data (forwards to server),
 * FIN (half-closes), and RST (aborts).
 */
int nat64_tcp_output(const uint8_t *pkt, uint16_t len);

/**
 * \brief Flush deferred TCP ACKs.
 *
 * Called from the platform select loop, outside the uip_buf processing
 * path, to avoid re-entrancy with tcpip_input().
 */
void nat64_tcp_flush_acks(void);

/**
 * \brief Check whether a session has buffered data awaiting delivery.
 * \param s The session to check.
 * \return true if data is pending, false otherwise.
 *
 * Used by the platform layer to suppress reading from the IPv4 socket
 * while previous data is still being paced to the IoT node.
 */
bool nat64_tcp_has_pending_data(const struct nat64_session *s);

/**
 * \brief Check whether the IoT node has already half-closed the session.
 * \param s The session to check.
 * \return true if the IoT-side FIN has been received, false otherwise.
 *
 * Used by the platform layer when the IPv4 server closes its end: if
 * the IoT side had already FIN'd, both halves are now closed and the
 * platform can destroy the session immediately rather than waiting
 * for the idle timer to reap it.
 */
bool nat64_tcp_peer_fin_received(const struct nat64_session *s);

/**
 * \brief Free any TCP sequence state associated with a session.
 * \param s The session being closed.
 *
 * Must be called when a session is closed or expires to prevent
 * stale seqstate from matching if the session slot is reused.
 */
void nat64_tcp_free_seqstate(const struct nat64_session *s);

/** @} */

#endif /* NAT64_TCP_H_ */
