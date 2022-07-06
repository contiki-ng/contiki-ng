# NullNet

Contiki-NG provides NullNet, a network layer that does nothing. NullNet is most useful for testing of lower layer protocols, or for IPv6-less networking in general. NullNet is enabled via the `MAKE_NET` variable, see [doc:configuration].

With NullNet, network-layer packets are created directly by the application, and sent as-is to the MAC layer. On the receiver side, the MAC layer will pass the packet up to NullNet, which will in turn call the application with a callback.

## Sending with NullNet

To send a packet with NullNet, first point `nullnet_buf` (declared in nullnet.h) to your buffer, and set `nullnet_len` to your payload length. Then, call `NETSTACK_NETWORK.output`, with the destination link-layer address as parameter. Use `NULL` or `linkaddr_null` to send a broadcast. For instance:
```c
#include "net/nullnet/nullnet.h"
...
uint8_t payload[64] = { 0 };
nullnet_buf = payload; /* Point NullNet buffer to 'payload' */
nullnet_len = 2; /* Tell NullNet that the payload length is two bytes */
NETSTACK_NETWORK.output(NULL); /* Send as broadcast */
```

NullNet will send your payload as-is. No header added. There isn't even a notion of port, which means that NullNet does not segregate multiple connections for you.

## Receiving with NullNet

To receive, set the NullNet application callback to a function of your own. This is done with `nullnet_set_input_callback`. The function must be of type `nullnet_input_callback`. For instance:
```c
#include "net/nullnet/nullnet.h"
...
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
...
}
...
/* At process initialization */
nullnet_set_input_callback(input_callback);
```

The function `input_callback` will be called by NullNet upon every packet reception, both unicast or broadcast. The function arguments provide a pointer to the payload, the payload length, and the link-layer addresses of the source and destination. For broadcast, `dest` is set to `linkaddr_null`.

[doc:configuration]: /doc/getting-started/The-Contiki-NG-configuration-system

[doc:nullnet-programming]: /doc/programming/NullNet-programming
