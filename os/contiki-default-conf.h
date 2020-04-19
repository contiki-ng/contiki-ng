/*
 * Copyright (c) 2012, Thingsquare, http://www.thingsquare.com/.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef CONTIKI_DEFAULT_CONF_H
#define CONTIKI_DEFAULT_CONF_H

/*---------------------------------------------------------------------------*/
/* Link-layer options
 */

/* IEEE802154_CONF_PANID defines the default PAN ID for IEEE 802.15.4 networks */
#ifndef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID 0xabcd
#endif /* IEEE802154_CONF_PANID */

/* IEEE802154_CONF_DEFAULT_CHANNEL defines the default channel for IEEE 802.15.4
 * networks, for MAC layers without a channel selection or channel hopping
 * mechanism. Current 802.15.4 MAC layers:
 * - CSMA: uses IEEE802154_CONF_DEFAULT_CHANNEL
 * - TSCH: uses its own TSCH_DEFAULT_HOPPING_SEQUENCE instead
 */
#ifndef IEEE802154_CONF_DEFAULT_CHANNEL
#define IEEE802154_CONF_DEFAULT_CHANNEL 26
#endif /* IEEE802154_CONF_DEF_CHANNEL */

/* QUEUEBUF_CONF_NUM specifies the number of queue buffers. Queue
   buffers are used throughout the Contiki netstack but the
   configuration option can be tweaked to save memory. Performance can
   suffer with a too low number of queue buffers though. */
#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 8
#endif /* QUEUEBUF_CONF_NUM */
/*---------------------------------------------------------------------------*/
/* uIPv6 configuration options.
 *
 * Many of the uIPv6 configuration options can be overriden by a
 * project-specific configuration to save memory.
 */

 /* NBR_TABLE_CONF_MAX_NEIGHBORS specifies the maximum number of neighbors
    that each node will be able to handle. */
#ifndef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 16
#endif /* NBR_TABLE_CONF_MAX_NEIGHBORS */

/* NETSTACK_MAX_ROUTE_ENTRIES specifies the maximum number of entries
   the routing module will handle. Applies to uIP routing tables if they are
   used, or to RPL non-storing mode links instead */
#ifndef NETSTACK_MAX_ROUTE_ENTRIES
#define NETSTACK_MAX_ROUTE_ENTRIES 16
#endif /* NETSTACK_MAX_ROUTE_ENTRIES */

/* UIP_CONF_BUFFER_SIZE specifies how much memory should be reserved
   for the uIP packet buffer. This sets an upper bound on the largest
   IP packet that can be received by the system. */
#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE 1280
#endif /* UIP_CONF_BUFFER_SIZE */

/* UIP_CONF_ROUTER specifies if the IPv6 node should be a router or
   not. By default, all Contiki nodes are routers. */
#ifndef UIP_CONF_ROUTER
#define UIP_CONF_ROUTER 1
#endif /* UIP_CONF_ROUTER */

/* UIP_CONF_IPV6_RPL tells whether the RPL routing protocol is running,
    whether implemented as RPL Lite or RPL Classic */
#define UIP_CONF_IPV6_RPL (ROUTING_CONF_RPL_LITE || ROUTING_CONF_RPL_CLASSIC)

/* If RPL is enabled also enable the RPL NBR Policy */
#if UIP_CONF_IPV6_RPL
/* Both Classic and Lite use rpl_nbr_gc_get_worst */
#ifndef NBR_TABLE_CONF_GC_GET_WORST
#define NBR_TABLE_CONF_GC_GET_WORST            rpl_nbr_gc_get_worst
#endif /* NBR_TABLE_CONF_GC_GET_WORST */
#if ROUTING_CONF_RPL_CLASSIC
/* Only Classic handles a max number of children */
#ifndef NBR_TABLE_CONF_CAN_ACCEPT_NEW
#define NBR_TABLE_CONF_CAN_ACCEPT_NEW          rpl_nbr_can_accept_new
#endif /* NBR_TABLE_CONF_CAN_ACCEPT_NEW */
/* Leave 3 spots for candidate parents incl. default route. */
#if RPL_NBR_POLICY_MAX_NEXTHOP_NEIGHBORS
#define RPL_NBR_POLICY_MAX_NEXTHOP_NEIGHBORS  MAX(NBR_TABLE_CONF_MAX_NEIGHBORS - 3, 0)
#endif /* RPL_NBR_POLICY_MAX_NEXTHOP_NEIGHBORS */
#endif /* ROUTING_CONF_RPL_CLASSIC */
#endif /* UIP_CONF_IPV6_RPL */

/* UIP_CONF_UDP specifies if UDP support should be included or
   not. Disabling UDP saves memory but breaks a lot of stuff. */
#ifndef UIP_CONF_UDP
#define UIP_CONF_UDP 1
#endif /* UIP_CONF_UDP */

/* UIP_CONF_UDP_CONNS specifies the maximum number of
   simultaneous UDP connections. */
#ifndef UIP_CONF_UDP_CONNS
#define UIP_CONF_UDP_CONNS 8
#endif /* UIP_CONF_UDP_CONNS */

/* UIP_CONF_TCP specifies if TCP support should be included or
   not. Disabling TCP saves memory. */
#ifndef UIP_CONF_TCP
#define UIP_CONF_TCP 0
#endif /* UIP_CONF_TCP */

/* UIP_CONF_TCP_CONNS specifies the maximum number of
   simultaneous TCP connections. */
#ifndef UIP_CONF_TCP_CONNS
#if UIP_CONF_TCP
#define UIP_CONF_TCP_CONNS 8
#else /* UIP_CONF_TCP */
#define UIP_CONF_TCP_CONNS 0
#endif /* UIP_CONF_TCP */
#endif /* UIP_CONF_TCP_CONNS */

/* UIP_CONF_ND6_SEND_RA enables standard IPv6 Router Advertisement.
 * We enable it by default when IPv6 is used without RPL. */
#ifndef UIP_CONF_ND6_SEND_RA
#if (NETSTACK_CONF_WITH_IPV6 && !UIP_CONF_IPV6_RPL)
#define UIP_CONF_ND6_SEND_RA 1
#else /* NETSTACK_CONF_WITH_IPV6 && !UIP_CONF_IPV6_RPL */
#define UIP_CONF_ND6_SEND_RA 0
#endif /* NETSTACK_CONF_WITH_IPV6 && !UIP_CONF_IPV6_RPL */
#endif /* UIP_CONF_ND6_SEND_RA */

/* UIP_CONF_ND6_SEND_NS enables standard IPv6 Neighbor Discovery Protocol
   (RFC 4861). We enable it by default when IPv6 is used without RPL.
   With RPL, the neighbor cache (link-local IPv6 <-> MAC address mapping)
   is fed whenever receiving DIO. This is often sufficient
   for RPL routing, i.e. to send to the preferred parent or any child.
   Link-local unicast to other neighbors may, however, not be possible if
   we never receive any DIO from them. This may happen if the link from the
   neighbor to us is weak, if DIO transmissions are suppressed (Trickle
   timer) or if the neighbor chooses not to transmit DIOs because it is
   a leaf node or for any reason. */
#ifndef UIP_CONF_ND6_SEND_NS
#if (NETSTACK_CONF_WITH_IPV6 && !UIP_CONF_IPV6_RPL)
#define UIP_CONF_ND6_SEND_NS 1
#else /* (NETSTACK_CONF_WITH_IPV6 && !UIP_CONF_IPV6_RPL) */
#define UIP_CONF_ND6_SEND_NS 0
#endif /* (NETSTACK_CONF_WITH_IPV6 && !UIP_CONF_IPV6_RPL) */
#endif /* UIP_CONF_ND6_SEND_NS */
/* To speed up the neighbor cache construction,
   enable UIP_CONF_ND6_AUTOFILL_NBR_CACHE. When a node does not the link-layer
   address of a neighbor, it will infer it from the link-local IPv6, assuming
   the node used autoconfiguration. Note that RPL uses its own freshness
   mechanism to select whether neighbors are still usable as a parent
   or not, regardless of the neighbor cache. Note that this is not
   standard-compliant (RFC 4861), as neighbors will be added regardless of
   their reachability and liveness. */
#ifndef UIP_CONF_ND6_AUTOFILL_NBR_CACHE
#if UIP_CONF_ND6_SEND_NS
#define UIP_CONF_ND6_AUTOFILL_NBR_CACHE 0
#else /* UIP_CONF_ND6_SEND_NS */
#define UIP_CONF_ND6_AUTOFILL_NBR_CACHE 1
#endif /* UIP_CONF_ND6_SEND_NS */
#endif /* UIP_CONF_ND6_AUTOFILL_NBR_CACHE */
/* UIP_CONF_ND6_SEND_NA allows to still comply with NDP even if the host does
   not perform NUD or DAD processes. By default it is activated so the host
   can still communicate with a full NDP peer. */
#ifndef UIP_CONF_ND6_SEND_NA
#if NETSTACK_CONF_WITH_IPV6
#define UIP_CONF_ND6_SEND_NA 1
#else /* NETSTACK_CONF_WITH_IPV6 */
#define UIP_CONF_ND6_SEND_NA 0
#endif /* NETSTACK_CONF_WITH_IPV6 */
#endif /* UIP_CONF_ND6_SEND_NS */

/*---------------------------------------------------------------------------*/
/* 6lowpan configuration options.
 *
 * These options change the behavior of the 6lowpan header compression
 * code (sicslowpan). They typically depend on the type of radio used
 * on the target platform, and are therefore platform-specific.
 */

/* SICSLOWPAN_CONF_FRAG specifies if 6lowpan fragmentation should be
   used or not. Fragmentation is on by default. */
#ifndef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG 1
#endif /* SICSLOWPAN_CONF_FRAG */

/* SICSLOWPAN_CONF_COMPRESSION specifies what 6lowpan compression
   mechanism to be used. 6lowpan hc06 is the default in Contiki. */
#ifndef SICSLOWPAN_CONF_COMPRESSION
#define SICSLOWPAN_CONF_COMPRESSION SICSLOWPAN_COMPRESSION_IPHC
#endif /* SICSLOWPAN_CONF_COMPRESSION */

#endif /* CONTIKI_DEFAULT_CONF_H */
