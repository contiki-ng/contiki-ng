# NAT64 for Contiki-NG

## Overview

NAT64 allows IPv6-only IoT devices to communicate with IPv4 servers on
the Internet. The border router translates between the two protocols so
that any 6LoWPAN node can reach IPv4 hosts without running a dual stack.

Contiki-NG includes a built-in NAT64 gateway that runs inside the native
border router. It requires no external software, kernel modules, or
separate DNS servers — just pass `--nat64` when starting the border
router. There is no longer any need for Jool, TAYGA, or another
external NAT64 implementation alongside the border router, and no
separate DNS64 resolver: the border router process handles both.

### Implementation approach and trade-offs

Unlike a traditional NAT64 box, this gateway does **not** translate at
the IPv4/IPv6 packet level. Instead, it terminates each IoT-side flow
inside the border-router process and re-emits the payload over a
regular **host socket** (`AF_INET`, BSD `socket()` API):

- One host UDP socket per active UDP flow.
- One host TCP socket per active TCP flow, terminated on both sides
  (TCP splice proxy).
- One unprivileged ICMP socket (`SOCK_DGRAM`, `IPPROTO_ICMP`) per
  active ping flow.

Practical consequences of this design:

- **The gateway consumes host OS resources per session** (file
  descriptors, kernel socket buffers). A traditional packet-level
  NAT64 keeps only a small in-memory binding entry per flow. The
  number of concurrent sessions is therefore capped — see
  `NAT64_MAX_SESSIONS`, `NAT64_MAX_TCP_SESSIONS` and
  `NAT64_MAX_SESSIONS_PER_NODE` under *Configuration*.
- **No raw IPv4 socket and no privileged kernel features required.**
  The native border router still needs `CAP_NET_ADMIN` for the TUN
  device, but NAT64 itself runs with ordinary user-level sockets
  (and with the unprivileged ICMP feature for ping).
- **Outbound, client-initiated flows only.** IPv4 hosts cannot
  initiate connections back to IoT nodes through the gateway (no
  V4 INIT bindings, no hairpinning).
- **DNS payloads are rewritten in place** (DNS64); all other UDP/TCP
  payloads are forwarded opaquely, so DTLS works end-to-end through
  the gateway.
- **TCP throughput is intentionally throttled** to small ACK-paced
  segments so each segment fits a single 802.15.4 frame without
  6LoWPAN fragmentation. This keeps lossy-link delivery robust at
  the cost of bulk-data throughput. See `NAT64_TCP_SEGMENT_SIZE`.

For a full RFC 6146 compliance matrix and a list of intentionally
omitted features (hairpinning, full TCP state machine, configurable
per-protocol timeouts, ...), see `os/services/nat64/README.md`.

## How it works

IPv4 addresses are embedded in IPv6 addresses using the well-known NAT64
prefix `64:ff9b::/96` (RFC 6052). For example, the IPv4 address `8.8.8.8`
is represented as `64:ff9b::808:808`.

When an IoT node sends a packet to a `64:ff9b::` destination, the border
router:

1. Extracts the IPv4 address from the destination.
2. Opens a kernel socket (UDP or TCP) to the IPv4 server.
3. Forwards the payload.
4. Translates the response back into an IPv6 packet and delivers it to
   the IoT node.

DNS queries are handled automatically: the built-in DNS64 translator
rewrites AAAA queries to A queries on the outbound path, and synthesizes
AAAA records with the NAT64 prefix from A records in the response.

## Quick start

### 1. Build the border router

```bash
cd examples/rpl-border-router
make TARGET=native
```

### 2. Start the border router with NAT64

For a serial-connected IoT network:

```bash
sudo ./build/native/border-router.native --nat64 -s /dev/ttyUSB0 fd00::1/64
```

For use with the Cooja simulator:

```bash
sudo ./build/native/border-router.native --nat64 -a localhost -p 60001 fd00::1/64
```

The `--nat64` flag enables the NAT64 gateway. The last argument sets the
IPv6 prefix for the IoT network.

### 3. Configure IoT nodes

IoT nodes need no special configuration for UDP. To use DNS resolution,
configure the DNS server address to point to an IPv4 DNS server via the
NAT64 prefix. For example, to use Google DNS (`8.8.8.8`):

```c
#define RESOLV_CONF_SUPPORTS_MDNS 0

#include "net/ipv6/uip-nameserver.h"
#include "net/ipv6/ip64-addr.h"

uip_ipaddr_t dns_server;
/* 64:ff9b::808:808 = 8.8.8.8 via NAT64 */
uip_nat64addr(&dns_server, 8, 8, 8, 8);
uip_nameserver_update(&dns_server, UIP_NAMESERVER_INFINITE_LIFETIME);
```

The node can then use `resolv_query()` to look up hostnames. The border
router's DNS64 translator ensures that the responses contain
NAT64-prefixed IPv6 addresses.

## Supported protocols

| Protocol | Support |
|----------|---------|
| UDP | Outbound client-initiated flows. CoAP, DNS, and other UDP payloads are forwarded transparently, except DNS on port 53, which is rewritten by DNS64. UDP payloads are not segmented by NAT64. |
| TCP | Outbound client-initiated flows. The border router runs a TCP splice proxy that handles connection setup, data forwarding, retransmission toward the IoT node, and teardown. |
| ICMP | Echo Request/Reply (ping) only, via Linux unprivileged ICMP sockets. Destination Unreachable errors are synthesized toward the IoT node when packets cannot be delivered. Other ICMP types are not translated. |

### End-to-end security and application protocols

The gateway forwards UDP and TCP payloads opaquely (the only payload it
ever rewrites is DNS on port 53), so end-to-end security between the
IoT node and the IPv4 server works without modification:

- **DTLS** works over the UDP path. Note two operational caveats:
    - The gateway does not segment UDP. Large handshake flights (e.g.,
      certificate-based ciphersuites) rely on 6LoWPAN fragmentation
      and can be slow on lossy links. PSK or raw-public-key
      ciphersuites keep flights small.
    - A UDP session that is idle longer than `NAT64_SESSION_TIMEOUT`
      (default 5 minutes) loses its kernel-side binding. To keep a
      DTLS association alive across long idle periods, use **DTLS
      Connection ID (RFC 9146)** or send keepalives within the
      timeout window.
- **MQTT** and other long-lived TCP sessions must send keepalives
  within `NAT64_SESSION_TIMEOUT`, otherwise the session is reaped.

## Security model

The gateway is designed to sit between an untrusted 6LoWPAN network
and the upstream IPv4 Internet. The following defences are enforced
unconditionally:

- **Forbidden IPv4 destinations** are rejected before any socket is
  opened. Loopback, RFC 1918 private ranges, link-local, multicast,
  shared-address space, documentation prefixes, and other special-use
  ranges (RFC 5735 / RFC 6890) cannot be reached via NAT64. This
  prevents a compromised IoT node from using the border router as a
  proxy to attack the local network.
- **Source-prefix validation** (RFC 6146 §3.5) drops IPv6 packets
  whose source already matches the NAT64 prefix, preventing routing
  loops.
- **Per-node session cap** (`NAT64_MAX_SESSIONS_PER_NODE`, default 8)
  limits how many concurrent sessions a single IoT node can hold,
  bounding resource consumption from a misbehaving or hostile node.
- **UDP sockets are connected to the peer** so the kernel filters
  incoming datagrams by source address. This blocks spoofed responses
  (a DNS cache-poisoning vector) from being forwarded into the
  6LoWPAN network.
- **TCP initial sequence numbers** are generated per RFC 6528 using
  HMAC-SHA-256 keyed with a secret read from `/dev/urandom` at
  startup. The border router refuses to start if `/dev/urandom` is
  unavailable rather than fall back to a predictable key.

## Example application

The `examples/nat64/` directory contains a demo that performs a DNS
lookup, sends UDP probes, and performs one HTTP GET request through
NAT64:

```bash
# In one terminal, start Cooja with the NAT64 simulation:
cd examples/nat64
make nat64-dns-cooja.csc

# In another terminal, start the border router:
sudo ./run-nat64-br.sh
```

See `examples/nat64/nat64-dns-client.c` for the application code.

## Configuration

### Build-time options

These can be set in `project-conf.h`:

| Option | Default | Description |
|--------|---------|-------------|
| `UIP_CONF_TCP` | 0 | Set to 1 to enable TCP (required for HTTP). |
| `UIP_CONF_RESOLV_ENTRIES` | 1 | Number of DNS cache entries. |
| `RESOLV_CONF_SUPPORTS_MDNS` | 1 | Set to 0 when using only unicast DNS via NAT64. |

### Gateway tuning

These are compile-time defines for the border router (set via `CFLAGS`
or in the border router's project configuration):

| Option | Default | Description |
|--------|---------|-------------|
| `NAT64_MAX_SESSIONS` | 128 | Maximum concurrent NAT64 sessions. |
| `NAT64_MAX_TCP_SESSIONS` | 16 | Maximum concurrent TCP connections. |
| `NAT64_MAX_SESSIONS_PER_NODE` | 8 | Per-node session limit (DoS protection). |
| `NAT64_SESSION_TIMEOUT` | `5 * 60 * CLOCK_SECOND` | Idle timeout after which a UDP or TCP session binding is reaped. A flat timeout is used for both protocols; long-lived sessions must send keepalives within this window. |
| `NAT64_TCP_SEGMENT_SIZE` | 76 | Size in bytes of each ACK-paced TCP segment delivered to the IoT node. Chosen to fit a single IEEE 802.15.4 frame without 6LoWPAN fragmentation. Larger values improve throughput at the cost of fragment loss on lossy links. |
| `SELECT_CONF_MAX` | 256 | Maximum file descriptors in the select loop. |

### 6LoWPAN compression context (throughput tuning)

NAT64 traffic carries IPv6 source/destination addresses in the
NAT64 prefix (`64:ff9b::/96` by default).  Without an IPHC
compression context for that prefix, each address occupies 16
bytes inline in the compressed 6LoWPAN frame; with a context,
the upper 64 bits are derived from the context and only 8 bytes
are inline.  That saves ~8 bytes per packet, which is enough
headroom to raise `NAT64_TCP_SEGMENT_SIZE` from 76 to ~84 bytes
(about a 10% TCP throughput improvement) and reduces 6LoWPAN
fragmentation pressure on UDP responses and DTLS handshakes.

To enable the context on IoT nodes, include the helper header in each
NAT64-using project:

```c
/* In the IoT node's project-conf.h: */
#include "services/nat64/nat64-6lowpan.h"
```

The native border router picks up the matching context automatically:
`examples/rpl-border-router/project-conf.h` includes the same
header whenever the border-router build includes the NAT64 module
(`BUILD_WITH_NAT64` is defined by `os/services/nat64/module-macros.h`).

Both ends MUST agree on the prefix bytes and the context number.
If only one side is configured, decompression on the other side
silently drops NAT64 frames — there is no error message at the
application layer.  This compile-time symmetric requirement is a
limitation of the current Contiki-NG 6LoWPAN implementation.

> **Future work:** once Contiki-NG implements RFC 6775 §4.2
> (6LoWPAN Context Option in Router Advertisements), the BR will
> advertise the context dynamically and IoT nodes will pick it up
> over the air, removing the recompile-everything-on-both-sides
> requirement.

If your deployment uses a non-standard NAT64 prefix configured
via `ip64_addr_set_prefix()`, override `NAT64_6LOWPAN_PREFIX_BYTES`
to match before including the header — both ends must use the
same value.

## Troubleshooting

- **`--nat64` is not accepted by the border router.** Make sure you are
  running a native border-router build that includes
  `os/services/nat64/native`. The standard native
  `examples/rpl-border-router` build does this automatically.
- **DNS lookups work, but NAT64 packets disappear on the 6LoWPAN side.**
  Check that the border router and every NAT64-using IoT node agree on
  the same IPHC context. For the default prefix, include
  `services/nat64/nat64-6lowpan.h` in the IoT node project; the native
  border router includes the matching context automatically when NAT64
  is compiled in.
- **Ping through NAT64 fails.** ICMP Echo uses Linux unprivileged ICMP
  sockets. The running user must either have the required capability or
  be permitted by `net.ipv4.ping_group_range`.
- **Private, loopback, link-local, or documentation IPv4 addresses do
  not work.** This is intentional gateway policy. Special-use IPv4
  ranges are rejected before a host socket is opened. Loopback can be
  enabled only for tests by building the native border router with
  `NAT64_ALLOW_LOOPBACK=1`.
- **Long-lived UDP or TCP sessions stop after being idle.** NAT64
  reaps idle sessions after `NAT64_SESSION_TIMEOUT` (5 minutes by
  default). Use application keepalives, or for DTLS use Connection ID
  or reconnect after idle periods.

## Standards compliance

The implementation follows RFC 6146 (Stateful NAT64) for the features
relevant to an IoT border router. See `os/services/nat64/README.md` for
a detailed compliance analysis, including which RFC requirements are
implemented and which are intentionally omitted with rationale.

Key RFCs:

- **RFC 6146** — Stateful NAT64 (core translation mechanism)
- **RFC 6147** — DNS64 (DNS query/response translation)
- **RFC 6052** — IPv6 addressing of IPv4/IPv6 translators (NAT64 prefix)
- **RFC 6528** — Defending against sequence number attacks (TCP ISN generation)
- **RFC 5735 / RFC 6890** — Special-use IPv4 address registries (forbidden destinations)
- **RFC 9146** — DTLS Connection ID (recommended for long-lived DTLS associations)
