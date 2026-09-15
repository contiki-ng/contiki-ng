# ip64 end-to-end test

Exercises the `os/services/ip64` NAT64 service on the native target, with the
ENC28J60 driver replaced by a capture driver (`ip64-test-driver.c`). No
hardware, tun device, Cooja instance, or superuser privileges are needed.

The build mirrors the module's only supported hardware deployment,
`arch/platform/zoul/orion`: the same `ip64-conf.h` shape and the same
`-DUIP_FALLBACK_INTERFACE=ip64_uip_fallback_interface` that
`Makefile.orion` sets.

For a test that puts ip64 on a real IPv4 network instead, and drives it from
software on the host that knows nothing about NAT64, see
`tests/08-native-runs/27-ip64-tap`.

## What the test covers

| Test | Path exercised |
|------|----------------|
| `arp_resolution` | uIP fallback dispatch, `ip64-arp` request generation |
| `udp_round_trip` | `ip64_6to4`, address-mapping allocation and reverse lookup, `ip64_4to6`, delivery to a UDP socket |
| `forwarded_flow` | Forwarding from the IPv6 network: a packet injected as a mote's is translated out under a mapping of its own, distinct from the router's |
| `dns64_rewrite` | `ip64_dns64_6to4` (AAAA to A) and `ip64_dns64_4to6` (A to AAAA) |
| `icmp_echo` | ICMP translation in both directions: an IPv4 ping reaches the local IPv6 host and its reply is translated back |
| `inbound_ports` | Inbound port handling: delivery to the local host below `EPHEMERAL_PORTRANGE`, and the drop of ephemeral-port traffic that matches no mapping |
| `dns64_copies_later_records` | `ip64_dns64_4to6` on a reply with two answers, where the first grows and moves the second along with it |
| `dns64_moves_the_additional_section` | `ip64_dns64_4to6` on a reply with an EDNS OPT record, which has to move when the answer before it grows |
| `dns64_rejects_oversized_record` | `ip64_dns64_4to6` against an A record claiming more data than the packet holds |
| `dns64_rejects_a_late_oversized_record` | The same, where the bad record is the second one, so a reply that has already been rewritten in part has to be dropped rather than forwarded |
| `dns64_rejects_unterminated_name` | `ip64_dns64_4to6` against a question name that runs past the end of the packet |
| `dns64_stops_when_the_output_is_full` | `ip64_dns64_4to6` given room for one record to grow, where two of them do |
| `dns64_6to4_stops_at_the_end` | `ip64_dns64_6to4` against a query truncated in the middle of its question |
| `sixto4_rejects_bad_length` | `ip64_6to4` against an IPv6 payload length field larger than the payload received, and against a packet shorter than a header |

The tests below `inbound_ports` call a translation or the DNS64 parser
directly. Each sets its output buffer up the way the caller does, and checks
both the return value and a canary past the room it was given, so an
out-of-bounds write shows up without a sanitiser. All of them fail against
the code as it stood before the fixes in this pull request.

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
