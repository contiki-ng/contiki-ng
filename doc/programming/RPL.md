# RPL

## About RPL

RPL is an IPv6 routing protocol for lossy and low-power networks specified in [RFC 6550](https://tools.ietf.org/html/rfc6550), which is implemented in Contiki-NG's IPv6 network stack. RPL builds and maintains a Destination-Oriented Directed Acyclic Graph (DODAG) topology that originates from a designated RPL root node, which typically also serves as a border router to the Internet. Routing information is disseminated through broadcast beacons aperiodically using [Trickle timers](https://tools.ietf.org/html/rfc6206). The topology is built according to a certain goal by an objective function. Such a goal can be to minimize the Estimated Transmission Count (ETX) on the path from the node to the RPL root, as is in our implementations of the two Objective Functions we support, [MRHOF](https://tools.ietf.org/html/rfc6719) and [0F0](https://tools.ietf.org/html/rfc6552). 

RPL supports different directions of traffic:
* Upward routing: from any node to a root.
* Downward routing: from the root to any node.
* Any-to-any routing: where traffic flows between arbitrary pairs of nodes in the DODAG by routing upwards to their closest common ancestor (or the root in non-storing mode) in the DODAG, and then downward to the destination node.

All upward routing is handled by having each node on the path toward the root forwarding traffic through a preferred parent. Downward routing can be handled with two different types of modes: non-storing and storing mode (see below).

## Implementations

Contiki-NG provides two implementations of RPL with different attributes: _RPL Classic_ and _RPL Lite_.

### RPL Classic

RPL Classic is the continuation of the original Contiki's RPL implementation: [ContikiRPL](http://www.diva-portal.org/smash/get/diva2:1042739/FULLTEXT01.pdf). This implementation was created as early as 2009, while the RPL standard was still under development. Over the years, it has gotten a lot of functionality that has been added to the RPL RFC and related RFCs, such as support for multiple instances and DODAGs, storing and non-storing mode, multicasting, and more. Hence, at the price of supporting a lot of functionality from standards and Internet drafts, the implementation has become complex and thus has gotten a large ROM footprint.

### RPL Lite

RPL Lite is Contiki-NG's default RPL implementation. It started as a major rewrite of the 2017 version of ContikiRPL, with a focus on the most important and stable functionality, as the community has experienced in many deployments and research experiments. RPL Lite removes support for storing mode in favor of non-storing mode, and removes the complexity of handling multiple instances and DODAGs. Through these changes, RPL Lite typically exhibits better performance and has a considerably smaller ROM footprint. On the flip side of these optimizations, it has a lower interoperability level with other implementations, which may use storing mode for instance.

## Modes of operation (MOP)

The different modes of operation refer to the different ways downward routing is done. However, they also have impact on TSCH Orchestra scheduling, on RAM usage on the nodes, and on packet sizes.

### Storing mode

For storing mode, routing tables are stored on each node, which can impose a significant memory footprint in large networks and be hard to maintain consistently, but there is on the other hand no need for potentially large source routing headers in the IPv6 packets.

RPL-Lite does not support the storing mode. To enable the storing mode, select RPL-Classic in the Makefile:

    MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC

The storing mode is the default for RPL-Classic. Alternatively, it can also be explicitly enabled with:

    #define RPL_CONF_MOP RPL_MOP_STORING_NO_MULTICAST

or, if multicast routing is desired, with:

    #define RPL_CONF_MOP RPL_MOP_STORING_MULTICAST


### Non-storing mode

For non-storing mode, IPv6 source routing is employed, which means that nodes do not have to store routing tables for nodes below them in the DODAG. On the other hand, packets can get large if many hops are along the path, whose addresses would then need to be embedded in the source routing header. Note that packets traveling upwards are not affected - only downward packets get the extra headers!

The DODAG root node still has to store the routing table of the whole network, so its memory usage will still be high in large networks.

The Orchestra scheduler is also affected by the non-storing mode. The Orchestra rule `unicast_per_neighbor_rpl_storing` cannot be used in this mode, since it relies on routing information on each node.

To enable the non-storing mode, either select RPL-Lite (where it is the default), or use this configuration:

    #define RPL_CONF_MOP RPL_MOP_NON_STORING

### Mode of operation 0 (MOP0)

This mode disables all downward routing. To enable MOP0, explicitly turn off `RPL_CONF_WITH_STORING` and `RPL_CONF_WITH_NON_STORING`:

    #define RPL_CONF_WITH_STORING           0
    #define RPL_CONF_WITH_NON_STORING       0
    #define RPL_CONF_MOP RPL_MOP_NO_DOWNWARD_ROUTES

## RPL Lite: topology formation and configuration

We review here how RPL Lite builds a topology and the configuration parameters that matter most in this process.
For an extensive list of configuration parameters, see https://github.com/contiki-ng/contiki-ng/blob/master/os/net/routing/rpl-lite/rpl-conf.h

### DAG Advertisement

When a node starts running as DAG root (whether it is border router or not), it will advertise the DAG with DIOs (DODAG Information Object).
DIO transmissions follow a Trickle timer.
Some important configuration parameters are:
* `NETSTACK_MAX_ROUTE_ENTRIES`: the number of routing entries at the root, which must be set to at least the network size
* `RPL_CONF_SUPPORTED_OFS`: a list of Objective Functions embedded in the node, any of which can be selected at run-time
* `RPL_CONF_OF_OCP`: the Objective Function advertised by the root.
* `RPL_CONF_DIO_INTERVAL_MIN`: the minimum Trickle interval is `2^RPL_CONF_DIO_INTERVAL_MIN` milliseconds. A value of 12 for instance results in 4.096 seconds.
* `RPL_CONF_DIO_INTERVAL_DOUBLINGS`: the maximum Trickle interval is `2^(RPL_CONF_DIO_INTERVAL_MIN+RPL_CONF_DIO_INTERVAL_DOUBLINGS)` milliseconds. A value of 8 (with a min doubling interval of 12) results in a maximum period of 1048.576 seconds (about 17 minutes).
* `RPL_CONF_DIO_REDUNDANCY`: the Trickle redundancy constant, used to suppress DIO transmissions in dense networks. Disabled by default in RPL Lite (value of 0).

### Joining

Nodes willing to join a network will transmit periodic DIS (DODAG Information Solicitation) to trigger Trickle reset at neighboring nodes, and increase their chances to hear a DIO:
* `RPL_CONF_DIS_INTERVAL`: the interval at which nodes looking for a DAG or a parent will send DIS messages

Whenever hearing a DIO, the node might choose to join the DAG. The first thing needed then is to select a RPL preferred parent. Because RPL Lite focuses on reliability, nodes do not select a parent until they have a precise estimate of their link quality to the neighbor (the `link-stats` module provides a freshness indicator for that purpose). Nodes will then perform link-probing to assess their neighbors' link:
* `RPL_CONF_PROBING_SEND_FUNC`: the probing function. By default, probing is simply a unicast DIO to the target neighbor
* `RPL_CONF_PROBING_INTERVAL`: the interval at which background probing is done, in clock ticks
* `RPL_CONF_PROBING_DELAY_FUNC`: the function that calculates the next delay. By default, the delay is dynamic: if there is urgent need for probing (when the node has no usable parent), probing will happen within seconds, else, some random interval based on `RPL_CONF_PROBING_INTERVAL`
* `RPL_CONF_PROBING_SELECT_FUNC`: the function that selects the next probing target. The default function probes the urgent probing target if any, or the preferred parent if its link statistics need refresh. Otherwise, it picks at random between (1) selecting the best neighbor with non-fresh link statistics, or (2) selecting the least recently updated neighbor

### Preferred parent selection

After enough probing, the node will select a neighbor as preferred parent. This is according to the selected Objective Function and metric. By default, MRHOF and ETX are used. There are a number of important configuration parameters for link estimation and Objective Function:
* `LINK_STATS_CONF_INIT_ETX_FROM_RSSI`: this is part of the `link-stats` module. When set (default), nodes estimate their neighbors' link quality when first hearing from them, based on the RSSI of, e.g, an incoming DIO. For a deeper understanding of how this is calculated, as well as how link quality is later maintained, take a look at `link-stats.c`
* `RPL_MRHOF_CONF_SQUARED_ETX`: when set, MRHOF will square the link ETX before adding it to the parent rank for path cost calculation. This results in more reliable paths, as it penalizes higher link ETX. Stronger links are typically selected, at the expense of longer paths and higher churn. The feature is disabled by default, as the higher churn can result in unstable operation in networks with poor links. Check out `rpl-mrhof.c` or `rpl-of0.c` for more configuration options.

### Route registration

Once a preferred parent is chosen, a node will then register itself through a DAO (Destination Advertisement Object). In non-storing mode (only mode in RPL lite), the DAO is sent directly to the root, using global IPv6 addresses. Upon receiving the DAO, the root will add the node to its routing state: it will store the child-parent relationship, used later for source routing. In RPL Lite, by default, DAO messages have the 'K' bit set, which means they must be acknowledged by the root:
* `RPL_CONF_DAO_RETRANSMISSION_TIMEOUT`: the delay after which nodes resend their DAO in case no DAO-ACK was received
* `RPL_CONF_DAO_MAX_RETRANSMISSIONS`: the maximum number of DAO retransmissions
* `RPL_CONF_DEFAULT_LIFETIME_UNIT`: the unit, in seconds, used for lifetime in DAO messages
* `RPL_CONF_DEFAULT_LIFETIME`: the lifetime, in lifetime units, advertised in DAO messages. The DAG root will delete the route after this lifetime expires. Nodes will resend a DAO automatically within 1-2 minutes before expiry.

After receiving a DAO-ACK, nodes know they are fully part of the network and reachable.
Only then will they start advertising DIOs in turn, and let more nodes join, forming a multi-hop mesh network.
* `RPL_CONF_DEFAULT_LEAF_ONLY`: if this is set, the node will join but only as a leaf, i.e., it will not send any DIO and will never be selected as parent

### DAG maintenance

When a node is part of a DAG, it will constantly maintain link estimates via probing, keep its preferred parent up to date, and advertise the DAG root accordingly. RPL local repairs (reset Trickle timer, reset link statistics) are performed when needed as required in the standard. When a node undergoes a significant rank change, it will also reset its Trickle timer for quicker topology update:
* `RPL_CONF_SIGNIFICANT_CHANGE_THRESHOLD`: the rank change threshold for Trickle reset. Whenever the current rank differs form the last advertised rank by at least this threshold, the node resets its Trickle.

When a node finds no more suitable preferred parent, it will start poisoning, i.e., advertise an infinite rank to let its sub-DAG know it no longer is a valid parent. It will then leave the network after a delay:
* `RPL_CONF_DELAY_BEFORE_LEAVING`: the delay after which a node actually leaves a network, by default, 5 minutes.

During this delay, the node performs poisoning. Meanwhile, it also starts sending periodic DIS again, in hope to discover a new usable parent. If this happens, the node will directly stop poisoning and consider itself part of the DAG again. If not, it will eventually leave the DAG after the delay and send DIS until it joins a new DAG.

