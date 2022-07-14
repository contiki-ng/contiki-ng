# RPL

We continue here from the IPv6 ping example ([tutorial:ipv6-ping]). We will now look at the basics of the RPL (pronounced 'ripple') routing protocol ([doc:rpl]).

To start with, we will build a stand-alone network with no connectivity to outside networks. The next tutorial ([tutorial:rpl-border-router]) will show you how add Internet connectivity with a border router.

RPL fundamentally perceives our mesh network as a tree topology, called a DAG (Directed Acyclic Graph) or DODAG (Destination-Oriented DAG). The network is instantiated (constructed) by the tree's root (DAG or DODAG root).

In Contiki-NG, RPL is enabled by default, with the RPL-lite implementation. Compile the example, and flash two or more nodes. Select one node that will act as the DAG root for our network, i.e., initiate the construction of a RPL network. Log in to this node.

Using the shell, set the node as DAG root with `rpl-set-root`:
```
> rpl-set-root 1
Setting as DAG root with prefix fd00::/64
```

This starts a root with the default prefix (`fd00::/64`). A different prefix can be provided as parameter. The root will start advertising the network via RPL DIO (DODAG Information Object) messages. Devices that receive a DIO message will join the RPL network.

Nodes can also search for available networks by sending periodic DIS (DODAG Information Solicitation) messages. Reception of a DIS message by a nearby device that has already joined a network will result in that device sending a DIO to advertise the presence of this network.

Try `rpl-status` to inspect a node's internal RPL state. At the root:
```
> rpl-status
RPL status:
-- Instance: 0
-- DAG root
-- DAG: fd00::212:4b00:616:fcc, version 240
-- Prefix: fd00::/64
-- MOP: Non-storing
-- OF: MRHOF
-- Hop rank increment: 128
-- Default lifetime: 1800 seconds
-- State: Reachable
-- Preferred parent: (NULL IP addr)
-- Rank: 128
-- Lowest rank: 65535 (1024)
-- DTSN out: 240
-- DAO sequence: last sent 240, last acked 240
-- Trickle timer: current 13, min 12, max 20, redundancy 0
```

Or at another node:
```
> rpl-status
RPL status:
-- Instance: 0
-- DAG node
-- DAG: fd00::212:4b00:616:fcc, version 240
-- Prefix: fd00::/64
-- MOP: Non-storing
-- OF: MRHOF
-- Hop rank increment: 128
-- Default lifetime: 1800 seconds
-- State: Reachable
-- Preferred parent: fe80::212:4b00:616:fcc
-- Rank: 489
-- Lowest rank: 489 (1024)
-- DTSN out: 240
-- DAO sequence: last sent 241, last acked 241
-- Trickle timer: current 12, min 12, max 20, redundancy 0
```

Back to the root node, inspect the installed routing entries with `routes`:
```
> routes
Default route:
-- None
Routing links (2 in total):
-- fd00::212:4b00:616:fcc (DODAG root) (lifetime: infinite)
-- fd00::212:4b00:616:fc4 to fd00::212:4b00:616:fcc (lifetime: 1620 seconds)
```

This shows all the routing links (RPL non-storing mode) known at the root, enabling downward routing. You should now be able to ping any node in the network (even if over multiple hops), using its global IPv6 address:
```
> ping fd00::212:4b00:616:fcc
Pinging fd00::212:4b00:616:fcc
Received ping reply from fd00::212:4b00:616:fcc, len 4, ttl 64, delay 7 ms
```

You can manually reset neighbor entries with a RPL local repair, using the shell command `rpl-local-repair`.
Or you can even re-initiate the DAG construction from scratch with a RPL global repair, using `rpl-global-repair`.

## Deep dive

How RPL works is documented at [doc:rpl]. We look here at what happens as the nodes build the topology. For the results below, simply enable log level 3 (Info) for the RPL module.

First, the node sends a DIS, looking for a DAG:
```
Node > [INFO: RPL       ] sending a DIS to ff02::1a
```

The root received the DIS, resets its Trickle timer and sends a new DIO:
```
Root > [INFO: RPL       ] received a DIS from fe80::212:4b00:616:fc4
Root > [INFO: RPL       ] reset DIO timer (Multicast DIS)
Root > [INFO: RPL       ] sending a multicast-DIO with rank 128 to ff02::1a
```

The node receives the DIO and joins the DAG:
```
Node > [INFO: RPL       ] received a multicast-DIO from fe80::212:4b00:616:fcc, instance_id 0, DAG ID fd00::212:4b00:616:fcc, version 240, dtsn 240, rank 128
Node > [INFO: RPL       ] adding global IP address fd00::212:4b00:616:fc4
Node > [INFO: RPL       ] reset MRHOF
Node > [INFO: RPL       ] initialized DAG with instance ID 0, DAG ID fd00::212:4b00:616:fcc, prexix fd00::/64, rank 65535
```

The node cannot estimate the quality of its link to the root yet, so it won't select any preferred parent. It will start "urgent probing" the candidate parent until it has calculated a fresh estimate of the link quality.
```
Node > [WARN: RPL       ] just joined, no parent yet, setting timer for leaving
Node > [INFO: RPL       ] refreshing lifetime
Node > [WARN: RPL       ] best parent is not fresh, schedule urgent probing to fe80::212:4b00:616:fcc
Node > [INFO: RPL       ] probing fe80::212:4b00:616:fcc (urgent) last tx 0 min ago
Node > [INFO: RPL       ] sending a unicast-DIO with rank 65535 to fe80::212:4b00:616:fcc
Node > [INFO: RPL       ] packet sent to 0012.4b00.0616.0fcc, status 0, tx 1, new link metric 162
...
```

Finally, after a few probes, the node decides to select the root as its parent:
```
Node > [INFO: RPL       ] parent switch: (NULL IP addr) -> fe80::212:4b00:616:fcc
Node > [WARN: RPL       ] found parent: fe80::212:4b00:616:fcc, staying in DAG
```

It sends a DAO to register itself with the root:
```
Node > [INFO: RPL       ] sending a DAO seqno 241, tx count 1, lifetime 30, prefix fd00::212:4b00:616:fc4 to fd00::212:4b00:616:fcc, parent fe80::212:4b00:616:fcc
Node > [INFO: RPL       ] creating hop-by-hop option
Node > [INFO: RPL       ] packet sent to 0012.4b00.0616.0fcc, status 0, tx 1, new link metric 142
```

The root receives the DAO:
```
Root > [INFO: RPL       ] received a DAO from fd00::212:4b00:616:fc4, seqno 241, lifetime 30, prefix fd00::212:4b00:616:fc4, prefix length 128, parent fd00::212:4b00:616:fcc 
Root > [INFO: RPL       ] NS: updating link, child fd00::212:4b00:616:fcc, parent (NULL IP addr), lifetime 4294967295, num_nodes 1
Root > [INFO: RPL       ] NS: updating link, child fd00::212:4b00:616:fc4, parent fd00::212:4b00:616:fcc, lifetime 1800, num_nodes 2
```

And sends a DAO-ACK. The DAO-ACK will be source-routed to the originator:
```
Root > [INFO: RPL       ] SRH creating source routing header with destination fd00::212:4b00:616:fc4 
Root > [INFO: RPL       ] SRH path len: 0, ComprI 15, ComprE 15, ext len 8 (padding 0)
Root > [INFO: RPL       ] packet sent to 0012.4b00.0616.0fc4, status 0, tx 1, new link metric 126
```

When the node receives the DAO-ACK, it finally considers itself a part of the DAG. It resets its Trickle timer, and starts sending DIOs:
```
Node > [INFO: RPL       ] received a DAO-ACK with seqno 241 (241 241) and status 0 from fd00::212:4b00:616:fcc
Node > [INFO: RPL       ] reset DIO timer (Reachable)
Node > [INFO: RPL       ] sending a multicast-DIO with rank 285 to ff02::1a
```

From this point onwards, other nodes may join through the newly joined node, forming a multi-hop mesh network.

[doc:rpl]: /doc/programming/RPL
[tutorial:rpl-border-router]: /doc/tutorials/RPL-border-router
[tutorial:ipv6-ping]: /doc/tutorials/IPv6-ping