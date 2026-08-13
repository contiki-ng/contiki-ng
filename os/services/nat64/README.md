# NAT64 Gateway for Contiki-NG

A lightweight stateful NAT64 gateway that enables IPv6-only IoT nodes to
communicate with IPv4 servers through a border router.  The design targets
outbound connections from constrained devices and deliberately omits
general-purpose NAT64 features that are irrelevant in a 6LoWPAN/RPL
network.

## Architecture

```
IoT node (IPv6)  ──6LoWPAN──>  Border router  ──IPv4 sockets──>  IPv4 server
                                 (NAT64)
```

The gateway intercepts IPv6 packets whose destination matches the NAT64
prefix (`64:ff9b::/96` by default) and forwards them over native IPv4
sockets.  Responses are translated back into IPv6 packets and injected
into the uIP stack via `tcpip_input()`.

### Module structure

| File | Purpose |
|------|---------|
| `nat64.c` | Core dispatcher — routes packets by protocol |
| `nat64-tcp.c` | TCP splice proxy with sequence number translation |
| `nat64-dns64.c` | Inline DNS64 translation (AAAA/A rewriting) |
| `nat64-platform.h` | Platform abstraction (session, socket API) |
| `native/nat64-sock.c` | Native platform: BSD socket implementation |

### Packet flow

**Outbound (IPv6 to IPv4):**
1. `nat64_output()` extracts the IPv4 destination from the NAT64 prefix
2. UDP: forwarded via a kernel UDP socket; DNS queries are rewritten
   (AAAA to A) by the DNS64 translator
3. TCP: SYN triggers a non-blocking `connect()`; subsequent data is
   spliced through a TCP proxy that maintains per-session sequence state

**Inbound (IPv4 to IPv6):**
1. The platform select loop receives data on IPv4 sockets
2. UDP: `nat64_udp_input()` fabricates an IPv6+UDP packet; DNS responses
   are rewritten (A to AAAA with the NAT64 prefix)
3. TCP: `nat64_tcp_data_in()` buffers server data per-session and
   delivers it in `NAT64_TCP_SEGMENT_SIZE`-byte segments (76 bytes by
   default), each fitting a single IEEE 802.15.4 frame without
   6LoWPAN fragmentation. The next segment is sent only after the IoT
   node ACKs the previous one

## Standards compliance

The primary reference is **RFC 6146** (Stateful NAT64: Network Address
and Protocol Translation from IPv6 Clients to IPv4 Servers).

### Supported features

| RFC requirement | Section | Status | Notes |
|-----------------|---------|--------|-------|
| UDP session handling | 3.5.1 | Implemented | Full outbound support |
| TCP session handling | 3.5.2 | Implemented | Outbound connections; 3-state model (CONNECTING, ESTABLISHED, CLOSING) sufficient for client-initiated flows |
| DNS64 translation | RFC 6147 | Implemented | Inline AAAA/A rewriting with DNS name compression support |
| RFC 6052 address mapping | 3.5.4 | Implemented | Well-known prefix `64:ff9b::/96`; configurable via `ip64_addr_set_prefix()` |
| Source prefix validation | 3.5 | Implemented | Drops IPv6 packets with NAT64 source to prevent routing loops |
| Session timeout | 3.5.1/3.5.2 | Implemented | Flat 5-minute timeout (see rationale below) |
| Fragment handling | 3.4 | Delegated | IPv6 reassembly by uIP; IPv4 fragmentation by kernel |
| ICMP Echo translation | 3.5.3 | Implemented | ICMPv6 Echo Request/Reply forwarded via Linux unprivileged ICMP sockets (`SOCK_DGRAM`, `IPPROTO_ICMP`); requires `net.ipv4.ping_group_range` to include the running user |
| ICMPv6 error synthesis | 3.5.3 | Implemented | Destination Unreachable (Code 0/1/3/4) synthesized toward the IoT node from socket-level errors and gateway policy decisions |

### Intentionally omitted features

The following RFC 6146 features are omitted because they provide no
practical benefit in a 6LoWPAN/RPL network where constrained IoT nodes
initiate outbound connections to IPv4 servers.

| RFC requirement | Section | Level | Rationale for omission |
|-----------------|---------|-------|------------------------|
| ICMP error translation (PTB, Time Exceeded, Parameter Problem) | 3.5.3 | MUST | The kernel-socket architecture handles IPv4 PMTU on the upstream side, and the IoT-side path is bounded by 6LoWPAN's effective MTU well below any plausible IPv4 PMTU.  Time Exceeded and Parameter Problem are diagnostic-only and typically run from the BR side itself.  Echo translation and Dest Unreach synthesis are implemented (see *Supported features* above). |
| Hairpinning | 3.8 | MUST | IoT nodes behind the border router reach each other directly via IPv6/RPL.  No node would address another node through the NAT64 prefix. |
| Inbound connections (V4 INIT) | 3.5.2 | N/A | The gateway is outbound-only by design.  IPv4 hosts cannot initiate connections to IoT nodes. |
| Simultaneous TCP open | 3.5.2.2 | MAY | Requires inbound connection support. |
| Address-dependent filtering | 3.5/5.2 | SHOULD | Only relevant when accepting inbound packets to existing bindings.  Outbound-only design makes this moot. |
| Port allocation policy | 3.5.1.1 | SHOULD | Port range/parity preservation targets carrier-grade NATs for protocols like RTP.  Kernel port assignment is adequate. |
| Full TCP state machine | 3.5.2.2 | MUST | RFC defines 8 states for bidirectional connection tracking.  The 3-state outbound-only model covers all client-initiated flows correctly. |
| Configurable per-protocol timeouts | 4 | SHOULD | RFC recommends UDP ~2 min, TCP established ~2h4min, TCP transitory ~4 min.  A flat 5-minute timeout is a deliberate trade-off: long-lived TCP sessions would waste scarce session slots on constrained gateways. |

### Design trade-offs

**Flat session timeout (5 minutes):**  RFC 6146 Section 4 recommends
TCP_EST of 7440 seconds (~2 hours).  This is impractical for the gateway,
which by default allows up to `NAT64_MAX_SESSIONS` sessions in total
(128) of which `NAT64_MAX_TCP_SESSIONS` (16) may be TCP — a handful of
idle TCP connections at the RFC timeout would exhaust the TCP portion of
the table.  The 5-minute timeout balances session reuse against slot
availability.  IoT applications using long-lived connections (e.g.,
MQTT) send keepalives well within this window.

**TCP splice proxy vs header translation:**  Rather than translating
IPv6/TCP headers to IPv4/TCP headers (as a traditional NAT64 would), the
gateway terminates TCP on both sides and splices the data streams.  This
avoids checksum and sequence number translation across address families
and lets the kernel handle IPv4 TCP congestion control and
retransmission.

**ACK-paced TCP segmentation:**  Server data is buffered per session
and delivered in 76-byte segments (`NAT64_TCP_SEGMENT_SIZE`), each
fitting a single 802.15.4 frame without 6LoWPAN fragmentation.  The
next segment is sent only after the IoT node ACKs the previous one,
and the platform layer suppresses `recv()` from the IPv4 socket while
buffered data is still pending.  Larger segments risk fragment loss
in lossy wireless networks.

**Optional 6LoWPAN compression context:**  `nat64-6lowpan.h`
registers the upper 64 bits of the NAT64 prefix as IPHC context 1,
saving ~8 header bytes per packet.  Both the BR and the IoT nodes
must include the header at compile time — the BR pulls it in
automatically when the NAT64 module is present in the native
border-router build, and IoT projects opt in via
`#include "services/nat64/nat64-6lowpan.h"` in their
`project-conf.h`. IPHC's SAM modes cap inline-byte counts at
0/2/8/16, so even a longer (96-bit) context cannot push the
saving further without a non-standard extension.  Once Contiki-NG
implements RFC 6775 §4.2 (6CO ND option), the symmetric
compile-time requirement can be lifted.

## Usage

See `doc/getting-started/NAT64-for-Contiki-NG.md` for full build,
Cooja, hardware, DNS, and troubleshooting instructions.

For the native border router, enable NAT64 at run time with the
`--nat64` command-line flag:

```bash
sudo ./border-router.native --nat64 -s /dev/ttyUSB0
```

IoT nodes reach IPv4 servers by embedding the IPv4 address in the NAT64
prefix.  For example, `8.8.8.8` becomes `64:ff9b::808:808`.  The DNS64
translator handles this automatically for DNS lookups — nodes simply
resolve hostnames via a DNS server at a NAT64-mapped address.
