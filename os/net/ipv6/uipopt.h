/*
 * Copyright (c) 2001-2003, Adam Dunkels.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 *
 */

/**
 * \file
 * Configuration options for uIP.
 * \author Adam Dunkels <adam@dunkels.com>
 *
 * This file contains various compile-time configuration options for
 * uIP along with definitions of their default values.
 *
 * When an option starts with "UIP_CONF", it can be changed to fit the
 * requirements of a specific system configuration, typically through
 * a "project-conf.h" file.
 */

/**
 * \addtogroup uip
 * @{
 */

/**
 * \defgroup uipopt Configuration options for uIP
 * @{
 */

#ifndef UIPOPT_H_
#define UIPOPT_H_

#ifndef UIP_LITTLE_ENDIAN
#define UIP_LITTLE_ENDIAN  3412
#endif /* UIP_LITTLE_ENDIAN */
#ifndef UIP_BIG_ENDIAN
#define UIP_BIG_ENDIAN     1234
#endif /* UIP_BIG_ENDIAN */

#include "contiki.h"

/*------------------------------------------------------------------------------*/

/**
 * \defgroup uipoptgeneral General configuration options
 * @{
 */

/**
 * The size of the uIP packet buffer.
 *
 * The uIP packet buffer should not be smaller than 60 bytes, and does
 * not need to be larger than 1514 bytes, which is the Ethernet frame
 * size limit. A lower size typically results in lower throughput for
 * various protocols, whereas a larger size can result in higher
 * throughput.

 * \note 6LoWPAN fragmentation may be used if the link layer has a
 * smaller maximum transmission unit (MTU) that cannot fit the uIP
 * packet buffer contents. See the SICSLOWPAN_CONF_FRAG option.
 *
 * \hideinitializer
 */
#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_BUFSIZE (UIP_LINK_MTU)
#else /* UIP_CONF_BUFFER_SIZE */
#define UIP_BUFSIZE (UIP_CONF_BUFFER_SIZE)
#endif /* UIP_CONF_BUFFER_SIZE */

/**
 * Determines if statistics support should be compiled in.
 *
 * The statistics is useful for debugging and to show the user.
 *
 * \hideinitializer
 */
#ifndef UIP_CONF_STATISTICS
#define UIP_STATISTICS  0
#else /* UIP_CONF_STATISTICS */
#define UIP_STATISTICS (UIP_CONF_STATISTICS)
#endif /* UIP_CONF_STATISTICS */

/** @} */
/*------------------------------------------------------------------------------*/
/**
 * \defgroup uipoptip IP configuration options
 * @{
 *
 */
/**
 * The IP TTL (time to live) of IP packets sent by uIP.
 *
 * This should normally not be changed.
 */
#ifdef UIP_CONF_TTL
#define UIP_TTL         UIP_CONF_TTL
#else /* UIP_CONF_TTL */
#define UIP_TTL         64
#endif /* UIP_CONF_TTL */

/**
 * The maximum time in seconds that an IP fragment should wait in the
 * reassembly buffer before it is dropped.
 *
 */
#define UIP_REASS_MAXAGE 60

/** @} */

/*------------------------------------------------------------------------------*/
/**
 * \defgroup uipoptipv6 IPv6 configuration options
 * @{
 *
 */

/** The maximum transmission unit at the IP layer. */
#define UIP_LINK_MTU 1280

#ifndef UIP_CONF_IPV6_QUEUE_PKT
/** Do we do per-neighbor queuing during address resolution (default: no) */
#define UIP_CONF_IPV6_QUEUE_PKT       0
#endif

#ifndef UIP_CONF_IPV6_CHECKS
/** Do we do IPv6 consistency checks (highly recommended, default: yes) */
#define UIP_CONF_IPV6_CHECKS          1
#endif

#ifndef UIP_CONF_IPV6_REASSEMBLY
/** Do we do IPv6 fragmentation (default: no) */
#define UIP_CONF_IPV6_REASSEMBLY      0
#endif

#ifndef UIP_CONF_NETIF_MAX_ADDRESSES
/** Default number of IPv6 addresses associated to the node's interface */
#define UIP_CONF_NETIF_MAX_ADDRESSES  3
#endif

#ifndef UIP_CONF_DS6_PREFIX_NBU
/** Default number of IPv6 prefixes associated to the node's interface */
#define UIP_CONF_DS6_PREFIX_NBU     2
#endif

#ifndef UIP_CONF_DS6_DEFRT_NBU
/** Minimum number of default routers */
#define UIP_CONF_DS6_DEFRT_NBU       2
#endif
/** @} */

/*------------------------------------------------------------------------------*/
/**
 * \defgroup uipoptudp UDP configuration options
 * @{
 *
 */

/**
 * Toggles whether UDP support should be compiled in or not.
 *
 * \hideinitializer
 */
#ifdef UIP_CONF_UDP
#define UIP_UDP UIP_CONF_UDP
#else /* UIP_CONF_UDP */
#define UIP_UDP           1
#endif /* UIP_CONF_UDP */

/**
 * Toggles if UDP checksums should be used or not.
 *
 * \hideinitializer
 */
#ifdef UIP_CONF_UDP_CHECKSUMS
#define UIP_UDP_CHECKSUMS (UIP_CONF_UDP_CHECKSUMS)
#else
#define UIP_UDP_CHECKSUMS 1
#endif

/**
 * The maximum amount of concurrent UDP connections.
 *
 * \hideinitializer
 */
#ifdef UIP_CONF_UDP_CONNS
#define UIP_UDP_CONNS (UIP_CONF_UDP_CONNS)
#else /* UIP_CONF_UDP_CONNS */
#define UIP_UDP_CONNS    10
#endif /* UIP_CONF_UDP_CONNS */

/** @} */
/*------------------------------------------------------------------------------*/
/**
 * \defgroup uipopttcp TCP configuration options
 * @{
 */

/**
 * Toggles whether TCP support should be compiled in or not.
 *
 * \hideinitializer
 */
#ifdef UIP_CONF_TCP
#define UIP_TCP (UIP_CONF_TCP)
#else /* UIP_CONF_TCP */
#define UIP_TCP           1
#endif /* UIP_CONF_TCP */

/**
 * Determines if support for opening connections from uIP should be
 * compiled in.
 *
 * If the applications that are running on top of uIP do not need to
 * open outgoing TCP connections, this configuration option can be
 * turned off to reduce the code size of uIP.
 *
 * \hideinitializer
 */
#ifndef UIP_CONF_ACTIVE_OPEN
#define UIP_ACTIVE_OPEN 1
#else /* UIP_CONF_ACTIVE_OPEN */
#define UIP_ACTIVE_OPEN (UIP_CONF_ACTIVE_OPEN)
#endif /* UIP_CONF_ACTIVE_OPEN */

/**
 * The maximum number of simultaneously open TCP connections.
 *
 * Since the TCP connections are statically allocated, turning this
 * configuration knob down results in less RAM used. Each TCP
 * connection requires approximately 30 bytes of memory.
 *
 * \hideinitializer
 */
#ifndef UIP_CONF_TCP_CONNS
#define UIP_TCP_CONNS       10
#else /* UIP_CONF_TCP_CONNS */
#define UIP_TCP_CONNS (UIP_CONF_TCP_CONNS)
#endif /* UIP_CONF_TCP_CONNS */


/**
 * The maximum number of simultaneously listening TCP ports.
 *
 * Each listening TCP port requires 2 bytes of memory.
 *
 * \hideinitializer
 */
#ifndef UIP_CONF_MAX_LISTENPORTS
#define UIP_LISTENPORTS 20
#else /* UIP_CONF_MAX_LISTENPORTS */
#define UIP_LISTENPORTS (UIP_CONF_MAX_LISTENPORTS)
#endif /* UIP_CONF_MAX_LISTENPORTS */

/**
 * Determines if support for TCP urgent data notification should be
 * compiled in.
 *
 * Urgent data (out-of-band data) is a rarely used TCP feature that
 * seldomly would be required.
 *
 * \hideinitializer
 */
#define UIP_URGDATA      0

/**
 * The initial retransmission timeout counted in timer pulses.
 *
 * This should not be changed.
 */
#define UIP_RTO         3

/**
 * The maximum number of times a segment should be retransmitted
 * before the connection should be aborted.
 *
 * This should not be changed.
 */
#define UIP_MAXRTX      8

/**
 * The maximum number of times a SYN segment should be retransmitted
 * before a connection request should be deemed to have been
 * unsuccessful.
 *
 * This should not need to be changed.
 */
#define UIP_MAXSYNRTX      5

/**
 * The TCP maximum segment size.
 *
 * This is should not be to set to more than
 * UIP_BUFSIZE - UIP_IPTCPH_LEN.
 */
#ifdef UIP_CONF_TCP_MSS
#if UIP_CONF_TCP_MSS > (UIP_BUFSIZE - UIP_IPTCPH_LEN)
#error UIP_CONF_TCP_MSS is too large for the current UIP_BUFSIZE
#endif /* UIP_CONF_TCP_MSS > (UIP_BUFSIZE - UIP_IPTCPH_LEN) */
#define UIP_TCP_MSS     (UIP_CONF_TCP_MSS)
#else /* UIP_CONF_TCP_MSS */
#define UIP_TCP_MSS     (UIP_BUFSIZE - UIP_IPTCPH_LEN)
#endif /* UIP_CONF_TCP_MSS */

/**
 * The size of the advertised receiver's window.
 *
 * Should be set low (i.e., to the size of the uip_buf buffer) if the
 * application is slow to process incoming data, or high (32768 bytes)
 * if the application processes data quickly.
 *
 * \hideinitializer
 */
#ifndef UIP_CONF_RECEIVE_WINDOW
#define UIP_RECEIVE_WINDOW (UIP_TCP_MSS)
#else
#define UIP_RECEIVE_WINDOW (UIP_CONF_RECEIVE_WINDOW)
#endif

/**
 * How long a connection should stay in the TIME_WAIT state.
 *
 * This can be reduced for faster entry into power saving modes.
 */
#ifndef UIP_CONF_WAIT_TIMEOUT
#define UIP_TIME_WAIT_TIMEOUT 120
#else
#define UIP_TIME_WAIT_TIMEOUT UIP_CONF_WAIT_TIMEOUT
#endif

/** @} */
/*------------------------------------------------------------------------------*/
/**
 * \defgroup uipoptarp ARP configuration options
 * @{
 */

/**
 * The size of the ARP table.
 *
 * This option should be set to a larger value if this uIP node will
 * have many connections from the local network.
 *
 * \hideinitializer
 */
#ifdef UIP_CONF_ARPTAB_SIZE
#define UIP_ARPTAB_SIZE (UIP_CONF_ARPTAB_SIZE)
#else
#define UIP_ARPTAB_SIZE 8
#endif

/**
 * The maximum age of ARP table entries measured in 10ths of seconds.
 *
 * An UIP_ARP_MAXAGE of 120 corresponds to 20 minutes (BSD
 * default).
 */
#define UIP_ARP_MAXAGE 120

/** @} */
/*------------------------------------------------------------------------------*/

/**
 * \defgroup uipoptmac layer 2 options (for ipv6)
 * @{
 */

#define UIP_DEFAULT_PREFIX_LEN 64

/**
 * The MAC-layer transmissons limit is encapslated in "Traffic Class" field.
 *
 * In Contiki-NG, if the Traffic Class field in the IPv6 header has
 * this bit set, the low-order bits are used as the MAC-layer
 * transmissons limit.
 */
#define UIP_TC_MAC_TRANSMISSION_COUNTER_BIT  0x40

/**
 * The bits in the "Traffic Class" field that describe the MAC
 * transmission limit.
 */
#define UIP_TC_MAC_TRANSMISSION_COUNTER_MASK 0x3F

#ifdef UIP_CONF_TAG_TC_WITH_VARIABLE_RETRANSMISSIONS
#define UIP_TAG_TC_WITH_VARIABLE_RETRANSMISSIONS UIP_CONF_TAG_TC_WITH_VARIABLE_RETRANSMISSIONS
#else
#define UIP_TAG_TC_WITH_VARIABLE_RETRANSMISSIONS 0
#endif

/**
 * This is the default value of MAC-layer transmissons for uIPv6
 *
 * It means that the limit is selected by the MAC protocol instead of uIPv6.
 */
#define UIP_MAX_MAC_TRANSMISSIONS_UNDEFINED 0

/** @} */

/*------------------------------------------------------------------------------*/

/**
 * \defgroup uipoptsics 6lowpan options (for ipv6)
 * @{
 */
/**
 * Timeout for packet reassembly at the 6LoWPAN layer
 * (should be < 60s)
 */
#ifdef SICSLOWPAN_CONF_MAXAGE
#define SICSLOWPAN_REASS_MAXAGE (SICSLOWPAN_CONF_MAXAGE)
#else
#define SICSLOWPAN_REASS_MAXAGE 8
#endif

/**
 * Determines whether the 6LoWPAN layer uses IP header compression.
 */
#ifndef SICSLOWPAN_CONF_COMPRESSION
#define SICSLOWPAN_COMPRESSION SICSLOWPAN_COMPRESSION_IPHC
#else
#define SICSLOWPAN_COMPRESSION SICSLOWPAN_CONF_COMPRESSION
#endif /* SICSLOWPAN_CONF_COMPRESSION */

/**
 * If we use IPHC compression, how many address contexts do we support.
 */
#ifndef SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS 1
#endif

/**
 * Determines whether 6LoWPAN fragmentation is enabled.
 */
#ifndef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG  1
#endif

/** @} */

/*------------------------------------------------------------------------------*/
/**
 * \defgroup uipoptcpu CPU architecture configuration
 * @{
 *
 * The CPU architecture configuration is where the endianness of the
 * CPU on which uIP is to be run is specified. Most CPUs today are
 * little endian, and the most notable exception are the Motorolas
 * which are big endian. The BYTE_ORDER macro should be changed to
 * reflect the CPU architecture on which uIP is to be run.
 */

/**
 * The byte order of the CPU architecture on which uIP is to be run.
 *
 * This option can be either UIP_BIG_ENDIAN (Motorola byte order) or
 * UIP_LITTLE_ENDIAN (Intel byte order).
 *
 * \hideinitializer
 */
#ifdef UIP_CONF_BYTE_ORDER
#define UIP_BYTE_ORDER     (UIP_CONF_BYTE_ORDER)
#else /* UIP_CONF_BYTE_ORDER */
#define UIP_BYTE_ORDER     (UIP_LITTLE_ENDIAN)
#endif /* UIP_CONF_BYTE_ORDER */

/** @} */

#endif /* UIPOPT_H_ */
/** @} */
/** @} */
