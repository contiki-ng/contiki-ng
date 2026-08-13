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
 *         NAT64 socket-based IPv4 forwarding for the native platform.
 *
 *         Implements the platform layer (nat64-platform.h) using BSD
 *         sockets on Linux/macOS.  Each NAT64 session owns one
 *         non-blocking socket (UDP, TCP, or unprivileged ICMP) that
 *         is registered with the native main-loop select callback.
 *         Inbound data is delivered to the protocol-agnostic core via
 *         ::nat64_udp_input / ::nat64_tcp_data_in / ::nat64_icmp_input,
 *         and socket-level errors are translated into ICMPv6
 *         Destination Unreachable codes returned to the IoT node.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"
#include "nat64.h"
#include "nat64-platform.h"
#include "nat64-tcp.h"
#include "sys/platform.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "NAT64"
#define LOG_LEVEL LOG_LEVEL_INFO

#ifndef NAT64_MAX_SESSIONS
#define NAT64_MAX_SESSIONS 128
#endif

#ifndef NAT64_SESSION_TIMEOUT
#define NAT64_SESSION_TIMEOUT (5 * 60 * CLOCK_SECOND)
#endif

#define NAT64_PRIO CONTIKI_VERBOSE_PRIO + 40

#ifndef NAT64_MAX_SESSIONS_PER_NODE
#define NAT64_MAX_SESSIONS_PER_NODE 8
#endif

static struct nat64_session sessions[NAT64_MAX_SESSIONS];

/* Whether NAT64 is active. Off by default and enabled with --nat64, unless a
 * build turns it on at compile time (the standalone translator module does). */
#ifndef NAT64_DEFAULT_ENABLED
#define NAT64_DEFAULT_ENABLED 0
#endif
static bool nat64_enabled = NAT64_DEFAULT_ENABLED;

/*---------------------------------------------------------------------------*/
/**
 * \brief Map a Linux errno to an ICMPv6 Destination Unreachable code.
 * \param err A `errno` value reported by a connect()/send()/recv()
 *            failure on an IPv4 socket.
 * \return One of the NAT64_ICMP6_* codes (RFC 4443 §3.1).
 *
 * Used by the platform layer to translate socket-level failures into
 * the ICMPv6 errors returned to the IoT node via
 * ::nat64_queue_icmp6_unreach_tuple.  Unrecognized errors fall back
 * to "no route to destination".
 */
static uint8_t
errno_to_icmp6_code(int err)
{
  switch(err) {
  case ECONNREFUSED:
    return NAT64_ICMP6_PORT;
  case EHOSTUNREACH:
  case ETIMEDOUT:
    return NAT64_ICMP6_ADDR;
  case ENETUNREACH:
    return NAT64_ICMP6_NOROUTE;
  case EACCES:
  case EPERM:
    return NAT64_ICMP6_ADMIN;
  default:
    return NAT64_ICMP6_NOROUTE;
  }
}

/*---------------------------------------------------------------------------*/
/* Session helpers.                                                          */
/*---------------------------------------------------------------------------*/

static void
close_session(struct nat64_session *s)
{
  if(s->proto == NAT64_PROTO_TCP) {
    nat64_tcp_free_seqstate(s);
  }
  if(s->fd >= 0) {
    select_set_callback(s->fd, NULL);
    close(s->fd);
    s->fd = -1;
  }
  s->active = false;
  s->proto = NAT64_PROTO_NONE;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Reap an expired session, notifying its peer if applicable.
 * \param s The session whose expiry timer has fired.
 *
 * For ESTABLISHED TCP sessions this synthesizes a FIN toward the IoT
 * node before tearing down, so the IoT-side TCP layer doesn't keep a
 * zombie connection until its own keepalive fires.
 */
static void
expire_session(struct nat64_session *s)
{
  if(s->proto == NAT64_PROTO_TCP &&
     s->tcp_state == NAT64_TCP_ESTABLISHED) {
    nat64_tcp_closed(s);
  }
  close_session(s);
}
/*---------------------------------------------------------------------------*/
static struct nat64_session *
find_session(enum nat64_session_proto proto,
             const uip_ip6addr_t *ip6_src, uint16_t srcport,
             const uip_ip4addr_t *dst, uint16_t dstport)
{
  unsigned i;
  for(i = 0; i < NAT64_MAX_SESSIONS; i++) {
    struct nat64_session *s = &sessions[i];
    if(s->active &&
       s->proto == proto &&
       s->ip6_peer_port == srcport &&
       s->ip4_remote_port == dstport &&
       uip_ip6addr_cmp(&s->ip6_peer, ip6_src) &&
       uip_ip4addr_cmp(&s->ip4_remote, dst)) {
      if(timer_expired(&s->expiry)) {
        expire_session(s);
        return NULL;
      }
      return s;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static unsigned
count_node_sessions(const uip_ip6addr_t *ip6_src)
{
  unsigned i, count = 0;
  for(i = 0; i < NAT64_MAX_SESSIONS; i++) {
    if(sessions[i].active &&
       !timer_expired(&sessions[i].expiry) &&
       uip_ip6addr_cmp(&sessions[i].ip6_peer, ip6_src)) {
      count++;
    }
  }
  return count;
}
/*---------------------------------------------------------------------------*/
static struct nat64_session *
alloc_session(const uip_ip6addr_t *ip6_src)
{
  unsigned i;

  if(count_node_sessions(ip6_src) >= NAT64_MAX_SESSIONS_PER_NODE) {
    LOG_WARN("Per-node session limit reached (%u)\n",
             NAT64_MAX_SESSIONS_PER_NODE);
    return NULL;
  }

  for(i = 0; i < NAT64_MAX_SESSIONS; i++) {
    if(!sessions[i].active) {
      return &sessions[i];
    }
    if(timer_expired(&sessions[i].expiry)) {
      expire_session(&sessions[i]);
      return &sessions[i];
    }
  }
  LOG_WARN("Session table full\n");
  return NULL;
}
/*---------------------------------------------------------------------------*/
static void
fill_session(struct nat64_session *s, enum nat64_session_proto proto,
             const uip_ip6addr_t *ip6_src, uint16_t srcport,
             const uip_ip4addr_t *dst, uint16_t dstport)
{
  s->proto = proto;
  uip_ip6addr_copy(&s->ip6_peer, ip6_src);
  s->ip6_peer_port = srcport;
  memcpy(&s->ip4_remote, dst, sizeof(uip_ip4addr_t));
  s->ip4_remote_port = dstport;
  s->active = true;
  timer_set(&s->expiry, NAT64_SESSION_TIMEOUT);
}
/*---------------------------------------------------------------------------*/
static bool
register_fd(struct nat64_session *s);

static struct sockaddr_in
make_addr(const uip_ip4addr_t *ip, uint16_t port)
{
  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  memcpy(&sa.sin_addr, ip, sizeof(uip_ip4addr_t));
  return sa;
}

/*---------------------------------------------------------------------------*/
/* Select callbacks.                                                         */
/*---------------------------------------------------------------------------*/

static void
handle_tcp_connect_complete(struct nat64_session *s)
{
  int err = 0;
  socklen_t errlen = sizeof(err);

  if(getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &err, &errlen) < 0 || err != 0) {
    int e = err ? err : errno;
    LOG_WARN("TCP connect failed: %s\n", strerror(e));
    nat64_queue_icmp6_unreach_tuple(&s->ip6_peer, s->ip6_peer_port,
                                    &s->ip4_remote, s->ip4_remote_port,
                                    IPPROTO_TCP, errno_to_icmp6_code(e));
    close_session(s);
    return;
  }

  LOG_INFO("TCP connected to %u.%u.%u.%u:%u (fd %d)\n",
           s->ip4_remote.u8[0], s->ip4_remote.u8[1],
           s->ip4_remote.u8[2], s->ip4_remote.u8[3],
           s->ip4_remote_port, s->fd);

  s->tcp_state = NAT64_TCP_ESTABLISHED;
  nat64_tcp_established(s);
}
/*---------------------------------------------------------------------------*/
static int
generic_set_fd(fd_set *rset, fd_set *wset)
{
  unsigned i;

  /* Flush deferred TCP ACKs and any pending ICMPv6 errors.  set_fd is
   * called on every main-loop iteration, so this ensures ACKs and
   * errors are delivered promptly even when select() times out with no
   * ready fds. */
  nat64_tcp_flush_acks();
  nat64_flush_icmp6();

  for(i = 0; i < NAT64_MAX_SESSIONS; i++) {
    struct nat64_session *s = &sessions[i];
    if(!s->active || s->fd < 0) {
      continue;
    }
    if(s->proto == NAT64_PROTO_TCP &&
       s->tcp_state == NAT64_TCP_CONNECTING) {
      FD_SET(s->fd, wset);
    } else if(s->proto == NAT64_PROTO_TCP &&
              nat64_tcp_has_pending_data(s)) {
      /* The TCP proxy owns only one server-to-IoT buffer.  Suppressing
       * reads here keeps unread bytes in the kernel until the IoT node
       * ACKs the current chunk. */
    } else {
      FD_SET(s->fd, rset);
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
generic_handle_fd(fd_set *rset, fd_set *wset)
{
  unsigned i;
  for(i = 0; i < NAT64_MAX_SESSIONS; i++) {
    struct nat64_session *s = &sessions[i];
    if(!s->active || s->fd < 0) {
      continue;
    }

    if(timer_expired(&s->expiry)) {
      expire_session(s);
      continue;
    }

    if(s->proto == NAT64_PROTO_TCP &&
       s->tcp_state == NAT64_TCP_CONNECTING &&
       FD_ISSET(s->fd, wset)) {
      handle_tcp_connect_complete(s);
      continue;
    }

    if(!FD_ISSET(s->fd, rset)) {
      continue;
    }

    if(s->proto == NAT64_PROTO_TCP &&
       s->tcp_state == NAT64_TCP_ESTABLISHED) {
      uint8_t buf[1500];
      ssize_t n = recv(s->fd, buf, sizeof(buf), 0);
      if(n > 0) {
        LOG_INFO("TCP recv %zd bytes from server (fd %d)\n", n, s->fd);
        nat64_tcp_data_in(s, buf, (uint16_t)n);
        timer_set(&s->expiry, NAT64_SESSION_TIMEOUT);
      } else if(n == 0) {
        LOG_INFO("TCP server closed connection (fd %d)\n", s->fd);
        nat64_tcp_closed(s);
        s->tcp_state = NAT64_TCP_CLOSING;
        if(nat64_tcp_peer_fin_received(s)) {
          /* IoT side already FIN'd — both halves done, reap now. */
          LOG_INFO("TCP both sides FIN'd, destroying session\n");
          close_session(s);
        }
      } else if(errno != EAGAIN && errno != EWOULDBLOCK) {
        LOG_ERR("TCP recv error (fd %d): %s\n", s->fd, strerror(errno));
        nat64_tcp_closed(s);
        s->tcp_state = NAT64_TCP_CLOSING;
        if(nat64_tcp_peer_fin_received(s)) {
          LOG_INFO("TCP both sides done, destroying session\n");
          close_session(s);
        }
      }
    } else if(s->proto == NAT64_PROTO_UDP) {
      uint8_t buf[1500];
      ssize_t n = recv(s->fd, buf, sizeof(buf), 0);
      if(n > 0) {
        nat64_udp_input(s, buf, (uint16_t)n);
        timer_set(&s->expiry, NAT64_SESSION_TIMEOUT);
      } else if(n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        int e = errno;

        LOG_ERR("UDP recvfrom error (fd %d): %s\n", s->fd, strerror(e));
        nat64_queue_icmp6_unreach_tuple(&s->ip6_peer, s->ip6_peer_port,
                                        &s->ip4_remote, s->ip4_remote_port,
                                        IPPROTO_UDP, errno_to_icmp6_code(e));
        close_session(s);
      }
    } else if(s->proto == NAT64_PROTO_ICMP) {
      uint8_t buf[256];
      ssize_t n = recv(s->fd, buf, sizeof(buf), 0);
      if(n > 0) {
        nat64_icmp_input(s, buf, (uint16_t)n);
        timer_set(&s->expiry, NAT64_SESSION_TIMEOUT);
      } else if(n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        LOG_ERR("ICMP recv error (fd %d): %s\n", s->fd, strerror(errno));
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static const struct select_callback nat64_select_cb = {
  generic_set_fd,
  generic_handle_fd,
};
/*---------------------------------------------------------------------------*/
static bool
register_fd(struct nat64_session *s)
{
  if(fcntl(s->fd, F_SETFL, O_NONBLOCK) < 0) {
    /* If the socket stays blocking, a single slow IPv4 server can
     * stall the entire main loop on the next send/recv.  Refuse the
     * session rather than risk that. */
    LOG_ERR("fcntl(F_SETFL, O_NONBLOCK) failed for fd %d: %s\n",
            s->fd, strerror(errno));
    close(s->fd);
    s->fd = -1;
    s->active = false;
    return false;
  }
  if(!select_set_callback(s->fd, &nat64_select_cb)) {
    LOG_ERR("select_set_callback failed for fd %d\n", s->fd);
    close(s->fd);
    s->fd = -1;
    s->active = false;
    return false;
  }
  return true;
}

/*---------------------------------------------------------------------------*/
/* Platform API.                                                             */
/*---------------------------------------------------------------------------*/

int
nat64_platform_udp_send(const uip_ip4addr_t *dst, uint16_t dstport,
                        const uip_ip6addr_t *ip6_src, uint16_t srcport,
                        const uint8_t *payload, uint16_t len)
{
  struct nat64_session *s;
  ssize_t sent;

  s = find_session(NAT64_PROTO_UDP, ip6_src, srcport, dst, dstport);
  if(s == NULL) {
    s = alloc_session(ip6_src);
    if(s == NULL) {
      nat64_queue_icmp6_unreach_tuple(ip6_src, srcport, dst, dstport,
                                      IPPROTO_UDP, NAT64_ICMP6_ADMIN);
      return -1;
    }
    s->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(s->fd < 0) {
      LOG_ERR("socket(DGRAM): %s\n", strerror(errno));
      return -1;
    }
    fill_session(s, NAT64_PROTO_UDP, ip6_src, srcport, dst, dstport);
    if(!register_fd(s)) {
      return -1;
    }
    /* Connect the UDP socket so the kernel filters incoming packets
     * by source address, preventing spoofed responses. */
    struct sockaddr_in peer = make_addr(dst, dstport);
    if(connect(s->fd, (struct sockaddr *)&peer, sizeof(peer)) < 0) {
      int e = errno;
      LOG_ERR("UDP connect: %s\n", strerror(e));
      nat64_queue_icmp6_unreach_tuple(ip6_src, srcport, dst, dstport,
                                      IPPROTO_UDP, errno_to_icmp6_code(e));
      close_session(s);
      return -1;
    }
    LOG_DBG("New UDP session fd %d\n", s->fd);
  }

  timer_set(&s->expiry, NAT64_SESSION_TIMEOUT);

  sent = send(s->fd, payload, len, 0);
  if(sent < 0) {
    int e = errno;
    LOG_ERR("sendto: %s\n", strerror(e));
    nat64_queue_icmp6_unreach_tuple(ip6_src, srcport, dst, dstport,
                                    IPPROTO_UDP, errno_to_icmp6_code(e));
    return -1;
  }
  return (int)sent;
}
/*---------------------------------------------------------------------------*/
struct nat64_session *
nat64_platform_tcp_connect(const uip_ip4addr_t *dst, uint16_t dstport,
                           const uip_ip6addr_t *ip6_src, uint16_t srcport,
                           uint32_t peer_isn)
{
  struct nat64_session *s;
  int ret;

  s = find_session(NAT64_PROTO_TCP, ip6_src, srcport, dst, dstport);
  if(s != NULL) {
    return s;
  }

  s = alloc_session(ip6_src);
  if(s == NULL) {
    nat64_queue_icmp6_unreach_tuple(ip6_src, srcport, dst, dstport,
                                    IPPROTO_TCP, NAT64_ICMP6_ADMIN);
    return NULL;
  }

  s->fd = socket(AF_INET, SOCK_STREAM, 0);
  if(s->fd < 0) {
    LOG_ERR("socket(STREAM): %s\n", strerror(errno));
    return NULL;
  }

  fill_session(s, NAT64_PROTO_TCP, ip6_src, srcport, dst, dstport);
  s->peer_isn = peer_isn;
  s->tcp_state = NAT64_TCP_CONNECTING;

  if(!register_fd(s)) {
    return NULL;
  }

  struct sockaddr_in dest = make_addr(dst, dstport);
  ret = connect(s->fd, (struct sockaddr *)&dest, sizeof(dest));
  if(ret < 0 && errno != EINPROGRESS) {
    int e = errno;
    LOG_ERR("connect: %s\n", strerror(e));
    nat64_queue_icmp6_unreach_tuple(ip6_src, srcport, dst, dstport,
                                    IPPROTO_TCP, errno_to_icmp6_code(e));
    close_session(s);
    return NULL;
  }

  if(ret == 0) {
    handle_tcp_connect_complete(s);
  }

  LOG_DBG("TCP connecting fd %d to %u.%u.%u.%u:%u\n",
          s->fd,
          dst->u8[0], dst->u8[1], dst->u8[2], dst->u8[3], dstport);
  return s;
}
/*---------------------------------------------------------------------------*/
int
nat64_platform_tcp_send(struct nat64_session *s,
                        const uint8_t *data, uint16_t len)
{
  ssize_t sent;

  if(s == NULL || s->tcp_state != NAT64_TCP_ESTABLISHED) {
    return -1;
  }

  timer_set(&s->expiry, NAT64_SESSION_TIMEOUT);

  sent = send(s->fd, data, len, 0);
  if(sent < 0) {
    if(errno == EAGAIN || errno == EWOULDBLOCK) {
      /* NAT64 does not own a copy of IoT-to-server bytes.  Returning 0
       * leaves peer_next unchanged, so the IoT TCP stack remains the
       * source of truth and retransmits the unsent data on its RTO. */
      LOG_WARN("TCP send would block (fd %d), IoT will retransmit\n",
               s->fd);
      return 0;
    }
    LOG_ERR("TCP send error (fd %d): %s\n", s->fd, strerror(errno));
    return -1;
  }
  LOG_INFO("TCP sent %zd bytes to server (fd %d)\n", sent, s->fd);
  return (int)sent;
}
/*---------------------------------------------------------------------------*/
void
nat64_platform_tcp_close(struct nat64_session *s)
{
  if(s == NULL) {
    return;
  }
  LOG_DBG("TCP shutdown(WR) fd %d\n", s->fd);
  /* Half-close: signal EOF to the IPv4 server but keep the read side
   * open so any remaining server->IoT data can still be delivered.
   * The session transitions to CLOSING only when the server itself
   * closes (recv() returns 0 in generic_handle_fd) or when an explicit
   * teardown occurs. */
  shutdown(s->fd, SHUT_WR);
}
/*---------------------------------------------------------------------------*/
void
nat64_platform_tcp_destroy(struct nat64_session *s)
{
  if(s == NULL) {
    return;
  }
  LOG_DBG("TCP destroy fd %d\n", s->fd);
  close_session(s);
}
/*---------------------------------------------------------------------------*/
void
nat64_platform_tcp_abort(struct nat64_session *s)
{
  if(s == NULL) {
    return;
  }
  if(s->fd >= 0) {
    /* SO_LINGER with l_linger=0 makes the subsequent close() emit a
     * TCP RST instead of a graceful FIN, so the upstream server sees
     * the connection abort directly. */
    struct linger lin = { .l_onoff = 1, .l_linger = 0 };
    setsockopt(s->fd, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));
  }
  LOG_DBG("TCP abort fd %d\n", s->fd);
  close_session(s);
}
/*---------------------------------------------------------------------------*/
int
nat64_platform_icmp_send(const uip_ip4addr_t *dst,
                         const uip_ip6addr_t *ip6_src, uint16_t identifier,
                         const uint8_t *icmp_pkt, uint16_t icmp_len)
{
  struct nat64_session *s;
  ssize_t sent;

  /* Sessions are keyed on (ip6_src, identifier, dst, 0).  ip4_remote_port
   * is unused for ICMP and stored as 0; ip6_peer_port stores the
   * ICMPv6 Echo identifier. */
  s = find_session(NAT64_PROTO_ICMP, ip6_src, identifier, dst, 0);
  if(s == NULL) {
    s = alloc_session(ip6_src);
    if(s == NULL) {
      nat64_queue_icmp6_unreach_tuple(ip6_src, identifier, dst, 0,
                                      IPPROTO_ICMPV6, NAT64_ICMP6_ADMIN);
      return -1;
    }

    /* Open an unprivileged ICMP socket.  Requires either CAP_NET_RAW
     * or the running GID to be in net.ipv4.ping_group_range. */
    s->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if(s->fd < 0) {
      int e = errno;
      LOG_ERR("socket(ICMP): %s\n", strerror(e));
      if(e == EACCES || e == EPERM) {
        LOG_ERR("Hint: add the running user's GID to "
                "net.ipv4.ping_group_range\n");
      }
      s->active = false;
      nat64_queue_icmp6_unreach_tuple(ip6_src, identifier, dst, 0,
                                      IPPROTO_ICMPV6,
                                      errno_to_icmp6_code(e));
      return -1;
    }

    fill_session(s, NAT64_PROTO_ICMP, ip6_src, identifier, dst, 0);

    if(!register_fd(s)) {
      return -1;
    }

    /* Connect so the kernel filters ICMP replies by source address. */
    struct sockaddr_in peer = make_addr(dst, 0);
    if(connect(s->fd, (struct sockaddr *)&peer, sizeof(peer)) < 0) {
      int e = errno;
      LOG_ERR("ICMP connect: %s\n", strerror(e));
      nat64_queue_icmp6_unreach_tuple(ip6_src, identifier, dst, 0,
                                      IPPROTO_ICMPV6,
                                      errno_to_icmp6_code(e));
      close_session(s);
      return -1;
    }
    LOG_DBG("New ICMP session fd %d id=%u\n", s->fd, identifier);
  }

  timer_set(&s->expiry, NAT64_SESSION_TIMEOUT);

  sent = send(s->fd, icmp_pkt, icmp_len, 0);
  if(sent < 0) {
    int e = errno;
    LOG_ERR("ICMP send: %s\n", strerror(e));
    nat64_queue_icmp6_unreach_tuple(ip6_src, identifier, dst, 0,
                                    IPPROTO_ICMPV6, errno_to_icmp6_code(e));
    return -1;
  }
  return (int)sent;
}
/*---------------------------------------------------------------------------*/
static bool
read_urandom(void *buf, size_t len)
{
  int fd = open("/dev/urandom", O_RDONLY);
  if(fd < 0) {
    LOG_ERR("Failed to open /dev/urandom: %s\n", strerror(errno));
    return false;
  }
  ssize_t n = read(fd, buf, len);
  close(fd);
  if(n != (ssize_t)len) {
    LOG_ERR("Short read from /dev/urandom\n");
    return false;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
bool
nat64_platform_init(void)
{
  unsigned i;
  uint8_t isn_key[16];

  memset(sessions, 0, sizeof(sessions));
  for(i = 0; i < NAT64_MAX_SESSIONS; i++) {
    sessions[i].fd = -1;
  }

  if(!read_urandom(isn_key, sizeof(isn_key))) {
    LOG_ERR("Cannot seed ISN secret — /dev/urandom unavailable\n");
    return false;
  }
  nat64_tcp_set_isn_secret(isn_key);
  memset(isn_key, 0, sizeof(isn_key));

  nat64_activate();
  LOG_INFO("Socket-based NAT64 initialized (%u max sessions)\n",
           NAT64_MAX_SESSIONS);
  return true;
}
/*---------------------------------------------------------------------------*/
static int
nat64_option_callback(const char *optarg)
{
  nat64_enabled = true;
  return 0;
}
CONTIKI_OPTION(NAT64_PRIO, { "nat64", no_argument, NULL, 0 },
               nat64_option_callback,
               "Enable NAT64 gateway (socket-based, no TUN device needed)\n");
/*---------------------------------------------------------------------------*/
bool
nat64_is_enabled(void)
{
  return nat64_enabled;
}
/*---------------------------------------------------------------------------*/
/** @} */
