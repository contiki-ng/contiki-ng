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
 *         NAT64 platform interface — socket-based.
 *
 *         Defines the session structure shared between the
 *         protocol-agnostic core and the platform layer, plus the
 *         small set of "send to IPv4" entry points the core invokes
 *         to forward packets out.  The session struct is exposed
 *         (rather than opaque) so that nat64.c and nat64-tcp.c can
 *         read the address/port fields directly without accessor
 *         overhead, while the platform layer remains the sole owner
 *         of the file descriptor and connection state.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#ifndef NAT64_PLATFORM_H_
#define NAT64_PLATFORM_H_

#include <stdbool.h>
#include <stdint.h>
#include "net/ipv6/uip.h"
#include "sys/timer.h"

/**
 * \brief Transport protocol tracked by a NAT64 session.
 *
 * For NAT64_PROTO_ICMP, the session's ip6_peer_port field stores the
 * ICMPv6 Echo identifier and ip4_remote_port is unused.
 */
enum nat64_session_proto {
  NAT64_PROTO_NONE,
  NAT64_PROTO_UDP,
  NAT64_PROTO_TCP,
  NAT64_PROTO_ICMP,
};

/**
 * \brief TCP connection state within the NAT64 splice proxy.
 */
enum nat64_tcp_state {
  NAT64_TCP_CONNECTING,   /**< Non-blocking connect() in progress. */
  NAT64_TCP_ESTABLISHED,  /**< Connection open, data can flow. */
  NAT64_TCP_CLOSING,      /**< Half-closed (SHUT_WR sent). */
};

/**
 * \brief A NAT64 session binding an IoT node's IPv6 flow to an IPv4 socket.
 */
struct nat64_session {
  bool active;                        /**< Session slot in use. */
  enum nat64_session_proto proto;     /**< UDP or TCP. */
  int fd;                             /**< IPv4 socket file descriptor. */
  uip_ip6addr_t ip6_peer;            /**< IoT node's IPv6 address. */
  uint16_t ip6_peer_port;            /**< IoT node's transport port. */
  uip_ip4addr_t ip4_remote;          /**< IPv4 server address. */
  uint16_t ip4_remote_port;          /**< IPv4 server port. */
  uint32_t peer_isn;                  /**< IoT node's ISN (TCP only). */
  enum nat64_tcp_state tcp_state;     /**< TCP connection state. */
  struct timer expiry;                /**< Session lifetime timer. */
};

/**
 * \brief Initialize the platform layer.
 * \return true on success, false on failure.
 *
 * On success, the NAT64 core has been activated and the platform is
 * ready to create transport sessions for translated packets.
 */
bool nat64_platform_init(void);

/**
 * \brief Check whether the NAT64 gateway has been enabled at runtime.
 * \return true if the user passed the platform's NAT64 enable option
 *         (e.g., `--nat64` on the native border router), false otherwise.
 *
 * Implemented by each platform layer alongside the option callback that
 * sets the underlying flag.
 */
bool nat64_is_enabled(void);

/**
 * \brief Forward a UDP payload to an IPv4 server.
 * \param dst     IPv4 destination address.
 * \param dstport Destination port (host byte order).
 * \param ip6_src IoT node's IPv6 source address (used for session lookup).
 * \param srcport Source port (host byte order).
 * \param payload UDP payload bytes.
 * \param len     Payload length.
 * \return Number of bytes sent, or -1 on error.
 *
 * Creates a new session and UDP socket if no matching session exists.
 */
int nat64_platform_udp_send(const uip_ip4addr_t *dst, uint16_t dstport,
                            const uip_ip6addr_t *ip6_src, uint16_t srcport,
                            const uint8_t *payload, uint16_t len);

/**
 * \brief Initiate a TCP connection to an IPv4 server.
 * \param dst      IPv4 destination address.
 * \param dstport  Destination port (host byte order).
 * \param ip6_src  IoT node's IPv6 source address.
 * \param srcport  Source port (host byte order).
 * \param peer_isn The IoT node's initial sequence number.
 * \return The session, or NULL on failure.
 *
 * The returned session may still be connecting.  The platform calls
 * nat64_tcp_established() later, from its event loop, once the IPv4
 * connection is usable.
 */
struct nat64_session *nat64_platform_tcp_connect(
  const uip_ip4addr_t *dst, uint16_t dstport,
  const uip_ip6addr_t *ip6_src, uint16_t srcport,
  uint32_t peer_isn);

/**
 * \brief Send data on an established TCP session.
 * \param s    The session (must be in ESTABLISHED state).
 * \param data Data to send.
 * \param len  Data length.
 * \return Number of bytes sent, 0 if would block, or -1 on error.
 *
 * A zero return means NAT64 did not retain the payload.  The TCP core
 * must leave its IoT-side acknowledgment point unchanged so the node
 * retransmits the same bytes.
 */
int nat64_platform_tcp_send(struct nat64_session *s,
                            const uint8_t *data, uint16_t len);

/**
 * \brief Half-close a TCP session (send FIN).
 * \param s The session to close.
 */
void nat64_platform_tcp_close(struct nat64_session *s);

/**
 * \brief Fully tear down a TCP session.
 * \param s The session to destroy.
 *
 * Closes the IPv4 socket, releases the per-session sequence state, and
 * frees the platform-layer session slot.  After this call the session
 * pointer is no longer valid.  Use this when both sides have FIN'd and
 * the connection is fully closed; for RST/abort semantics use
 * ::nat64_platform_tcp_abort instead.
 */
void nat64_platform_tcp_destroy(struct nat64_session *s);

/**
 * \brief Abort a TCP session by sending RST upstream.
 * \param s The session to abort.
 *
 * Sets SO_LINGER with a zero linger time so that close() emits a TCP
 * RST instead of a graceful FIN, then tears down the session as in
 * ::nat64_platform_tcp_destroy.  Used when the IoT node sends a RST,
 * so the IPv4 server sees an equivalent abort rather than a delayed
 * graceful close.
 */
void nat64_platform_tcp_abort(struct nat64_session *s);

/**
 * \brief Forward an ICMPv4 Echo Request to an IPv4 destination.
 * \param dst        IPv4 destination address.
 * \param ip6_src    IoT node's IPv6 source address.
 * \param identifier ICMPv6 Echo identifier (host byte order).
 * \param icmp_pkt   ICMPv4 Echo Request bytes (type 8 + code + checksum
 *                   + identifier + sequence + data).
 * \param icmp_len   Length of icmp_pkt in bytes.
 * \return Number of bytes sent, or -1 on error.
 *
 * Allocates a session keyed on (ip6_src, identifier, dst) and a
 * Linux unprivileged ICMP socket (SOCK_DGRAM, IPPROTO_ICMP).  The
 * session receives matching Echo Replies and forwards them via
 * nat64_icmp_input().
 */
int nat64_platform_icmp_send(const uip_ip4addr_t *dst,
                             const uip_ip6addr_t *ip6_src,
                             uint16_t identifier,
                             const uint8_t *icmp_pkt, uint16_t icmp_len);

/** @} */

#endif /* NAT64_PLATFORM_H_ */
