# NAT64 DNS Lookup Example

This example demonstrates how a Contiki-NG IoT node can resolve public
domain names and communicate with IPv4 servers through a NAT64-enabled
border router.

## Overview

The IoT node configures Google Public DNS (8.8.8.8) as its nameserver,
encoded as a NAT64 address (`64:ff9b::808:808`). It then uses the
standard Contiki-NG DNS resolver to look up hostnames such as
`www.contiki-ng.org` and `www.example.com`.

The border router's NAT64 module intercepts the DNS queries, translates
them from IPv6 to IPv4, and forwards them to 8.8.8.8 via a host UDP
socket. Responses are translated back with DNS64 synthesis: A records
(IPv4 addresses) become AAAA records with the `64:ff9b::/96` prefix.

After successful lookups, the node exercises both transport paths:
it sends UDP probes for the non-HTTP hosts and performs one HTTP GET
to `www.contiki-ng.org` to test the TCP splice proxy end to end.

## Testing with Cooja (no hardware needed)

### 1. Start the Cooja simulation

Open `nat64-dns-cooja.csc` in Cooja. The simulation contains:
- **Mote 1** — SLIP radio bridge (connects Cooja to the native border router)
- **Mote 2** — NAT64 DNS client (the example application)

Start the simulation. Cooja will listen on TCP port 60001 for the
border router to connect.

### 2. Start the native border router with NAT64

In a separate terminal:

```sh
cd examples/nat64
sudo ./run-nat64-br.sh
```

This builds and starts the native border router, which connects to
Cooja on `localhost:60001` and enables the NAT64 gateway.

### 3. Observe the output

In Cooja's log listener, mote 2 will show:

```
NAT64 DNS client starting, waiting for network...
DNS server set to 64:ff9b::808:808 (8.8.8.8 via NAT64)
Querying DNS for "www.contiki-ng.org"...
Resolved "www.contiki-ng.org" -> 64:ff9b::b63c:d8a7
Starting HTTP GET http://www.contiki-ng.org/
HTTP complete: received <n> body bytes
Resolved "www.example.com" -> 64:ff9b::<ipv4>
Sent UDP probe to 64:ff9b::<ipv4> (www.example.com)
```

## Running on real hardware

### 1. Build and flash the DNS client on the sensor node

```sh
cd examples/nat64
make TARGET=<your-target>
```

Replace `<your-target>` with the platform of your sensor node (e.g.,
`nrf`, `cc2538dk`, `zoul`).

Flash the resulting binary onto the node. Then flash the border router
firmware from `examples/rpl-border-router` onto a second node that
will serve as the 802.15.4 radio interface for the native border
router.

### 2. Start the native border router with NAT64

```sh
cd examples/nat64
sudo ./run-nat64-br.sh /dev/ttyUSB0
```

Replace `/dev/ttyUSB0` with the serial port of the radio interface
node. Without an argument, the script falls back to connecting to
Cooja on `localhost:60001`.

## How It Works

```
IoT node                    Border router (native)          Internet
   |                              |                            |
   |-- DNS query (IPv6/UDP) ----->|                            |
   |   to 64:ff9b::808:808       |-- DNS query (IPv4/UDP) --->|
   |                              |   to 8.8.8.8              |
   |                              |                            |
   |                              |<-- DNS response (A rec) ---|
   |<-- DNS response (AAAA) -----|   (DNS64 synthesis)        |
   |   64:ff9b::<ipv4>           |                            |
   |                              |                            |
   |-- UDP probe or HTTP GET ---->|                            |
   |   to 64:ff9b::<ipv4>        |-- UDP probe (IPv4) ------->|
   |                              |   or TCP connect+HTTP ---->|
```

## Files

- `nat64-dns-client.c` — The example application.
- `project-conf.h` — Configuration: DNS cache size, MDNS disabled.
- `Makefile` — Includes the `resolv` module.
- `nat64-dns-cooja.csc` — Cooja simulation file.
- `run-nat64-br.sh` — Script to start the native border router with NAT64.
