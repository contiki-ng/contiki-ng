# Packet buffers

This page, intended for protocol developers, describes the different types of buffers used in Contiki-NG.
The focus is on the 6LoWPAN stack, but all information about Packetbuf and Queuebuf also applies to NullNet.

## uIP buffer

At the network layer and above, packet payloads are stored in `uip_buf`.

To send data, upper-layer protocols or applications are expected to write to this buffer and then trigger a transmission.
The uIP stack first adds the required IPv6 header and possible extension headers.
6LoWPAN will then compress the headers and fragment the datagram if needed.
Each fragment will be, in turn, passed to the MAC layer for transmission.

At reception time, the reverse procedure takes place.
The MAC layer will call 6LoWPAN, which will decompress headers and reassemble the datagram into `uip_buf`, before calling the uIP stack.
The datagram is then accessible for processing by uIP and upper layers.

Access rules for `uip_buf`:
* only from 6LoWPAN, uIP, or above, but not from any layer below
* only outside of interrupt context

## Packetbuf

6LoWPAN will build the link-layer packets directly into the global `packetbuf`.
In addition to the payload, `packetbuf` carries a number of attributes / meta-data (see `packetbuf.h`).
Once the packet is ready, 6LoWPAN passes it to the MAC layer for transmission.
Likewise, when receiving packets, the MAC layer passes packets to 6LoWPAN via `packetbuf`.
The `packetbuf` API is detailed at [doxygen:packetbuf].

Access rules for `packetbuf`:
* only from 6LoWPAN or below, but not from any layer above
* only outside of interrupt context

## Queuebuf

The `queuebuf` module provides a way to manage multiple packets at a time.
The content of every `queuebuf` instance is basically the same as in the global `packetbuf`.
Modules that need `queuebuf` are responsible for maintaining pointers to them -- there is no global pointer to `queuebuf` like there is for `packetbuf`.

For instance, 6LoWPAN uses `queuebuf` when fragmenting IPv6 datagrams into multiple packets.
The MAC layers CSMA and TSCH also use `queuebuf` for their transmit queues.

Access rules for `queuebuf`:
* only from 6LoWPAN or below, but not from any layer above
* outside of interrupt context, or from interrupt context if the `queuebuf` instance is protected with a lock (like in TSCH)

When enabled, the `queuebuf` module will be initialised automatically by Contiki-NG. Depending on your platform's configuration, it may be possible to disable the `queuebuf` module by setting:

```c
#define QUEUEBUF_CONF_ENABLED 0
```

Keep in mind that some Contiki-NG modules require the `queuebuf` module (e.g., CSMA, TSCH, and 6LoWPAN fragmentation support), so you should disable it only if you do not need any of this functionality.

[doxygen:packetbuf]: https://contiki-ng.readthedocs.io/en/develop/_api/group__packetbuf.html
