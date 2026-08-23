# ip64 TAP test

Runs the `os/services/ip64` NAT64 service on the native target with its IPv4
side on a Linux TAP device, so that ordinary host software exchanges IPv4
traffic with the node. Where `../26-ip64` asserts on the frames ip64 produces,
this test asserts that a program that knows nothing about NAT64 can talk to
the node.

## What the test checks

| Check | Path exercised |
|-------|----------------|
| The echo server receives IPv4 from `192.0.2.50` | `ip64_6to4`, ARP resolution of the host, and a real IPv4 receiver |
| The node logs `IP64_TAP_ECHO_OK` | `ip64_4to6` and the reverse mapping lookup, on a reply the test did not build |
| `ping 192.0.2.50` from the host succeeds | inbound ICMP echo translation, and ip64 answering the host's ARP request |
| The DNS server receives an **A** query | `ip64_dns64_6to4`: the node is IPv6-only and asks for AAAA, which an IPv4-only server could never answer |
| The node resolves the name to `64:ff9b::192.0.2.99` | `ip64_dns64_4to6`: the A record in the answer is turned into an AAAA record the node can use |

The suite builds the node a second time with `IP64_CONF_DHCP=1` and runs the
same script over it, so that the address comes from a DHCP server on the host
as it does on the Orion border router. The node reports which build it is in
its `IP64_TAP_MODE` line, and the script then asserts on the lease as well.

| Check | Path exercised |
|-------|----------------|
| The DHCP server leases `192.0.2.50` | `ip64-dhcpc`, and `ip64_6to4` on a broadcast destination: the client sends to `64:ff9b::255.255.255.255` |
| The node reports the leased address and router | `ip64_ipv4_dhcp`, and the IPv4-broadcast-to-multicast path in `ip64_4to6` that carries the reply back |

The checks in the first table then run again over the leased address, which
shows that the leased configuration is usable and not merely parsed.

The DNS answer, `192.0.2.99`, is deliberately not the DNS server's own
address, so the address the node reports can only have come from the record.

The node creates the TAP device itself, gives the host end `192.0.2.1`, and
answers to `192.0.2.50`. Both addresses are in TEST-NET-1 (RFC 5737), which
keeps the test off any address range a real network is likely to use. They
are compiled in, and the script is written around them; `IP64_TAP_DEV`
selects a device other than `tap0`.

The echo server and the DNS server both bind `192.0.2.1` rather than every
address, so that they cannot collide with a resolver already running on the
machine. That address exists only after the node has created the device,
which is why the script starts the node first.

## Privileges and host state

Creating the TAP device and configuring the interface need `CAP_NET_ADMIN`,
so the test has to run as root. It re-executes itself under `sudo` where that
needs no password, which is how it runs in CI: the container the Linux jobs
use is privileged, but its user is not root. Failing that, it stops with an
explanation, or skips if `IP64_TAP_ALLOW_SKIP=1` is set.

Running a native node as root has a side effect that predates this test: the
native platform opens its own tun device for IPv6, configures it, and sets
`net.ipv6.conf.all.forwarding=1`. Every native IPv6 node run as root does
this, `tests/17-tun-rpl-br` included. The TAP device itself is not
persistent and disappears when the node exits.

## Running it

The test is part of the `tests/08-native-runs` suite, which builds the node
for both configurations and runs the script over each:

```
make -C tests/08-native-runs
```

The two runs on their own, from the suite directory, once the node is built
for the configuration in question:

```
make -C 27-ip64-tap TARGET=native
./27-ip64-tap.sh
make -C 27-ip64-tap TARGET=native DEFINES=IP64_CONF_DHCP=1
./27-ip64-tap.sh
```

## What it does not cover

- TCP, and the ip64 special-ports translation of inbound traffic.
- Lease renewal and expiry: the test takes a lease and uses it, but does not
  run long enough to renew one.
- The ENC28J60 driver and its SPI glue, which have no native equivalent.
