# UDP communication

This page documents how to send and receive raw UDP datagrams with Contiki-NG.
We focus on the module `simple-udp`.
The API of this module is detailed at [doxygen:simple-udp].

We present here a simple example of how to use it.
First, register a socket, both at the sender and receiver:
```c
#include "simple-udp.h"
...
static struct simple_udp_connection udp_conn;
simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);
```

This registers `udp_conn`, with host and remote port `UDP_PORT`. No fixed remote IPv6 address is provided (third argument is `NULL`). The last argument is the input callback function. Provide an implementation for it as follows:
```c
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
...
}
```

For each incoming datagram, the function above will be called with parameters that provide source/destination addresses and ports, and the payload.

Lastly, this is how to send a UDP datagram:
```c
uint8_t payload[64] = { 0 };
simple_udp_sendto(&udp_conn, payload, 2, &destination_ipaddr);
```

You're all set! The last parameter is the IPv6 address (type `uip_ipaddr_t`) of the destination. If the connection was registered initially with a given remote, you do not need to specify the destination every time you send, simply use `simple_udp_send` instead. Finally, you can also send to a particular UDP port with `simple_udp_sendto_port`.

[doxygen:simple-udp]: https://contiki-ng.readthedocs.io/en/develop/_api/group__simple-udp.html
