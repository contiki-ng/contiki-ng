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
 * \defgroup nat64 NAT64 gateway
 *
 * Stateful NAT64 service that lets IPv6-only IoT nodes reach IPv4
 * servers through a Contiki-NG border router.  The implementation
 * targets outbound, client-initiated flows over 6LoWPAN/RPL and
 * deliberately omits RFC 6146 features that have no practical use in
 * a constrained network (hairpinning, inbound bindings, full TCP
 * state machine, ...). See `os/services/nat64/README.md` for the
 * full design rationale and standards-compliance matrix.
 *
 * The module is split into:
 *   - a protocol-agnostic core (\ref nat64.h, nat64.c) that dispatches
 *     IPv6 packets and synthesizes ICMPv6 errors,
 *   - a TCP splice proxy (\ref nat64-tcp.h, nat64-tcp.c),
 *   - an inline DNS64 translator (\ref nat64-dns64.h, nat64-dns64.c),
 *   - and a thin platform layer (\ref nat64-platform.h) implemented for
 *     native Linux/macOS by `native/nat64-sock.c`.
 *
 * @{
 */

/**
 * \file
 *         NAT64 gateway core API.
 *
 *         Public entry points used by the platform layer (output
 *         path) and by transport-specific helpers to inject
 *         translated packets back into the uIP stack.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#ifndef NAT64_H_
#define NAT64_H_

#include <stdbool.h>
#include <stdint.h>
#include "net/ipv6/uip.h"

/* Forward declaration — the platform layer defines the full struct. */
struct nat64_session;

/**
 * \brief Process an outgoing IPv6 packet destined for an IPv4 host.
 * \param ipv6_pkt Pointer to the raw IPv6 packet (header + payload).
 * \param len      Total length of the packet in bytes.
 * \return 1 if the packet was handled, 0 otherwise.
 *
 * Dispatches the packet to the appropriate transport handler (UDP or
 * TCP) based on the IPv6 next-header field.  Packets with a NAT64
 * source address are silently dropped (RFC 6146 Section 3.5).
 */
int nat64_output(const uint8_t *ipv6_pkt, uint16_t len);

/**
 * \brief Check whether an IPv6 address embeds an IPv4 address via
 *        the NAT64 prefix.
 * \param addr The IPv6 address to check.
 * \return true if the address matches the configured NAT64 prefix.
 */
bool nat64_is_ip64_addr(const uip_ip6addr_t *addr);

/**
 * \brief Initialize the NAT64 gateway.
 *
 * Called by the platform layer after session state has been set up.
 */
void nat64_activate(void);

/**
 * \name Platform-to-core callbacks
 *
 * Called by the platform layer (e.g., the select loop in nat64-sock.c)
 * when data arrives on an IPv4 socket or a TCP connection changes state.
 * Each callback fabricates the corresponding IPv6 packet and injects it
 * into the uIP stack via tcpip_input().
 * @{
 */

/**
 * \brief Inject a UDP response from an IPv4 server.
 * \param s       The session that received the data.
 * \param payload UDP payload bytes (DNS responses are rewritten inline).
 * \param len     Payload length in bytes.
 */
void nat64_udp_input(struct nat64_session *s,
                     const uint8_t *payload, uint16_t len);

/**
 * \brief Notify that a TCP connection to an IPv4 server completed.
 * \param s The session whose connect() succeeded.
 *
 * Sends a SYN-ACK to the IoT node to complete the three-way handshake.
 */
void nat64_tcp_established(struct nat64_session *s);

/**
 * \brief Forward TCP data from an IPv4 server to the IoT node.
 * \param s    The established TCP session.
 * \param data Pointer to the received data.
 * \param len  Data length in bytes.
 *
 * Data is buffered per session and delivered in NAT64_TCP_SEGMENT_SIZE
 * (76-byte) ACK-paced segments to fit a single 802.15.4 frame without
 * 6LoWPAN fragmentation.
 */
void nat64_tcp_data_in(struct nat64_session *s,
                       const uint8_t *data, uint16_t len);

/**
 * \brief Notify that an IPv4 server closed a TCP connection.
 * \param s The session that was closed.
 *
 * Sends a FIN to the IoT node.
 */
void nat64_tcp_closed(struct nat64_session *s);

/**
 * \brief Inject an ICMPv4 Echo Reply received from an IPv4 host.
 * \param s        The ICMP session that received the reply.
 * \param icmp_pkt ICMPv4 reply bytes (type 0 + code + checksum +
 *                 identifier + sequence + data).
 * \param len      Length of icmp_pkt in bytes.
 *
 * Translates the ICMPv4 Echo Reply (type 0) into an ICMPv6 Echo
 * Reply (type 129), preserving the original identifier from the
 * session, and injects it into the uIP stack.
 */
void nat64_icmp_input(struct nat64_session *s,
                      const uint8_t *icmp_pkt, uint16_t len);

/** @} */

/**
 * \name ICMPv6 Destination Unreachable codes (RFC 4443 §3.1)
 * @{
 */
#define NAT64_ICMP6_NOROUTE 0 /**< No route to destination. */
#define NAT64_ICMP6_ADMIN   1 /**< Communication administratively prohibited. */
#define NAT64_ICMP6_ADDR    3 /**< Address unreachable. */
#define NAT64_ICMP6_PORT    4 /**< Port unreachable. */
/** @} */

/**
 * \name ICMPv6 Destination Unreachable synthesis
 *
 * The gateway synthesizes ICMPv6 errors toward the IoT node when a
 * packet cannot be delivered (forbidden destination, connection
 * refused, host unreachable, session table exhaustion, ...). The
 * errors are queued and drained by nat64_flush_icmp6() from the
 * platform layer's select loop, to avoid re-entrancy with
 * tcpip_input() from output paths.
 * @{
 */

/**
 * \brief Queue an ICMPv6 Destination Unreachable for delivery to the IoT node.
 * \param invoking_pkt The original IPv6 packet that triggered the error.
 * \param invoking_len Length of the invoking packet in bytes.
 * \param code         ICMPv6 Code (NAT64_ICMP6_*).
 *
 * As much of the invoking packet as fits is embedded in the error,
 * so the IoT node's transport layer can match the error to the
 * right socket.
 */
void nat64_queue_icmp6_unreach(const uint8_t *invoking_pkt,
                               uint16_t invoking_len, uint8_t code);

/**
 * \brief Queue an ICMPv6 Destination Unreachable for a 5-tuple whose
 *        connection failed.
 * \param ip6_src  IoT node's IPv6 address.
 * \param src_port IoT node's transport port (host byte order).
 * \param ip4_dst  IPv4 destination that could not be reached.
 * \param dst_port Destination port (host byte order).
 * \param ipproto  Transport protocol (6 for TCP, 17 for UDP).
 * \param code     ICMPv6 Code (NAT64_ICMP6_*).
 *
 * Used when the original IPv6 packet is no longer available (e.g.,
 * asynchronous TCP connect failure).  Fabricates a minimal invoking
 * packet from the 5-tuple.
 */
void nat64_queue_icmp6_unreach_tuple(const uip_ip6addr_t *ip6_src,
                                     uint16_t src_port,
                                     const uip_ip4addr_t *ip4_dst,
                                     uint16_t dst_port,
                                     uint8_t ipproto, uint8_t code);

/**
 * \brief Drain the queue of pending ICMPv6 errors into the uIP stack.
 *
 * Called from the platform layer's select-loop set_fd callback,
 * outside the uip_buf processing path, to avoid re-entrancy.
 */
void nat64_flush_icmp6(void);

/** @} */

/** @} */ /* end of \defgroup nat64 */

#endif /* NAT64_H_ */
