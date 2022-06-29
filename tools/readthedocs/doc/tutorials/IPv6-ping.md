# IPv6 ping

We will continue here from our `hello-world` example (see [tutorial:hello-world]) with shell enabled (see [tutorial:shell]). We will look at IPv6 communication (see [doc:ipv6]). Note that it is possible to ping even a native node: when running it with sufficient permissions (e.g. `sudo`), native nodes start a `tun` interface and can be pinged. Here we look at pinging hardware nodes. Compile the project and upload it to both nodes (either by not specifying `MOTES` at all, which will upload to all, or by listing both tty devices e.g. with `MOTES=/dev/ttyUSB0 /dev/ttyUSB1`).

This tutorial uses the default CSMA MAC protocol, for introduction using TSCH see a tutorial on how to [switch the hello-world example to TSCH](/doc/tutorials/Switching-to-TSCH).

Log in to one node (e.g. `make TARGET=zoul MOTES=/dev/ttyUSB0 login`) and get its link-local IPv6 address via the shell:
```
> ip-addr
Node IPv6 addresses:
-- fe80::212:4b00:616:fc4
```

The, log in to the other node, and ping the first node's IPv6 address, e.g.:
```
> ping fe80::212:4b00:616:fc4
Pinging fe80::212:4b00:616:fc4
Received ping reply from fe80::212:4b00:616:fc4, len 4, ttl 64, delay 31 ms
```

If you enable IPv6 INFO logs (set `#define LOG_CONF_LEVEL_IPV6 LOG_LEVEL_INFO` in your `project-conf.h`), you will see more information. On the source host side:
```
> ping fe80::212:4b00:616:fc4
Pinging fe80::212:4b00:616:fc4
[INFO: ICMPv6    ] Sending ICMPv6 packet to ff02::1a, type 128, code 0, len 4
[INFO: IPv6      ] packet received from fe80::212:4b00:616:fc4 to fe80::212:4b00:616:fcc
[INFO: IPv6      ] icmp6: input length 48 type: 129 
[INFO: ICMPv6    ] Received Echo Reply from fe80::212:4b00:616:fc4 to fe80::212:4b00:616:fcc
Received ping reply from fe80::212:4b00:616:fc4, len 4, ttl 64, delay 70 ms
```

And on the destination host side:
```
> ping fe80::212:4b00:616:fc4
Pinging fe80::212:4b00:616:fc4
[INFO: IPv6      ] icmp6: input length 48 type: 128 
[INFO: ICMPv6    ] Received Echo Request from fe80::212:4b00:616:fcc to fe80::212:4b00:616:fc4
[INFO: ICMPv6    ] Sending Echo Reply to fe80::212:4b00:616:fcc from fe80::212:4b00:616:fc4
[INFO: IPv6      ] Sending packet with length 48 (8)
[INFO: ICMPv6    ] Sending ICMPv6 packet to fe80::212:4b00:616:fcc, type 155, code 0, len 2
```

You may wonder how the node found out one another's MAC address.
By default `UIP_ND6_AUTOFILL_NBR_CACHE` is enabled (try the make `viewconf` command, [doc:configuration]), which lets the node derive the MAC address from the EUI-64 contained in the IPv6 address.
Contiki-NG enables this feature by default, but one can also run IPv6 Neighbor Discovery instead by enabling `UIP_ND6_SEND_NS`.

You can inspect a node's neighbor table at any time with the shell command `ip-nbr`:
```
> ip-nbr
Node IPv6 neighbors:
-- fe80::212:4b00:616:fc4 <-> 0012.4b00.0616.0fc4, router 0, state Reachable 
```

[doc:ipv6]: /doc/programming/IPv6
[doc:configuration]: /doc/getting-started/The-Contiki-NG-configuration-system
[tutorial:hello-world]: /doc/tutorials/Hello,-World!
[tutorial:shell]: /doc/tutorials/Shell
[tutorial:logging]: /doc/tutorials/Logging
