/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * \addtogroup uip
 * @{
 */

/**
 * \file
 *    Neighbor discovery (RFC 4861)
 * \author Mathilde Durvy <mdurvy@cisco.com>
 * \author Julien Abeille <jabeille@cisco.com>
 */

#include <string.h>
#include <inttypes.h>
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/uip-nd6.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-nameserver.h"
#include "lib/random.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "IPv6 NDP"
#define LOG_LEVEL LOG_LEVEL_IPV6

/*------------------------------------------------------------------*/
/** @{ */
/** \name Pointers to the header structures.
 */

/**@{  Pointers to messages just after icmp header */
#define UIP_ND6_RS_BUF            ((uip_nd6_rs *)UIP_ICMP_PAYLOAD)
#define UIP_ND6_RA_BUF            ((uip_nd6_ra *)UIP_ICMP_PAYLOAD)
#define UIP_ND6_NS_BUF            ((uip_nd6_ns *)UIP_ICMP_PAYLOAD)
#define UIP_ND6_NA_BUF            ((uip_nd6_na *)UIP_ICMP_PAYLOAD)
/** @} */
/** Pointer to ND option */
#define ND6_OPT(opt)                         ((unsigned char *)(UIP_ICMP_PAYLOAD + (opt)))
#define ND6_OPT_HDR_BUF(opt)               ((uip_nd6_opt_hdr *)ND6_OPT(opt))
#define ND6_OPT_PREFIX_BUF(opt)    ((uip_nd6_opt_prefix_info *)ND6_OPT(opt))
#define ND6_OPT_MTU_BUF(opt)               ((uip_nd6_opt_mtu *)ND6_OPT(opt))
#define ND6_OPT_RDNSS_BUF(opt)             ((uip_nd6_opt_dns *)ND6_OPT(opt))
/** @} */

#if UIP_ND6_SEND_NS || UIP_ND6_SEND_NA || UIP_ND6_SEND_RA || !UIP_CONF_ROUTER
static uint16_t nd6_opt_offset; /** Offset from the end of the icmpv6 header to the option in uip_buf*/
static uint8_t *nd6_opt_llao;   /**  Pointer to llao option in uip_buf */
static uip_ds6_nbr_t *nbr; /**  Pointer to a nbr cache entry*/
static uip_ds6_addr_t *addr; /**  Pointer to an interface address */
#endif /* UIP_ND6_SEND_NS || UIP_ND6_SEND_NA || UIP_ND6_SEND_RA || !UIP_CONF_ROUTER */

#if UIP_ND6_SEND_NS || UIP_ND6_SEND_RA || !UIP_CONF_ROUTER
static uip_ds6_defrt_t *defrt; /**  Pointer to a router list entry */
#endif /* UIP_ND6_SEND_NS || UIP_ND6_SEND_RA || !UIP_CONF_ROUTER */

#if !UIP_CONF_ROUTER            /* TBD see if we move it to ra_input */
static uip_nd6_opt_prefix_info *nd6_opt_prefix_info; /**  Pointer to prefix information option in uip_buf */
static uip_ipaddr_t ipaddr;
#endif
#if (!UIP_CONF_ROUTER || UIP_ND6_SEND_RA)
static uip_ds6_prefix_t *prefix; /**  Pointer to a prefix list entry */
#endif

#if UIP_ND6_SEND_NA || UIP_ND6_SEND_RA || !UIP_CONF_ROUTER
/*------------------------------------------------------------------*/
/* Copy link-layer address from LLAO option to a word-aligned uip_lladdr_t */
static int
extract_lladdr_from_llao_aligned(uip_lladdr_t *dest)
{
  if(dest != NULL && nd6_opt_llao != NULL) {
    memcpy(dest, &nd6_opt_llao[UIP_ND6_OPT_DATA_OFFSET], UIP_LLADDR_LEN);
    return 1;
  }
  return 0;
}
#endif /* UIP_ND6_SEND_NA || UIP_ND6_SEND_RA || !UIP_CONF_ROUTER */
/*------------------------------------------------------------------*/
#if UIP_ND6_SEND_NA /* UIP_ND6_SEND_NA */
/* create a llao */
static void
create_llao(uint8_t *llao, uint8_t type)
{
  llao[UIP_ND6_OPT_TYPE_OFFSET] = type;
  llao[UIP_ND6_OPT_LEN_OFFSET] = UIP_ND6_OPT_LLAO_LEN >> 3;
  memcpy(&llao[UIP_ND6_OPT_DATA_OFFSET], &uip_lladdr, UIP_LLADDR_LEN);
  /* padding on some */
  memset(&llao[UIP_ND6_OPT_DATA_OFFSET + UIP_LLADDR_LEN], 0,
         UIP_ND6_OPT_LLAO_LEN - 2 - UIP_LLADDR_LEN);
}
#endif /* UIP_ND6_SEND_NA */
/*------------------------------------------------------------------*/
/**
 * Neighbor Solicitation Processing
 *
 * The NS can be received in 3 cases (procedures):
 * - sender is performing DAD (ip src = unspecified, no SLLAO option)
 * - sender is performing NUD (ip dst = unicast)
 * - sender is performing address resolution (ip dest = solicited node mcast
 * address)
 *
 * We do:
 * - if the tgt belongs to me, reply, otherwise ignore
 * - if i was performing DAD for the same address, two cases:
 * -- I already sent a NS, hence I win
 * -- I did not send a NS yet, hence I lose
 *
 * If we need to send a NA in response (i.e. the NS was done for NUD, or
 * address resolution, or DAD and there is a conflict), we do it in this
 * function: set src, dst, tgt address in the three cases, then for all cases
 * set the rest, including  SLLAO
 *
 */
#if UIP_ND6_SEND_NA
static void
ns_input(void)
{
  if(uip_l3_icmp_hdr_len + sizeof(uip_nd6_ns) > uip_len) {
    LOG_ERR("Insufficient data for reading ND6 NS header fields");
    goto discard;
  }

  uint8_t flags = 0;

  LOG_INFO("Received NS from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(" with target address ");
  LOG_INFO_6ADDR((uip_ipaddr_t *)(&UIP_ND6_NS_BUF->tgtipaddr));
  LOG_INFO_("\n");
  UIP_STAT(++uip_stat.nd6.recv);

  if((UIP_IP_BUF->ttl != UIP_ND6_HOP_LIMIT) ||
     (uip_is_addr_mcast(&UIP_ND6_NS_BUF->tgtipaddr)) ||
     (UIP_ICMP_BUF->icode != 0)) {
    LOG_ERR("Discarding invalid NS\n");
    goto discard;
  }

  /* Options processing */
  nd6_opt_llao = NULL;
  nd6_opt_offset = UIP_ND6_NS_LEN;
  while(uip_l3_icmp_hdr_len + nd6_opt_offset + UIP_ND6_OPT_HDR_LEN < uip_len) {
    if(ND6_OPT_HDR_BUF(nd6_opt_offset)->len == 0) {
      LOG_ERR("Discarding invalid NS\n");
      goto discard;
    }

    switch(ND6_OPT_HDR_BUF(nd6_opt_offset)->type) {
    case UIP_ND6_OPT_SLLAO:
      if(uip_l3_icmp_hdr_len + nd6_opt_offset +
         UIP_ND6_OPT_DATA_OFFSET + UIP_LLADDR_LEN > uip_len) {
        LOG_ERR("Insufficient data for NS SLLAO option\n");
        goto discard;
      }
      nd6_opt_llao = &uip_buf[uip_l3_icmp_hdr_len + nd6_opt_offset];

      /* There must be NO option in a DAD NS */
      if(uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) {
        LOG_ERR("Discarding invalid NS\n");
        goto discard;
      } else {
        uip_lladdr_t lladdr_aligned;
        extract_lladdr_from_llao_aligned(&lladdr_aligned);
        nbr = uip_ds6_nbr_lookup(&UIP_IP_BUF->srcipaddr);
        if(nbr == NULL) {
          uip_ds6_nbr_add(&UIP_IP_BUF->srcipaddr, &lladdr_aligned,
                          0, NBR_STALE, NBR_TABLE_REASON_IPV6_ND, NULL);
        } else {
          const uip_lladdr_t *lladdr = uip_ds6_nbr_get_ll(nbr);
          if(lladdr == NULL) {
            goto discard;
          }
          if(memcmp(&nd6_opt_llao[UIP_ND6_OPT_DATA_OFFSET],
                    lladdr, UIP_LLADDR_LEN) != 0) {
            if(uip_ds6_nbr_update_ll(&nbr,
                                     (const uip_lladdr_t *)&lladdr_aligned)
               < 0) {
              /* failed to update the lladdr */
              goto discard;
            }
            nbr->state = NBR_STALE;
          } else {
            if(nbr->state == NBR_INCOMPLETE) {
              nbr->state = NBR_STALE;
            }
          }
        }
      }
      break;
    default:
      LOG_WARN("ND option not supported in NS");
      break;
    }
    nd6_opt_offset += (ND6_OPT_HDR_BUF(nd6_opt_offset)->len << 3);
  }

  addr = uip_ds6_addr_lookup(&UIP_ND6_NS_BUF->tgtipaddr);
  if(addr != NULL) {
    if(uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) {
      /* DAD CASE */
#if UIP_ND6_DEF_MAXDADNS > 0
      if(!uip_is_addr_solicited_node(&UIP_IP_BUF->destipaddr)) {
        LOG_ERR("Discarding invalid NS\n");
        goto discard;
      }

      if(addr->state != ADDR_TENTATIVE) {
        uip_create_linklocal_allnodes_mcast(&UIP_IP_BUF->destipaddr);
        uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);
        flags = UIP_ND6_NA_FLAG_OVERRIDE;
        goto create_na;
      } else {
        /** \todo if I sent a NS before him, I win */
        uip_ds6_dad_failed(addr);
        goto discard;
      }
#else /* UIP_ND6_DEF_MAXDADNS > 0 */
      goto discard;  /* DAD CASE */
#endif /* UIP_ND6_DEF_MAXDADNS > 0 */
    }

    if(uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)) {
      /**
       * \NOTE do we do something here? we both are using the same address.
       * If we are doing dad, we could cancel it, though we should receive a
       * NA in response of DAD NS we sent, hence DAD will fail anyway. If we
       * were not doing DAD, it means there is a duplicate in the network!
       */
      LOG_ERR("Discarding invalid NS\n");
      goto discard;
    }

    /* Address resolution case */
    if(uip_is_addr_solicited_node(&UIP_IP_BUF->destipaddr)) {
      uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &UIP_IP_BUF->srcipaddr);
      uip_ipaddr_copy(&UIP_IP_BUF->srcipaddr, &UIP_ND6_NS_BUF->tgtipaddr);
      flags = UIP_ND6_NA_FLAG_SOLICITED | UIP_ND6_NA_FLAG_OVERRIDE;
      goto create_na;
    }

    /* NUD CASE */
    if(uip_ds6_addr_lookup(&UIP_IP_BUF->destipaddr) == addr) {
      uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &UIP_IP_BUF->srcipaddr);
      uip_ipaddr_copy(&UIP_IP_BUF->srcipaddr, &UIP_ND6_NS_BUF->tgtipaddr);
      flags = UIP_ND6_NA_FLAG_SOLICITED | UIP_ND6_NA_FLAG_OVERRIDE;
      goto create_na;
    } else {
      LOG_ERR("Discarding invalid NS\n");
      goto discard;
    }
  } else {
    goto discard;
  }

create_na:
  /* If the node is a router it should set R flag in NAs */
#if UIP_CONF_ROUTER
  flags = flags | UIP_ND6_NA_FLAG_ROUTER;
#endif
  uipbuf_clear();
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  uipbuf_set_len_field(UIP_IP_BUF, UIP_ICMPH_LEN + UIP_ND6_NA_LEN + UIP_ND6_OPT_LLAO_LEN);
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = UIP_ND6_HOP_LIMIT;

  UIP_ICMP_BUF->type = ICMP6_NA;
  UIP_ICMP_BUF->icode = 0;

  UIP_ND6_NA_BUF->flagsreserved = flags;
  memcpy(&UIP_ND6_NA_BUF->tgtipaddr, &addr->ipaddr, sizeof(uip_ipaddr_t));

  create_llao(&uip_buf[uip_l3_icmp_hdr_len + UIP_ND6_NA_LEN],
              UIP_ND6_OPT_TLLAO);

  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();

  uipbuf_set_len(UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_NA_LEN + UIP_ND6_OPT_LLAO_LEN);

  UIP_STAT(++uip_stat.nd6.sent);
  LOG_INFO("Sending NA to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(" from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" with target address ");
  LOG_INFO_6ADDR(&UIP_ND6_NA_BUF->tgtipaddr);
  LOG_INFO_("\n");
  return;

discard:
  uipbuf_clear();
  return;
}
#endif /* UIP_ND6_SEND_NA */

/*------------------------------------------------------------------*/
#if UIP_ND6_SEND_NS
void
uip_nd6_ns_output(const uip_ipaddr_t *src, const uip_ipaddr_t *dest,
                  uip_ipaddr_t *tgt)
{
  uipbuf_clear();
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = UIP_ND6_HOP_LIMIT;

  if(dest == NULL) {
    uip_create_solicited_node(tgt, &UIP_IP_BUF->destipaddr);
  } else {
    uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, dest);
  }
  UIP_ICMP_BUF->type = ICMP6_NS;
  UIP_ICMP_BUF->icode = 0;
  UIP_ND6_NS_BUF->reserved = 0;
  uip_ipaddr_copy((uip_ipaddr_t *)&UIP_ND6_NS_BUF->tgtipaddr, tgt);
  /*
   * check if we add a SLLAO option: for DAD, MUST NOT, for NUD, MAY
   * (here yes), for Address resolution , MUST
   */
  if(!(uip_ds6_is_my_addr(tgt))) {
    if(src != NULL) {
      uip_ipaddr_copy(&UIP_IP_BUF->srcipaddr, src);
    } else {
      uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);
    }
    if(uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) {
      LOG_ERR("Dropping NS due to no suitable source address\n");
      uipbuf_clear();
      return;
    }
    uipbuf_set_len_field(UIP_IP_BUF, UIP_ICMPH_LEN + UIP_ND6_NS_LEN + UIP_ND6_OPT_LLAO_LEN);

    create_llao(&uip_buf[uip_l3_icmp_hdr_len + UIP_ND6_NS_LEN],
                UIP_ND6_OPT_SLLAO);

    uip_len =
      UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_NS_LEN + UIP_ND6_OPT_LLAO_LEN;
  } else {
    uip_create_unspecified(&UIP_IP_BUF->srcipaddr);
    UIP_IP_BUF->len[1] = UIP_ICMPH_LEN + UIP_ND6_NS_LEN;
    uip_len = UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_NS_LEN;
  }

  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();

  UIP_STAT(++uip_stat.nd6.sent);
  LOG_INFO("Sending NS to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(" from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" with target address ");
  LOG_INFO_6ADDR(tgt);
  LOG_INFO_("\n");
  return;
}
#endif /* UIP_ND6_SEND_NS */

#if UIP_ND6_SEND_NS
/*------------------------------------------------------------------*/
/**
 * Neighbor Advertisement Processing
 *
 * we might have to send a pkt that had been buffered while address
 * resolution was performed (if we support buffering, see UIP_CONF_QUEUE_PKT)
 *
 * As per RFC 4861, on link layer that have addresses, TLLAO options MUST be
 * included when responding to multicast solicitations, SHOULD be included in
 * response to unicast (here we assume it is for now)
 *
 * NA can be received after sending NS for DAD, Address resolution or NUD. Can
 * be unsolicited as well.
 * It can trigger update of the state of the neighbor in the neighbor cache,
 * router in the router list.
 * If the NS was for DAD, it means DAD failed
 *
 */
static void
na_input(void)
{
  uint8_t is_llchange;
  uint8_t is_router;
  uint8_t is_solicited;
  uint8_t is_override;
  uip_lladdr_t lladdr_aligned;

  LOG_INFO("Received NA from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(" with target address ");
  LOG_INFO_6ADDR((uip_ipaddr_t *)(&UIP_ND6_NA_BUF->tgtipaddr));
  LOG_INFO_("\n");
  UIP_STAT(++uip_stat.nd6.recv);

  /*
   * booleans. the three last one are not 0 or 1 but 0 or 0x80, 0x40, 0x20
   * but it works. Be careful though, do not use tests such as is_router == 1
   */
  is_llchange = 0;
  is_router = ((UIP_ND6_NA_BUF->flagsreserved & UIP_ND6_NA_FLAG_ROUTER));
  is_solicited =
    ((UIP_ND6_NA_BUF->flagsreserved & UIP_ND6_NA_FLAG_SOLICITED));
  is_override =
    ((UIP_ND6_NA_BUF->flagsreserved & UIP_ND6_NA_FLAG_OVERRIDE));

  if((UIP_IP_BUF->ttl != UIP_ND6_HOP_LIMIT) ||
     (UIP_ICMP_BUF->icode != 0) ||
     (uip_is_addr_mcast(&UIP_ND6_NA_BUF->tgtipaddr)) ||
     (is_solicited && uip_is_addr_mcast(&UIP_IP_BUF->destipaddr))) {
    LOG_ERR("Discarding invalid NA\n");
    goto discard;
  }

  /* Options processing: we handle TLLAO, and must ignore others */
  nd6_opt_offset = UIP_ND6_NA_LEN;
  nd6_opt_llao = NULL;
  while(uip_l3_icmp_hdr_len + nd6_opt_offset < uip_len) {
    if(ND6_OPT_HDR_BUF(nd6_opt_offset)->len == 0) {
      LOG_ERR("Discarding invalid NA\n");
      goto discard;
    }

    switch(ND6_OPT_HDR_BUF(nd6_opt_offset)->type) {
    case UIP_ND6_OPT_TLLAO:
      nd6_opt_llao = (uint8_t *)ND6_OPT_HDR_BUF(nd6_opt_offset);
      break;
    default:
      LOG_WARN("ND option not supported in NA\n");
      break;
    }
    nd6_opt_offset += (ND6_OPT_HDR_BUF(nd6_opt_offset)->len << 3);
  }
  addr = uip_ds6_addr_lookup(&UIP_ND6_NA_BUF->tgtipaddr);
  /* Message processing, including TLLAO if any */
  if(addr != NULL) {
#if UIP_ND6_DEF_MAXDADNS > 0
    if(addr->state == ADDR_TENTATIVE) {
      uip_ds6_dad_failed(addr);
    }
#endif /*UIP_ND6_DEF_MAXDADNS > 0 */
    LOG_ERR("Discarding invalid NA\n");
    goto discard;
  } else {
    const uip_lladdr_t *lladdr;
    nbr = uip_ds6_nbr_lookup(&UIP_ND6_NA_BUF->tgtipaddr);
    if(nbr == NULL) {
      goto discard;
    }
    lladdr = uip_ds6_nbr_get_ll(nbr);
    if(lladdr == NULL) {
      goto discard;
    }
    if(nd6_opt_llao != NULL) {
      is_llchange =
        memcmp(&nd6_opt_llao[UIP_ND6_OPT_DATA_OFFSET], lladdr,
               UIP_LLADDR_LEN) == 0 ? 0 : 1;
    }
    if(nbr->state == NBR_INCOMPLETE) {
      if(nd6_opt_llao == NULL || !extract_lladdr_from_llao_aligned(&lladdr_aligned)) {
        goto discard;
      }
      if(uip_ds6_nbr_update_ll(&nbr,
                               (const uip_lladdr_t *)&lladdr_aligned) < 0) {
        /* failed to update the lladdr */
        goto discard;
      }

      /* Note: No need to refresh the state of the nbr here.
       * It has already been refreshed upon receiving the unicast IPv6 ND packet.
       * See: uip_ds6_nbr_refresh_reachable_state()
       */
      if(!is_solicited) {
        nbr->state = NBR_STALE;
      }
      nbr->isrouter = is_router;
    } else { /* NBR is not INCOMPLETE */
      if(!is_override && is_llchange) {
        if(nbr->state == NBR_REACHABLE) {
          nbr->state = NBR_STALE;
        }
        goto discard;
      } else {
        /**
         *  If this is an cache override, or same lladdr, or no llao -
         *  do updates of nbr states.
         */
        if(is_override || !is_llchange || nd6_opt_llao == NULL) {
          if(nd6_opt_llao != NULL && is_llchange) {
            if(!extract_lladdr_from_llao_aligned(&lladdr_aligned) ||
               uip_ds6_nbr_update_ll(&nbr,
                                     (const uip_lladdr_t *)&lladdr_aligned)
               < 0) {
              /* failed to update the lladdr */
              goto discard;
            }
          }
          /* Note: No need to refresh the state of the nbr here.
           * It has already been refreshed upon receiving the unicast IPv6 ND packet.
           * See: uip_ds6_nbr_refresh_reachable_state()
           */
        }
      }
      if(nbr->isrouter && !is_router) {
        defrt = uip_ds6_defrt_lookup(&UIP_IP_BUF->srcipaddr);
        if(defrt != NULL) {
          uip_ds6_defrt_rm(defrt);
        }
      }
      nbr->isrouter = is_router;
    }
  }
#if UIP_CONF_IPV6_QUEUE_PKT
  /* The nbr is now reachable, check if we had buffered a pkt for it */
  if(uip_packetqueue_buflen(&nbr->packethandle) != 0) {
    uip_len = uip_packetqueue_buflen(&nbr->packethandle);
    memcpy(UIP_IP_BUF, uip_packetqueue_buf(&nbr->packethandle), uip_len);
    uip_packetqueue_free(&nbr->packethandle);
    return;
  }

#endif /*UIP_CONF_IPV6_QUEUE_PKT */

discard:
  uipbuf_clear();
  return;
}
#endif /* UIP_ND6_SEND_NS */

#if UIP_CONF_ROUTER
#if UIP_ND6_SEND_RA
/*---------------------------------------------------------------------------*/
static void
rs_input(void)
{

  LOG_INFO("Received RS from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_("\n");
  UIP_STAT(++uip_stat.nd6.recv);

  /*
   * Check hop limit / icmp code
   * target address must not be multicast
   * if the NA is solicited, dest must not be multicast
   */
  if((UIP_IP_BUF->ttl != UIP_ND6_HOP_LIMIT) || (UIP_ICMP_BUF->icode != 0)) {
    LOG_ERR("Discarding invalid RS\n");
    goto discard;
  }

  /* Only valid option is Source Link-Layer Address option any thing
     else is discarded */
  nd6_opt_offset = UIP_ND6_RS_LEN;
  nd6_opt_llao = NULL;

  while(uip_l3_icmp_hdr_len + nd6_opt_offset < uip_len) {
    if(ND6_OPT_HDR_BUF(nd6_opt_offset)->len == 0) {
      LOG_ERR("Discarding invalid RS\n");
      goto discard;
    }

    switch(ND6_OPT_HDR_BUF(nd6_opt_offset)->type) {
    case UIP_ND6_OPT_SLLAO:
      nd6_opt_llao = (uint8_t *)ND6_OPT_HDR_BUF(nd6_opt_offset);
      break;
    default:
      LOG_WARN("ND option not supported in RS\n");
      break;
    }
    nd6_opt_offset += (ND6_OPT_HDR_BUF(nd6_opt_offset)->len << 3);
  }
  /* Options processing: only SLLAO */
  if(nd6_opt_llao != NULL) {
    if(uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) {
      LOG_ERR("Discarding invalid RS\n");
      goto discard;
    } else {
      uip_lladdr_t lladdr_aligned;
      extract_lladdr_from_llao_aligned(&lladdr_aligned);
      if((nbr = uip_ds6_nbr_lookup(&UIP_IP_BUF->srcipaddr)) == NULL) {
        /* we need to add the neighbor */
        uip_ds6_nbr_add(&UIP_IP_BUF->srcipaddr, &lladdr_aligned,
                        0, NBR_STALE, NBR_TABLE_REASON_IPV6_ND, NULL);
      } else {
        /* If LL address changed, set neighbor state to stale */
        const uip_lladdr_t *lladdr = uip_ds6_nbr_get_ll(nbr);
        if(lladdr == NULL) {
          goto discard;
        }
        if(memcmp(&nd6_opt_llao[UIP_ND6_OPT_DATA_OFFSET],
                  lladdr, UIP_LLADDR_LEN) != 0) {
          uip_ds6_nbr_t nbr_data;
          nbr_data = *nbr;
          uip_ds6_nbr_rm(nbr);
          nbr = uip_ds6_nbr_add(&UIP_IP_BUF->srcipaddr, &lladdr_aligned,
                                0, NBR_STALE, NBR_TABLE_REASON_IPV6_ND, NULL);
          if(nbr == NULL) {
            goto discard;
          }
          nbr->reachable = nbr_data.reachable;
          nbr->sendns = nbr_data.sendns;
          nbr->nscount = nbr_data.nscount;
        }
        nbr->isrouter = 0;
      }
    }
  }

  /* Schedule a sollicited RA */
  uip_ds6_send_ra_sollicited();

discard:
  uipbuf_clear();
  return;
}
/*---------------------------------------------------------------------------*/
void
uip_nd6_ra_output(const uip_ipaddr_t *dest)
{

  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = UIP_ND6_HOP_LIMIT;

  if(dest == NULL) {
    uip_create_linklocal_allnodes_mcast(&UIP_IP_BUF->destipaddr);
  } else {
    /* For sollicited RA */
    uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, dest);
  }
  uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);

  UIP_ICMP_BUF->type = ICMP6_RA;
  UIP_ICMP_BUF->icode = 0;

  UIP_ND6_RA_BUF->cur_ttl = uip_ds6_if.cur_hop_limit;

  UIP_ND6_RA_BUF->flags_reserved =
    (UIP_ND6_M_FLAG << 7) | (UIP_ND6_O_FLAG << 6);

  UIP_ND6_RA_BUF->router_lifetime = uip_htons(UIP_ND6_ROUTER_LIFETIME);
  UIP_ND6_RA_BUF->reachable_time = 0;
  UIP_ND6_RA_BUF->retrans_timer = 0;

  uip_len = UIP_IPH_LEN + UIP_ICMPH_LEN + UIP_ND6_RA_LEN;
  nd6_opt_offset = UIP_ND6_RA_LEN;

  /* Prefix list */
  for(prefix = uip_ds6_prefix_list;
      prefix < uip_ds6_prefix_list + UIP_DS6_PREFIX_NB; prefix++) {
    if((prefix->isused) && (prefix->advertise)) {
      ND6_OPT_PREFIX_BUF(nd6_opt_offset)->type = UIP_ND6_OPT_PREFIX_INFO;
      ND6_OPT_PREFIX_BUF(nd6_opt_offset)->len = UIP_ND6_OPT_PREFIX_INFO_LEN / 8;
      ND6_OPT_PREFIX_BUF(nd6_opt_offset)->preflen = prefix->length;
      ND6_OPT_PREFIX_BUF(nd6_opt_offset)->flagsreserved1 = prefix->l_a_reserved;
      ND6_OPT_PREFIX_BUF(nd6_opt_offset)->validlt = uip_htonl(prefix->vlifetime);
      ND6_OPT_PREFIX_BUF(nd6_opt_offset)->preferredlt = uip_htonl(prefix->plifetime);
      ND6_OPT_PREFIX_BUF(nd6_opt_offset)->reserved2 = 0;
      uip_ipaddr_copy(&(ND6_OPT_PREFIX_BUF(nd6_opt_offset)->prefix), &(prefix->ipaddr));
      nd6_opt_offset += UIP_ND6_OPT_PREFIX_INFO_LEN;
      uip_len += UIP_ND6_OPT_PREFIX_INFO_LEN;
    }
  }

  /* Source link-layer option */
  create_llao((uint8_t *)ND6_OPT_HDR_BUF(nd6_opt_offset), UIP_ND6_OPT_SLLAO);

  uip_len += UIP_ND6_OPT_LLAO_LEN;
  nd6_opt_offset += UIP_ND6_OPT_LLAO_LEN;

  /* MTU */
  ND6_OPT_MTU_BUF(nd6_opt_offset)->type = UIP_ND6_OPT_MTU;
  ND6_OPT_MTU_BUF(nd6_opt_offset)->len = UIP_ND6_OPT_MTU_LEN >> 3;
  ND6_OPT_MTU_BUF(nd6_opt_offset)->reserved = 0;
  ND6_OPT_MTU_BUF(nd6_opt_offset)->mtu = uip_htonl(1500);

  uip_len += UIP_ND6_OPT_MTU_LEN;
  nd6_opt_offset += UIP_ND6_OPT_MTU_LEN;

#if UIP_ND6_RA_RDNSS
  if(uip_nameserver_count() > 0) {
    uint8_t i = 0;
    uip_ipaddr_t *ip = &ND6_OPT_RDNSS_BUF(nd6_opt_offset)->ip;
    uip_ipaddr_t *dns = NULL;
    ND6_OPT_RDNSS_BUF(nd6_opt_offset)->type = UIP_ND6_OPT_RDNSS;
    ND6_OPT_RDNSS_BUF(nd6_opt_offset)->reserved = 0;
    ND6_OPT_RDNSS_BUF(nd6_opt_offset)->lifetime = uip_nameserver_next_expiration();
    if(ND6_OPT_RDNSS_BUF(nd6_opt_offset)->lifetime != UIP_NAMESERVER_INFINITE_LIFETIME) {
      ND6_OPT_RDNSS_BUF(nd6_opt_offset)->lifetime -= clock_seconds();
    }
    while((dns = uip_nameserver_get(i)) != NULL) {
      uip_ipaddr_copy(ip++, dns);
      i++;
    }
    ND6_OPT_RDNSS_BUF(nd6_opt_offset)->len = UIP_ND6_OPT_RDNSS_LEN + (i << 1);
    LOG_INFO("%d nameservers reported\n", i);
    uip_len += ND6_OPT_RDNSS_BUF(nd6_opt_offset)->len << 3;
    nd6_opt_offset += ND6_OPT_RDNSS_BUF(nd6_opt_offset)->len << 3;
  }
#endif /* UIP_ND6_RA_RDNSS */

  uipbuf_set_len_field(UIP_IP_BUF, uip_len - UIP_IPH_LEN);

  /*ICMP checksum */
  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();

  UIP_STAT(++uip_stat.nd6.sent);
  LOG_INFO("Sending RA to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(" from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_("\n");
  return;
}
#endif /* UIP_ND6_SEND_RA */
#endif /* UIP_CONF_ROUTER */

#if !UIP_CONF_ROUTER
/*---------------------------------------------------------------------------*/
void
uip_nd6_rs_output(void)
{
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->tcflow = 0;
  UIP_IP_BUF->flow = 0;
  UIP_IP_BUF->proto = UIP_PROTO_ICMP6;
  UIP_IP_BUF->ttl = UIP_ND6_HOP_LIMIT;
  uip_create_linklocal_allrouters_mcast(&UIP_IP_BUF->destipaddr);
  uip_ds6_select_src(&UIP_IP_BUF->srcipaddr, &UIP_IP_BUF->destipaddr);
  UIP_ICMP_BUF->type = ICMP6_RS;
  UIP_ICMP_BUF->icode = 0;

  if(uip_is_addr_unspecified(&UIP_IP_BUF->srcipaddr)) {
    UIP_IP_BUF->len[1] = UIP_ICMPH_LEN + UIP_ND6_RS_LEN;
    uip_len = uip_l3_icmp_hdr_len + UIP_ND6_RS_LEN;
  } else {
    uip_len = uip_l3_icmp_hdr_len + UIP_ND6_RS_LEN + UIP_ND6_OPT_LLAO_LEN;
    uipbuf_set_len_field(UIP_IP_BUF, UIP_ICMPH_LEN + UIP_ND6_RS_LEN + UIP_ND6_OPT_LLAO_LEN);

    create_llao(&uip_buf[uip_l3_icmp_hdr_len + UIP_ND6_RS_LEN],
                UIP_ND6_OPT_SLLAO);
  }

  UIP_ICMP_BUF->icmpchksum = 0;
  UIP_ICMP_BUF->icmpchksum = ~uip_icmp6chksum();

  UIP_STAT(++uip_stat.nd6.sent);
  LOG_INFO("Sending RS to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(" from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_("\n");
  return;
}
/*---------------------------------------------------------------------------*/
/**
 * Process a Router Advertisement
 *
 * - Possible actions when receiving a RA: add router to router list,
 *   recalculate reachable time, update link hop limit, update retrans timer.
 * - If MTU option: update MTU.
 * - If SLLAO option: update entry in neighbor cache
 * - If prefix option: start autoconf, add prefix to prefix list
 */
void
ra_input(void)
{
  uip_lladdr_t lladdr_aligned;

  LOG_INFO("Received RA from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_("\n");
  UIP_STAT(++uip_stat.nd6.recv);

  if((UIP_IP_BUF->ttl != UIP_ND6_HOP_LIMIT) ||
     (!uip_is_addr_linklocal(&UIP_IP_BUF->srcipaddr)) ||
     (UIP_ICMP_BUF->icode != 0)) {
    LOG_ERR("Discarding invalid RA");
    goto discard;
  }

  if(UIP_ND6_RA_BUF->cur_ttl != 0) {
    uip_ds6_if.cur_hop_limit = UIP_ND6_RA_BUF->cur_ttl;
    LOG_INFO("uip_ds6_if.cur_hop_limit %u\n", uip_ds6_if.cur_hop_limit);
  }

  if(UIP_ND6_RA_BUF->reachable_time != 0) {
    if(uip_ds6_if.base_reachable_time !=
       uip_ntohl(UIP_ND6_RA_BUF->reachable_time)) {
      uip_ds6_if.base_reachable_time = uip_ntohl(UIP_ND6_RA_BUF->reachable_time);
      uip_ds6_if.reachable_time = uip_ds6_compute_reachable_time();
    }
  }
  if(UIP_ND6_RA_BUF->retrans_timer != 0) {
    uip_ds6_if.retrans_timer = uip_ntohl(UIP_ND6_RA_BUF->retrans_timer);
  }

  /* Options processing */
  nd6_opt_offset = UIP_ND6_RA_LEN;
  while(uip_l3_icmp_hdr_len + nd6_opt_offset < uip_len) {
    if(ND6_OPT_HDR_BUF(nd6_opt_offset)->len == 0) {
      LOG_ERR("Discarding invalid RA");
      goto discard;
    }
    switch(ND6_OPT_HDR_BUF(nd6_opt_offset)->type) {
    case UIP_ND6_OPT_SLLAO:
      LOG_DBG("Processing SLLAO option in RA\n");
      nd6_opt_llao = (uint8_t *)ND6_OPT_HDR_BUF(nd6_opt_offset);
      nbr = uip_ds6_nbr_lookup(&UIP_IP_BUF->srcipaddr);
      if(!extract_lladdr_from_llao_aligned(&lladdr_aligned)) {
        /* failed to extract llao - discard packet */
        goto discard;
      }
      if(nbr == NULL) {
        nbr = uip_ds6_nbr_add(&UIP_IP_BUF->srcipaddr, &lladdr_aligned,
                              1, NBR_STALE, NBR_TABLE_REASON_IPV6_ND, NULL);
      } else {
        const uip_lladdr_t *lladdr = uip_ds6_nbr_get_ll(nbr);
        if(lladdr == NULL) {
          goto discard;
        }
        if(nbr->state == NBR_INCOMPLETE) {
          nbr->state = NBR_STALE;
        }
        if(memcmp(&nd6_opt_llao[UIP_ND6_OPT_DATA_OFFSET],
                  lladdr, UIP_LLADDR_LEN) != 0) {
          /* change of link layer address */
          if(uip_ds6_nbr_update_ll(&nbr,
                                   (const uip_lladdr_t *)&lladdr_aligned) < 0) {
            /* failed to update the lladdr */
            goto discard;
          }
          nbr->state = NBR_STALE;
        }
        nbr->isrouter = 1;
      }
      break;
    case UIP_ND6_OPT_MTU:
      LOG_DBG("Processing MTU option in RA\n");
      uip_ds6_if.link_mtu =
        uip_ntohl(((uip_nd6_opt_mtu *)ND6_OPT_HDR_BUF(nd6_opt_offset))->mtu);
      break;
    case UIP_ND6_OPT_PREFIX_INFO:
      LOG_DBG("Processing PREFIX option in RA\n");
      nd6_opt_prefix_info = (uip_nd6_opt_prefix_info *)ND6_OPT_HDR_BUF(nd6_opt_offset);
      if((uip_ntohl(nd6_opt_prefix_info->validlt) >=
          uip_ntohl(nd6_opt_prefix_info->preferredlt))
         && (!uip_is_addr_linklocal(&nd6_opt_prefix_info->prefix))) {
        /* on-link flag related processing */
        if(nd6_opt_prefix_info->flagsreserved1 & UIP_ND6_RA_FLAG_ONLINK) {
          prefix =
            uip_ds6_prefix_lookup(&nd6_opt_prefix_info->prefix,
                                  nd6_opt_prefix_info->preflen);
          if(prefix == NULL) {
            if(nd6_opt_prefix_info->validlt != 0) {
              if(nd6_opt_prefix_info->validlt != UIP_ND6_INFINITE_LIFETIME) {
                prefix = uip_ds6_prefix_add(&nd6_opt_prefix_info->prefix,
                                            nd6_opt_prefix_info->preflen,
                                            uip_ntohl(nd6_opt_prefix_info->
                                                      validlt));
              } else {
                prefix = uip_ds6_prefix_add(&nd6_opt_prefix_info->prefix,
                                            nd6_opt_prefix_info->preflen, 0);
              }
            }
          } else {
            switch(nd6_opt_prefix_info->validlt) {
            case 0:
              uip_ds6_prefix_rm(prefix);
              break;
            case UIP_ND6_INFINITE_LIFETIME:
              prefix->isinfinite = 1;
              break;
            default:
              LOG_DBG("Updating timer of prefix ");
              LOG_DBG_6ADDR(&prefix->ipaddr);
              LOG_DBG_(" new value %" PRIu32 "\n", uip_ntohl(nd6_opt_prefix_info->validlt));
              stimer_set(&prefix->vlifetime,
                         uip_ntohl(nd6_opt_prefix_info->validlt));
              prefix->isinfinite = 0;
              break;
            }
          }
        }
        /* End of on-link flag related processing */
        /* autonomous flag related processing */
        if((nd6_opt_prefix_info->flagsreserved1 & UIP_ND6_RA_FLAG_AUTONOMOUS)
           && (nd6_opt_prefix_info->validlt != 0)
           && (nd6_opt_prefix_info->preflen == UIP_DEFAULT_PREFIX_LEN)) {

          uip_ipaddr_copy(&ipaddr, &nd6_opt_prefix_info->prefix);
          uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
          addr = uip_ds6_addr_lookup(&ipaddr);
          if((addr != NULL) && (addr->type == ADDR_AUTOCONF)) {
            if(nd6_opt_prefix_info->validlt != UIP_ND6_INFINITE_LIFETIME) {
              /* The processing below is defined in RFC4862 section 5.5.3 e */
              if((uip_ntohl(nd6_opt_prefix_info->validlt) > 2 * 60 * 60) ||
                 (uip_ntohl(nd6_opt_prefix_info->validlt) >
                  stimer_remaining(&addr->vlifetime))) {
                LOG_DBG("Updating timer of address ");
                LOG_DBG_6ADDR(&addr->ipaddr);
                LOG_DBG_(" new value %lu\n",
                         (unsigned long)uip_ntohl(nd6_opt_prefix_info->validlt));
                stimer_set(&addr->vlifetime,
                           uip_ntohl(nd6_opt_prefix_info->validlt));
              } else {
                stimer_set(&addr->vlifetime, 2 * 60 * 60);
                LOG_DBG("Updating timer of address ");
                LOG_DBG_6ADDR(&addr->ipaddr);
                LOG_DBG_(" new value %lu\n", (unsigned long)(2 * 60 * 60));
              }
              addr->isinfinite = 0;
            } else {
              addr->isinfinite = 1;
            }
          } else {
            if(uip_ntohl(nd6_opt_prefix_info->validlt) ==
               UIP_ND6_INFINITE_LIFETIME) {
              uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
            } else {
              uip_ds6_addr_add(&ipaddr, uip_ntohl(nd6_opt_prefix_info->validlt),
                               ADDR_AUTOCONF);
            }
          }
        }
        /* End of autonomous flag related processing */
      }
      break;
#if UIP_ND6_RA_RDNSS
    case UIP_ND6_OPT_RDNSS:
      LOG_DBG("Processing RDNSS option\n");
      uint8_t naddr = (ND6_OPT_RDNSS_BUF(nd6_opt_offset)->len - 1) / 2;
      uip_ipaddr_t *ip = (uip_ipaddr_t *)(&ND6_OPT_RDNSS_BUF(nd6_opt_offset)->ip);
      LOG_DBG("got %d nameservers\n", naddr);
      while(naddr-- > 0) {
        LOG_DBG("nameserver: ");
        LOG_DBG_6ADDR(ip);
        LOG_DBG_(" lifetime: %" PRIx32 "\n", uip_ntohl(ND6_OPT_RDNSS_BUF(nd6_opt_offset)->lifetime));
        uip_nameserver_update(ip, uip_ntohl(ND6_OPT_RDNSS_BUF(nd6_opt_offset)->lifetime));
        ip++;
      }
      break;
#endif /* UIP_ND6_RA_RDNSS */
    default:
      LOG_ERR("ND option not supported in RA\n");
      break;
    }
    nd6_opt_offset += (ND6_OPT_HDR_BUF(nd6_opt_offset)->len << 3);
  }

  defrt = uip_ds6_defrt_lookup(&UIP_IP_BUF->srcipaddr);
  if(UIP_ND6_RA_BUF->router_lifetime != 0) {
    if(nbr != NULL) {
      nbr->isrouter = 1;
    }
    if(defrt == NULL) {
      uip_ds6_defrt_add(&UIP_IP_BUF->srcipaddr,
                        (unsigned
                         long)(uip_ntohs(UIP_ND6_RA_BUF->router_lifetime)));
    } else {
      stimer_set(&(defrt->lifetime),
                 (unsigned long)(uip_ntohs(UIP_ND6_RA_BUF->router_lifetime)));
    }
  } else {
    if(defrt != NULL) {
      uip_ds6_defrt_rm(defrt);
    }
  }

#if UIP_CONF_IPV6_QUEUE_PKT
  /* If the nbr just became reachable (e.g. it was in NBR_INCOMPLETE state
   * and we got a SLLAO), check if we had buffered a pkt for it */
  /*  if((nbr != NULL) && (nbr->queue_buf_len != 0)) {
     uip_len = nbr->queue_buf_len;
     memcpy(UIP_IP_BUF, nbr->queue_buf, uip_len);
     nbr->queue_buf_len = 0;
     return;
     }*/
  if(nbr != NULL && uip_packetqueue_buflen(&nbr->packethandle) != 0) {
    uip_len = uip_packetqueue_buflen(&nbr->packethandle);
    memcpy(UIP_IP_BUF, uip_packetqueue_buf(&nbr->packethandle), uip_len);
    uip_packetqueue_free(&nbr->packethandle);
    return;
  }

#endif /*UIP_CONF_IPV6_QUEUE_PKT */

discard:
  uipbuf_clear();
  return;
}
#endif /* !UIP_CONF_ROUTER */
/*------------------------------------------------------------------*/
/* ICMPv6 input handlers */
#if UIP_ND6_SEND_NA
UIP_ICMP6_HANDLER(ns_input_handler, ICMP6_NS, UIP_ICMP6_HANDLER_CODE_ANY,
                  ns_input);
#endif
#if UIP_ND6_SEND_NS
UIP_ICMP6_HANDLER(na_input_handler, ICMP6_NA, UIP_ICMP6_HANDLER_CODE_ANY,
                  na_input);
#endif

#if UIP_CONF_ROUTER && UIP_ND6_SEND_RA
UIP_ICMP6_HANDLER(rs_input_handler, ICMP6_RS, UIP_ICMP6_HANDLER_CODE_ANY,
                  rs_input);
#endif

#if !UIP_CONF_ROUTER
UIP_ICMP6_HANDLER(ra_input_handler, ICMP6_RA, UIP_ICMP6_HANDLER_CODE_ANY,
                  ra_input);
#endif
/*---------------------------------------------------------------------------*/
void
uip_nd6_init()
{
#if UIP_ND6_SEND_NA
  /* Only handle NSs if we are prepared to send out NAs */
  uip_icmp6_register_input_handler(&ns_input_handler);
#endif

#if UIP_ND6_SEND_NS
  /*
   * Only handle NAs if we are prepared to send out NSs. */
  uip_icmp6_register_input_handler(&na_input_handler);
#endif

#if UIP_CONF_ROUTER && UIP_ND6_SEND_RA
  /* Only accept RS if we are a router and happy to send out RAs */
  uip_icmp6_register_input_handler(&rs_input_handler);
#endif

#if !UIP_CONF_ROUTER
  /* Only process RAs if we are not a router */
  uip_icmp6_register_input_handler(&ra_input_handler);
#endif
}
/*---------------------------------------------------------------------------*/
/** @} */
