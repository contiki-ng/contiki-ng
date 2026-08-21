# ip64 end-to-end test

Exercises the `os/services/ip64` NAT64 service on the native target, with the
ENC28J60 driver replaced by a capture driver (`ip64-test-driver.c`). No
hardware, tun device, Cooja instance, or superuser privileges are needed.

The build mirrors the module's only supported hardware deployment,
`arch/platform/zoul/orion`: the same `ip64-conf.h` shape and the same
`-DUIP_FALLBACK_INTERFACE=ip64_uip_fallback_interface` that
`Makefile.orion` sets.

## What the test covers

| Test | Path exercised |
|------|----------------|
| `arp_resolution` | uIP fallback dispatch, `ip64-arp` request generation |
| `udp_round_trip` | `ip64_6to4`, address-mapping allocation and reverse lookup, `ip64_4to6`, delivery to a UDP socket |
| `forwarded_flow` | Forwarding from the IPv6 network: a packet injected as a mote's is translated out under a mapping of its own, distinct from the router's |
| `dns64_rewrite` | `ip64_dns64_6to4` (AAAA to A) and `ip64_dns64_4to6` (A to AAAA) |
| `icmp_echo` | ICMP translation in both directions: an IPv4 ping reaches the local IPv6 host and its reply is translated back |
| `inbound_ports` | Inbound port handling: delivery to the local host below `EPHEMERAL_PORTRANGE`, and the drop of ephemeral-port traffic that matches no mapping |

Outbound packets come from ordinary `simple_udp` sockets and are collected
through the fallback interface. Inbound packets are handed to
`ip64_eth_interface_input()` as Ethernet frames, exactly as a driver would
deliver them.

## What it does not cover

- The ENC28J60 driver and its SPI glue, which have no native equivalent.
- The DHCPv4 client: the test configures the IPv4 address statically
  (`IP64_CONF_DHCP 0`).
- TCP, and the ip64 special-ports translation of inbound traffic.
- The return leg to a mote: a translated reply leaves over the radio, where
  the test cannot observe it, so `forwarded_flow` checks the mapping entry
  that `ip64_4to6()` resolves a reply against instead of the packet itself.
- The CC2538-based Orion target. Cooja cannot emulate CC2538, so no
  simulator-based test can cover the deployed binary either.
