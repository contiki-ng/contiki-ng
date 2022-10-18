# IPv6 multicast

Contiki-NG's IPv6 stack supports multicast. We currently support the following modes:

* 'Multicast Protocol for Low-Power and Lossy Networks' (MPL)

  This is an implementation of the algorithm described in [MPL (RFC 7731)][MPL]. MPL is routing-independent and therefore works with both storing and non-storing modes, both RPL-classic and RPL-lite.

* 'Stateless Multicast RPL Forwarding' (SMRF)

  SMRF is a very simple, lightweight multicast forwarding engine for RPL networks. RPL in MOP 3 handles group management as per the RPL docs, whereas the SMRF engine handles datagram forwarding. SMRF can only forward multicast traffic downwards in a DODAG.

  SMRF requires RPL storing mode operation, which is only supported in RPL-classic.

  SMRF is presented and evaluated in:

    * G. Oikonomou, I. Phillips, T. Tryfonas, "[IPv6 Multicast Forwarding in RPL-Based Wireless Sensor Networks][smrf-wpc]", Wireless Personal Communications, Springer US, 73(3), pp. 1089-1116, 2013
    * G. Oikonomou, I. Phillips, "[Stateless Multicast Forwarding with RPL in 6LoWPAN Sensor Networks][smrf-percom]", in Proc. 2012 IEEE International Conference on Pervasive Computing and Communications Workshops (PERCOM Workshops), Lugano, Switzerland, pp. 272-277, 2012 

* 'Enhanced Stateless Multicast RPL Forwarding' (ESMRF)
    
  ESMRF is an enhanced version of the SMRF engine with the aim of resolving the sending limitation of SMRF to allow any node within the DODAG to send multicast traffic up, as well as down the RPL DODAG. ESMRF requires RPL storing mode operation, which is only supported in RPL-classic.

  ESMRF is presented and evaluated in:  K. Qorany Abdel Fadeel and K. El Sayed, "[ESMRF: Enhanced Stateless Multicast RPL Forwarding For IPv6-based Low-Power and Lossy Networks][esmrf]", in Proc. 2015 Workshop on IoT challenges in Mobile and Industrial Systems (IoT-Sys '15), ACM, New York, NY, USA, pp. 19-24, 2015.
    
* 'Multicast Forwarding with Trickle'

  This is an implementation of the algorithm described in the [Trickle Multicast][trickle-multicast] internet draft. The version of this draft that's currently implementated is documented in `roll-tm.h`
   
  Trickle multicast is routing-independent and therefore works with both storing and non-storing modes, both RPL-classic and RPL-lite.

  This is basically an implementation of a very early version of the Internet Draft that eventually became [MPL (RFC 7731)][MPL]. It is retained since it has been tested much more extensively, but it will eventually be phased out in favour of the MPL engine.

More engines can (and hopefully will) be added in the future.

## The Big Gotcha

Currently we only support traffic originating and destined inside a single 6LoWPAN.
To be able to send multicast traffic from the internet to 6LoWPAN nodes or the other
way round, we need border routers or other gateway devices to be able to achieve
the following:

* Add/Remove Trickle Multicast, RPL or other HBHO headers as necessary for datagrams
  entering / exiting the 6LoWPAN
* Advertise multicast group membership to the internet (e.g. with MLD)

These are currently not implemented and are in the ToDo list. Contributions welcome.

## Where to Start

The best place is `examples/multicast`.

There is a cooja example demonstrating basic functionality.

## How to Use

Look in `os/net/ipv6/multicast/uip-mcast6-engines.h` for a list of supported
multicast engines.

To turn on multicast support, add this line in your `project-` or `contiki-conf.h`
```c
#define UIP_MCAST6_CONF_ENGINE xyz
```
  where xyz is a value from `uip-mcast6-engines.h`.

To disable:
```c
#define UIP_MCAST6_CONF_ENGINE 0
```
You also need to make sure the multicast code gets built. Your example's
(or platform's) Makefile should include this:
```
MODULES += os/net/ipv6/multicast
```

## How to extend

Let's assume you want to write an engine called foo.
The multicast API defines a multicast engine driver in a fashion similar to
the various NETSTACK layer drivers. This API defines functions for basic
multicast operations (init, in, out).
In order to extend multicast with a new engine, perform the following steps:

- Open `uip-mcast6-engines.h` and assign a unique integer code to your engine
```c
#define UIP_MCAST6_ENGINE_FOO        xyz
```
  - Include your engine's `foo.h`

- In `foo.c`, implement:
  * `init()`
  * `in()`
  * `out()`
  * Define your driver like so:
```c
const struct uip_mcast6_driver foo_driver = { ... }
```
- If you want to maintain stats:
  * Standard multicast stats are maintained in `uip_mcast6_stats`. Don't access
    this struct directly, use the macros provided in `uip-mcast6-stats.h` instead
  * You can add your own stats extensions. To do so, declare your own stats
    struct in your engine's module, e.g `struct foo_stats`
  * When you initialise the stats module with `UIP_MCAST6_STATS_INIT`, pass
    a pointer to your stats variable as the macro's argument.
    An example of how to extend multicast stats, look at the ROLL TM engine

- Open `uip-mcast6.h` and add a section in the `#if` spree. This aims to
  configure the uIPv6 core. More specifically, you need to:
  * Specify if you want to put RPL in MOP3 by defining
      `RPL_WITH_MULTICAST`: 1: MOP 3, 0: non-multicast MOP
  * Define your engine details
```c
#define UIP_MCAST6             foo_driver
#define UIP_MCAST6_STATS       foo_stats
typedef struct foo_stats uip_mcast6_stats_t;
```
  * Optionally, add a configuration check block to stop builds when the
    configuration is not sane.

If you need your engine to perform operations not supported by the generic
UIP_MCAST6 API, you will have to hook those in the uip core manually. As an
example, see how the core is modified so that it can deliver ICMPv6 datagrams
to the ROLL TM engine.

[smrf-percom]: http://dx.doi.org/10.1109/PerComW.2012.6197494
[smrf-wpc]: http://dx.doi.org/10.1007/s11277-013-1250-5
[esmrf]: http://doi.acm.org/10.1145/2753476.2753479 
[trickle-multicast]: http://tools.ietf.org/html/draft-ietf-roll-trickle-mcast
[MPL]: https://tools.ietf.org/html/rfc7731
