# Communication Security

Contiki-NG comes with standard communication protocols, that provide both per-hop and end-to-end confidentiality and integrity.

## Application-layer security
Application-layer security is end-to-end, over IPv6 in the Contiki-NG case.
We currently support DTLS, thanks to a slightly modified version of TinyDTLS which we host at https://github.com/contiki-ng/tinydtls. On top of TinyDTLS, we support CoAPs [doc:coaps], which is the secure version of CoAP. With CoAPs, the CoAP header and payload is encrypted and authenticated end-to-end, that is, from IP host to IP host. This offers some level of protection against malicious routers (can not read nor tamper the data). Note that the only mode included so far is pre-shared keys.

## Link-layer security

Link-layer security is per-hop, i.e., secures packets on air between two neighbors.
Even with an application-layer security protocol such as DTLS is in place, this offers a number of benefits:
* Secures all communication, not only UDP. Means ICMPv6 and TCP but also non-IPv6 traffic such as MAC-layer beacons and ACKs.
* As a consequence of the above, protects against attacks on the routing protocol (e.g. sinkhole) or against the communication stack itself (e.g. DoS, malformed packet injection).

We currently support link-layer security for IEEE 802.15.4 TSCH [doc:tsch-security], with a set of two keys, one for beacons and one for data traffic. This was in conformance with early 6TiSCH minimal architecture drafts, and we do plan to upgrade for full support of the 6TiSCH architecture once finalized.

For CSMA, we should have link-layer security support fairly soon, see https://github.com/contiki-ng/contiki-ng/pull/558

[doc:coaps]: CoAP.md#coaps---secure-coap
[doc:tsch-security]: TSCH-and-6TiSCH.md#using-tsch-with-security
