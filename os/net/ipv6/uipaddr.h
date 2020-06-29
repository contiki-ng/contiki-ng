/*
 * Copyright (c) 2001-2003, Adam Dunkels.
 * Copyright (c) 2020 alexrayne <alexraynepe196@gmail.com>.
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

#ifndef OS_NET_IP_UIPADDR_H_
#define OS_NET_IP_UIPADDR_H_

/**
 * \addtogroup uip
 * @{
 */

/**
 * \file
 * Header file for the uIP TCP/IP stack.
 * \author  Adam Dunkels <adam@dunkels.com>
 * \author  Julien Abeille <jabeille@cisco.com> (IPv6 related code)
 * \author  Mathilde Durvy <mdurvy@cisco.com> (IPv6 related code)
 * \author  alexrayne <alexraynepe196@gmail.com> (deploy from uip.h)
 *
 * Representation of an IP address and related routinues/macro.
 *
 */

#include <stdint.h>
/* For memcmp */
#include <string.h>
#include "net/ip/uipopt.h"


/**
 * Representation of an IP address.
 *
 */
typedef union uip_ip4addr_t {
  uint8_t  u8[4];                       /* Initializer, must come first. */
  uint16_t u16[2];
} uip_ip4addr_t;

typedef union uip_ip6addr_t {
  uint8_t  u8[16];                      /* Initializer, must come first. */
  uint16_t u16[8];
} uip_ip6addr_t;

#if NETSTACK_CONF_WITH_IPV6
typedef uip_ip6addr_t uip_ipaddr_t;
#else /* NETSTACK_CONF_WITH_IPV6 */
typedef uip_ip4addr_t uip_ipaddr_t;
#endif /* NETSTACK_CONF_WITH_IPV6 */


/*---------------------------------------------------------------------------*/
#define UIP_802154_SHORTADDR_LEN 2
#define UIP_802154_LONGADDR_LEN  8

/** \brief 16 bit 802.15.4 address */
typedef struct uip_802154_shortaddr {
  uint8_t addr[UIP_802154_SHORTADDR_LEN];
} uip_802154_shortaddr;
/** \brief 64 bit 802.15.4 address */
typedef struct uip_802154_longaddr {
  uint8_t addr[UIP_802154_LONGADDR_LEN];
} uip_802154_longaddr;

/** \brief 802.11 address */
typedef struct uip_80211_addr {
  uint8_t addr[6];
} uip_80211_addr;

/** \brief 802.3 address */
typedef struct uip_eth_addr {
  uint8_t addr[6];
} uip_eth_addr;


#ifndef UIP_CONF_LL_802154
#define UIP_CONF_LL_802154 1
#endif /* UIP_CONF_LL_802154 */

#if UIP_CONF_LL_802154
/** \brief 802.15.4 address */
#if LINKADDR_SIZE == UIP_802154_LONGADDR_LEN
typedef uip_802154_longaddr uip_lladdr_t;
#elif LINKADDR_SIZE == UIP_802154_SHORTADDR_LEN
typedef uip_802154_shortaddr uip_lladdr_t;
#else /* LINKADDR_SIZE == 8 */
#error unsupported configuration of LINKADDR_SIZE
#endif /* LINKADDR_SIZE == 8 */
/** \brief Link layer address length */
#define UIP_LLADDR_LEN LINKADDR_SIZE
#else /*UIP_CONF_LL_802154*/
#if UIP_CONF_LL_80211
/** \brief 802.11 address */
typedef uip_80211_addr uip_lladdr_t;
/** \brief Link layer address length */
#define UIP_LLADDR_LEN 6
#else /*UIP_CONF_LL_80211*/
/** \brief Ethernet address */
typedef uip_eth_addr uip_lladdr_t;
/** \brief Link layer address length */
#define UIP_LLADDR_LEN 6
#endif /*UIP_CONF_LL_80211*/
#endif /*UIP_CONF_LL_802154*/

/* uIP convenience and converting functions. */

/**
 * \defgroup uipconvfunc uIP conversion functions
 * @{
 *
 * These functions can be used for converting between different data
 * formats used by uIP.
 */

/**
 * Convert an IP address to four bytes separated by commas.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 printf("ipaddr=%d.%d.%d.%d\n", uip_ipaddr_to_quad(&ipaddr));
 \endcode
 *
 * \param a A pointer to a uip_ipaddr_t.
 * \hideinitializer
 */
#define uip_ipaddr_to_quad(a) (a)->u8[0],(a)->u8[1],(a)->u8[2],(a)->u8[3]

/**
 * Construct an IP address from four bytes.
 *
 * This function constructs an IP address of the type that uIP handles
 * internally from four bytes. The function is handy for specifying IP
 * addresses to use with e.g. the uip_connect() function.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 struct uip_conn *c;

 uip_ipaddr(&ipaddr, 192,168,1,2);
 c = uip_connect(&ipaddr, UIP_HTONS(80));
 \endcode
 *
 * \param addr A pointer to a uip_ipaddr_t variable that will be
 * filled in with the IP address.
 *
 * \param addr0 The first octet of the IP address.
 * \param addr1 The second octet of the IP address.
 * \param addr2 The third octet of the IP address.
 * \param addr3 The forth octet of the IP address.
 *
 * \hideinitializer
 */
#define uip_ipaddr(addr, addr0,addr1,addr2,addr3) do {  \
    (addr)->u8[0] = addr0;                              \
    (addr)->u8[1] = addr1;                              \
    (addr)->u8[2] = addr2;                              \
    (addr)->u8[3] = addr3;                              \
  } while(0)

/**
 * Construct an IPv6 address from eight 16-bit words.
 *
 * This function constructs an IPv6 address.
 *
 * \hideinitializer
 */
#define uip_ip6addr(addr, addr0,addr1,addr2,addr3,addr4,addr5,addr6,addr7) do { \
    (addr)->u16[0] = UIP_HTONS(addr0);                                      \
    (addr)->u16[1] = UIP_HTONS(addr1);                                      \
    (addr)->u16[2] = UIP_HTONS(addr2);                                      \
    (addr)->u16[3] = UIP_HTONS(addr3);                                      \
    (addr)->u16[4] = UIP_HTONS(addr4);                                      \
    (addr)->u16[5] = UIP_HTONS(addr5);                                      \
    (addr)->u16[6] = UIP_HTONS(addr6);                                      \
    (addr)->u16[7] = UIP_HTONS(addr7);                                      \
  } while(0)

/**
 * Construct an IPv6 address from sixteen 8-bit words.
 *
 * This function constructs an IPv6 address.
 *
 * \hideinitializer
 */
#define uip_ip6addr_u8(addr, addr0,addr1,addr2,addr3,addr4,addr5,addr6,addr7,addr8,addr9,addr10,addr11,addr12,addr13,addr14,addr15) do { \
    (addr)->u8[0] = addr0;                                       \
    (addr)->u8[1] = addr1;                                       \
    (addr)->u8[2] = addr2;                                       \
    (addr)->u8[3] = addr3;                                       \
    (addr)->u8[4] = addr4;                                       \
    (addr)->u8[5] = addr5;                                       \
    (addr)->u8[6] = addr6;                                       \
    (addr)->u8[7] = addr7;                                       \
    (addr)->u8[8] = addr8;                                       \
    (addr)->u8[9] = addr9;                                       \
    (addr)->u8[10] = addr10;                                     \
    (addr)->u8[11] = addr11;                                     \
    (addr)->u8[12] = addr12;                                     \
    (addr)->u8[13] = addr13;                                     \
    (addr)->u8[14] = addr14;                                     \
    (addr)->u8[15] = addr15;                                     \
  } while(0)


/**
 * Copy an IP address from one place to another.
 *
 * Copies an IP address from one place to another.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr1, ipaddr2;

 uip_ipaddr(&ipaddr1, 192,16,1,2);
 uip_ipaddr_copy(&ipaddr2, &ipaddr1);
 \endcode
 *
 * \param dest The destination for the copy.
 * \param src The source from where to copy.
 *
 * \hideinitializer
 */
#ifndef uip_ipaddr_copy
#define uip_ipaddr_copy(dest, src) (*(dest) = *(src))
#endif
#ifndef uip_ip4addr_copy
#define uip_ip4addr_copy(dest, src) (*((uip_ip4addr_t *)dest) = *((uip_ip4addr_t *)src))
#endif
#ifndef uip_ip6addr_copy
#define uip_ip6addr_copy(dest, src) (*((uip_ip6addr_t *)dest) = *((uip_ip6addr_t *)src))
#endif

/**
 * Compare two IP addresses
 *
 * Compares two IP addresses.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr1, ipaddr2;

 uip_ipaddr(&ipaddr1, 192,16,1,2);
 if(uip_ipaddr_cmp(&ipaddr2, &ipaddr1)) {
  printf("They are the same");
 }
 \endcode
 *
 * \param addr1 The first IP address.
 * \param addr2 The second IP address.
 *
 * \hideinitializer
 */
#define uip_ip4addr_cmp(addr1, addr2) ((addr1)->u16[0] == (addr2)->u16[0] && \
                                       (addr1)->u16[1] == (addr2)->u16[1])
#define uip_ip6addr_cmp(addr1, addr2) (memcmp(addr1, addr2, sizeof(uip_ip6addr_t)) == 0)

#if NETSTACK_CONF_WITH_IPV6
#define uip_ipaddr_cmp(addr1, addr2) uip_ip6addr_cmp(addr1, addr2)
#else /* NETSTACK_CONF_WITH_IPV6 */
#define uip_ipaddr_cmp(addr1, addr2) uip_ip4addr_cmp(addr1, addr2)
#endif /* NETSTACK_CONF_WITH_IPV6 */

/**
 * Compare two IP addresses with netmasks
 *
 * Compares two IP addresses with netmasks. The masks are used to mask
 * out the bits that are to be compared.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr1, ipaddr2, mask;

 uip_ipaddr(&mask, 255,255,255,0);
 uip_ipaddr(&ipaddr1, 192,16,1,2);
 uip_ipaddr(&ipaddr2, 192,16,1,3);
 if(uip_ipaddr_maskcmp(&ipaddr1, &ipaddr2, &mask)) {
 printf("They are the same");
 }
 \endcode
 *
 * \param addr1 The first IP address.
 * \param addr2 The second IP address.
 * \param mask The netmask.
 *
 * \hideinitializer
 */

#define uip_ipaddr_maskcmp(addr1, addr2, mask)          \
  (((((uint16_t *)addr1)[0] & ((uint16_t *)mask)[0]) ==       \
    (((uint16_t *)addr2)[0] & ((uint16_t *)mask)[0])) &&      \
   ((((uint16_t *)addr1)[1] & ((uint16_t *)mask)[1]) ==       \
    (((uint16_t *)addr2)[1] & ((uint16_t *)mask)[1])))

#define uip_ipaddr_prefixcmp(addr1, addr2, length) (memcmp(addr1, addr2, length>>3) == 0)



/*
 * Check if an address is a broadcast address for a network.
 *
 * Checks if an address is the broadcast address for a network. The
 * network is defined by an IP address that is on the network and the
 * network's netmask.
 *
 * \param addr The IP address.
 * \param netaddr The network's IP address.
 * \param netmask The network's netmask.
 *
 * \hideinitializer
 */
/*#define uip_ipaddr_isbroadcast(addr, netaddr, netmask)
  ((uip_ipaddr_t *)(addr)).u16 & ((uip_ipaddr_t *)(addr)).u16*/



/**
 * Mask out the network part of an IP address.
 *
 * Masks out the network part of an IP address, given the address and
 * the netmask.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr1, ipaddr2, netmask;

 uip_ipaddr(&ipaddr1, 192,16,1,2);
 uip_ipaddr(&netmask, 255,255,255,0);
 uip_ipaddr_mask(&ipaddr2, &ipaddr1, &netmask);
 \endcode
 *
 * In the example above, the variable "ipaddr2" will contain the IP
 * address 192.168.1.0.
 *
 * \param dest Where the result is to be placed.
 * \param src The IP address.
 * \param mask The netmask.
 *
 * \hideinitializer
 */
#define uip_ipaddr_mask(dest, src, mask) do {                           \
    ((uint16_t *)dest)[0] = ((uint16_t *)src)[0] & ((uint16_t *)mask)[0];        \
    ((uint16_t *)dest)[1] = ((uint16_t *)src)[1] & ((uint16_t *)mask)[1];        \
  } while(0)

/**
 * Pick the first octet of an IP address.
 *
 * Picks out the first octet of an IP address.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 uint8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr1(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 1.
 *
 * \hideinitializer
 */
#define uip_ipaddr1(addr) ((addr)->u8[0])

/**
 * Pick the second octet of an IP address.
 *
 * Picks out the second octet of an IP address.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 uint8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr2(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 2.
 *
 * \hideinitializer
 */
#define uip_ipaddr2(addr) ((addr)->u8[1])

/**
 * Pick the third octet of an IP address.
 *
 * Picks out the third octet of an IP address.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 uint8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr3(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 3.
 *
 * \hideinitializer
 */
#define uip_ipaddr3(addr) ((addr)->u8[2])

/**
 * Pick the fourth octet of an IP address.
 *
 * Picks out the fourth octet of an IP address.
 *
 * Example:
 \code
 uip_ipaddr_t ipaddr;
 uint8_t octet;

 uip_ipaddr(&ipaddr, 1,2,3,4);
 octet = uip_ipaddr4(&ipaddr);
 \endcode
 *
 * In the example above, the variable "octet" will contain the value 4.
 *
 * \hideinitializer
 */
#define uip_ipaddr4(addr) ((addr)->u8[3])

/** @} */

#if NETSTACK_CONF_WITH_IPV6
/** Length of the link local prefix */
#define UIP_LLPREF_LEN     10

/**
 * \brief Is IPv6 address a the unspecified address
 * a is of type uip_ipaddr_t
 */
#define uip_is_addr_loopback(a)                  \
  ((((a)->u16[0]) == 0) &&                       \
   (((a)->u16[1]) == 0) &&                       \
   (((a)->u16[2]) == 0) &&                       \
   (((a)->u16[3]) == 0) &&                       \
   (((a)->u16[4]) == 0) &&                       \
   (((a)->u16[5]) == 0) &&                       \
   (((a)->u16[6]) == 0) &&                       \
   (((a)->u8[14]) == 0) &&                       \
   (((a)->u8[15]) == 0x01))
/**
 * \brief Is IPv6 address a the unspecified address
 * a is of type uip_ipaddr_t
 */
#define uip_is_addr_unspecified(a)               \
  ((((a)->u16[0]) == 0) &&                       \
   (((a)->u16[1]) == 0) &&                       \
   (((a)->u16[2]) == 0) &&                       \
   (((a)->u16[3]) == 0) &&                       \
   (((a)->u16[4]) == 0) &&                       \
   (((a)->u16[5]) == 0) &&                       \
   (((a)->u16[6]) == 0) &&                       \
   (((a)->u16[7]) == 0))

/** \brief Is IPv6 address a the link local all-nodes multicast address */
#define uip_is_addr_linklocal_allnodes_mcast(a)     \
  ((((a)->u8[0]) == 0xff) &&                        \
   (((a)->u8[1]) == 0x02) &&                        \
   (((a)->u16[1]) == 0) &&                          \
   (((a)->u16[2]) == 0) &&                          \
   (((a)->u16[3]) == 0) &&                          \
   (((a)->u16[4]) == 0) &&                          \
   (((a)->u16[5]) == 0) &&                          \
   (((a)->u16[6]) == 0) &&                          \
   (((a)->u8[14]) == 0) &&                          \
   (((a)->u8[15]) == 0x01))

/** \brief Is IPv6 address a the link local all-routers multicast address */
#define uip_is_addr_linklocal_allrouters_mcast(a)     \
  ((((a)->u8[0]) == 0xff) &&                        \
   (((a)->u8[1]) == 0x02) &&                        \
   (((a)->u16[1]) == 0) &&                          \
   (((a)->u16[2]) == 0) &&                          \
   (((a)->u16[3]) == 0) &&                          \
   (((a)->u16[4]) == 0) &&                          \
   (((a)->u16[5]) == 0) &&                          \
   (((a)->u16[6]) == 0) &&                          \
   (((a)->u8[14]) == 0) &&                          \
   (((a)->u8[15]) == 0x02))

/**
 * \brief is addr (a) a link local unicast address, see RFC 4291
 *  i.e. is (a) on prefix FE80::/10
 *  a is of type uip_ipaddr_t*
 */
#define uip_is_addr_linklocal(a)                 \
  ((a)->u8[0] == 0xfe &&                         \
   (a)->u8[1] == 0x80)

/** \brief set IP address a to unspecified */
#define uip_create_unspecified(a) uip_ip6addr(a, 0, 0, 0, 0, 0, 0, 0, 0)

/** \brief set IP address a to the link local all-nodes multicast address */
#define uip_create_linklocal_allnodes_mcast(a) uip_ip6addr(a, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001)

/** \brief set IP address a to the link local all-routers multicast address */
#define uip_create_linklocal_allrouters_mcast(a) uip_ip6addr(a, 0xff02, 0, 0, 0, 0, 0, 0, 0x0002)
#define uip_create_linklocal_prefix(addr) do { \
    (addr)->u16[0] = UIP_HTONS(0xfe80);            \
    (addr)->u16[1] = 0;                        \
    (addr)->u16[2] = 0;                        \
    (addr)->u16[3] = 0;                        \
  } while(0)

/**
 * \brief  is addr (a) a solicited node multicast address, see RFC 4291
 *  a is of type uip_ipaddr_t*
 */
#define uip_is_addr_solicited_node(a)          \
  ((((a)->u8[0])  == 0xFF) &&                  \
   (((a)->u8[1])  == 0x02) &&                  \
   (((a)->u16[1]) == 0x00) &&                  \
   (((a)->u16[2]) == 0x00) &&                  \
   (((a)->u16[3]) == 0x00) &&                  \
   (((a)->u16[4]) == 0x00) &&                  \
   (((a)->u8[10]) == 0x00) &&                  \
   (((a)->u8[11]) == 0x01) &&                  \
   (((a)->u8[12]) == 0xFF))

/**
 * \brief put in b the solicited node address corresponding to address a
 * both a and b are of type uip_ipaddr_t*
 * */
#define uip_create_solicited_node(a, b)    \
  (((b)->u8[0]) = 0xFF);                        \
  (((b)->u8[1]) = 0x02);                        \
  (((b)->u16[1]) = 0);                          \
  (((b)->u16[2]) = 0);                          \
  (((b)->u16[3]) = 0);                          \
  (((b)->u16[4]) = 0);                          \
  (((b)->u8[10]) = 0);                          \
  (((b)->u8[11]) = 0x01);                       \
  (((b)->u8[12]) = 0xFF);                       \
  (((b)->u8[13]) = ((a)->u8[13]));              \
  (((b)->u16[7]) = ((a)->u16[7]))

/**
 * \brief was addr (a) forged based on the mac address m
 * a type is uip_ipaddr_t
 * m type is uiplladdr_t
 */
#if UIP_CONF_LL_802154
#define uip_is_addr_mac_addr_based(a, m) \
  ((((a)->u8[8])  == (((m)->addr[0]) ^ 0x02)) &&   \
   (((a)->u8[9])  == (m)->addr[1]) &&            \
   (((a)->u8[10]) == (m)->addr[2]) &&            \
   (((a)->u8[11]) == (m)->addr[3]) &&            \
   (((a)->u8[12]) == (m)->addr[4]) &&            \
   (((a)->u8[13]) == (m)->addr[5]) &&            \
   (((a)->u8[14]) == (m)->addr[6]) &&            \
   (((a)->u8[15]) == (m)->addr[7]))
#else

#define uip_is_addr_mac_addr_based(a, m) \
  ((((a)->u8[8])  == (((m)->addr[0]) | 0x02)) &&   \
   (((a)->u8[9])  == (m)->addr[1]) &&            \
   (((a)->u8[10]) == (m)->addr[2]) &&            \
   (((a)->u8[11]) == 0xff) &&            \
   (((a)->u8[12]) == 0xfe) &&            \
   (((a)->u8[13]) == (m)->addr[3]) &&            \
   (((a)->u8[14]) == (m)->addr[4]) &&            \
   (((a)->u8[15]) == (m)->addr[5]))

#endif /*UIP_CONF_LL_802154*/

/**
 * \brief is address a multicast address, see RFC 4291
 * a is of type uip_ipaddr_t*
 * */
#define uip_is_addr_mcast(a)                    \
  (((a)->u8[0]) == 0xFF)

/**
 * \brief is address a global multicast address (FFxE::/16),
 * a is of type uip_ip6addr_t*
 * */
#define uip_is_addr_mcast_global(a) \
  ((((a)->u8[0]) == 0xFF) && \
   (((a)->u8[1] & 0x0F) == 0x0E))

/**
 * \brief is address a non-routable multicast address.
 * Scopes 1 (interface-local) and 2 (link-local) are non-routable
 * See RFC4291 and draft-ietf-6man-multicast-scopes
 * a is of type uip_ip6addr_t*
 * */
#define uip_is_addr_mcast_non_routable(a) \
  ((((a)->u8[0]) == 0xFF) && \
   (((a)->u8[1] & 0x0F) <= 0x02))

/**
 * \brief is address a routable multicast address.
 * Scope 3 (Realm-Local) or higher are routable
 * Realm-Local scope is defined in draft-ietf-6man-multicast-scopes
 * See RFC4291 and draft-ietf-6man-multicast-scopes
 * a is of type uip_ip6addr_t*
 * */
#define uip_is_addr_mcast_routable(a) \
  ((((a)->u8[0]) == 0xFF) && \
   (((a)->u8[1] & 0x0F) > 0x02))

/**
 * \brief is group-id of multicast address a
 * the all nodes group-id
 */
#define uip_is_mcast_group_id_all_nodes(a) \
  ((((a)->u16[1])  == 0) &&                 \
   (((a)->u16[2])  == 0) &&                 \
   (((a)->u16[3])  == 0) &&                 \
   (((a)->u16[4])  == 0) &&                 \
   (((a)->u16[5])  == 0) &&                 \
   (((a)->u16[6])  == 0) &&                 \
   (((a)->u8[14])  == 0) &&                 \
   (((a)->u8[15])  == 1))

/**
 * \brief is group-id of multicast address a
 * the all routers group-id
 */
#define uip_is_mcast_group_id_all_routers(a) \
  ((((a)->u16[1])  == 0) &&                 \
   (((a)->u16[2])  == 0) &&                 \
   (((a)->u16[3])  == 0) &&                 \
   (((a)->u16[4])  == 0) &&                 \
   (((a)->u16[5])  == 0) &&                 \
   (((a)->u16[6])  == 0) &&                 \
   (((a)->u8[14])  == 0) &&                 \
   (((a)->u8[15])  == 2))


/**
 * \brief are last three bytes of both addresses equal?
 * This is used to compare solicited node multicast addresses
 */
#define uip_are_solicited_bytes_equal(a, b)             \
  ((((a)->u8[13])  == ((b)->u8[13])) &&                 \
   (((a)->u8[14])  == ((b)->u8[14])) &&                 \
   (((a)->u8[15])  == ((b)->u8[15])))

#endif /*NETSTACK_CONF_WITH_IPV6*/

/**  @} */

#endif /* OS_NET_IP_UIPADDR_H_ */
