/*
 * Copyright (c) 2002-2003, Adam Dunkels.
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
 *         DNS host name to IP address resolver.
 * \author Adam Dunkels <adam@dunkels.com>
 * \author Robert Quattlebaum <darco@deepdarc.com>
 *
 *         This file implements a DNS host name to IP address resolver,
 *         as well as an MDNS responder and resolver.
 */

/**
 * \addtogroup uip
 * @{
 */

/**
 * \defgroup uipdns uIP hostname resolver functions
 * @{
 *
 * The uIP DNS resolver functions are used to lookup a hostname and
 * map it to a numerical IP address. It maintains a list of resolved
 * hostnames that can be queried with the resolv_lookup()
 * function. New hostnames can be resolved using the resolv_query()
 * function.
 *
 * The event resolv_event_found is posted when a hostname has been
 * resolved. It is up to the receiving process to determine if the
 * correct hostname has been found by calling the resolv_lookup()
 * function with the hostname.
 */

#include "net/ipv6/tcpip.h"
#include "net/ipv6/uip-udp-packet.h"
#include "net/ipv6/uip-nameserver.h"
#include "lib/random.h"
#include "resolv.h"
#include <inttypes.h>
#include <stdbool.h>

/** If RESOLV_CONF_SUPPORTS_MDNS is set, then queries
 *  for domain names in the `local` TLD will use MDNS and
 *  will respond to MDNS queries for this device's hostname,
 *  as described by draft-cheshire-dnsext-multicastdns.
 */
#ifndef RESOLV_CONF_SUPPORTS_MDNS
#define RESOLV_SUPPORTS_MDNS 0
#else
#define RESOLV_SUPPORTS_MDNS RESOLV_CONF_SUPPORTS_MDNS
#endif

#if UIP_UDP

#include <string.h>

#if RESOLV_SUPPORTS_MDNS
#include <ctype.h>
#endif /* RESOLV_SUPPORTS_MDNS */

#include "sys/log.h"

#define LOG_MODULE "Resolv"
#define LOG_LEVEL LOG_LEVEL_NONE

int strcasecmp(const char *s1, const char *s2);

int strncasecmp(const char *s1, const char *s2, size_t n);

#ifndef RESOLV_CONF_MDNS_INCLUDE_GLOBAL_V6_ADDRS
#define RESOLV_CONF_MDNS_INCLUDE_GLOBAL_V6_ADDRS 0
#endif

/** The maximum number of retries when asking for a name. */
#ifndef RESOLV_CONF_MAX_RETRIES
#define RESOLV_CONF_MAX_RETRIES 4
#endif

#ifndef RESOLV_CONF_MAX_MDNS_RETRIES
#define RESOLV_CONF_MAX_MDNS_RETRIES 3
#endif

#ifndef RESOLV_CONF_MAX_DOMAIN_NAME_SIZE
#define RESOLV_CONF_MAX_DOMAIN_NAME_SIZE 32
#endif

#ifdef RESOLV_CONF_AUTO_REMOVE_TRAILING_DOTS
#define RESOLV_AUTO_REMOVE_TRAILING_DOTS RESOLV_CONF_AUTO_REMOVE_TRAILING_DOTS
#else
#define RESOLV_AUTO_REMOVE_TRAILING_DOTS RESOLV_SUPPORTS_MDNS
#endif

#ifdef RESOLV_CONF_VERIFY_ANSWER_NAMES
#define RESOLV_VERIFY_ANSWER_NAMES RESOLV_CONF_VERIFY_ANSWER_NAMES
#else
#define RESOLV_VERIFY_ANSWER_NAMES RESOLV_SUPPORTS_MDNS
#endif

#ifdef RESOLV_CONF_SUPPORTS_RECORD_EXPIRATION
#define RESOLV_SUPPORTS_RECORD_EXPIRATION RESOLV_CONF_SUPPORTS_RECORD_EXPIRATION
#else
#define RESOLV_SUPPORTS_RECORD_EXPIRATION 1
#endif

#if RESOLV_SUPPORTS_MDNS && !RESOLV_VERIFY_ANSWER_NAMES
#error RESOLV_SUPPORTS_MDNS cannot be set without RESOLV_CONF_VERIFY_ANSWER_NAMES
#endif

#if !defined(CONTIKI_TARGET_NAME) && defined(BOARD)
#define stringy2(x) #x
#define stringy(x)  stringy2(x)
#define CONTIKI_TARGET_NAME stringy(BOARD)
#endif

#ifndef CONTIKI_CONF_DEFAULT_HOSTNAME
#ifdef CONTIKI_TARGET_NAME
#define CONTIKI_CONF_DEFAULT_HOSTNAME "contiki-"CONTIKI_TARGET_NAME
#else
#define CONTIKI_CONF_DEFAULT_HOSTNAME "contiki"
#endif
#endif

#define DNS_TYPE_A      1
#define DNS_TYPE_CNAME  5
#define DNS_TYPE_PTR   12
#define DNS_TYPE_MX    15
#define DNS_TYPE_TXT   16
#define DNS_TYPE_AAAA  28
#define DNS_TYPE_SRV   33
#define DNS_TYPE_ANY  255
#define DNS_TYPE_NSEC  47

#define NATIVE_DNS_TYPE DNS_TYPE_AAAA /* IPv6 */

#define DNS_CLASS_IN    1
#define DNS_CLASS_ANY 255

#ifndef DNS_PORT
#define DNS_PORT 53
#endif

#ifndef MDNS_PORT
#define MDNS_PORT 5353
#endif

#ifndef MDNS_RESPONDER_PORT
#define MDNS_RESPONDER_PORT 5354
#endif

/** \internal The DNS message header. */
struct dns_hdr {
    uint16_t id;
    uint8_t flags1, flags2;
#define DNS_FLAG1_RESPONSE        0x80
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03
    uint16_t numquestions;
    uint16_t numanswers;
    uint16_t numauthrr;
    uint16_t numextrarr;
};

/** \internal The DNS answer message structure. */
struct dns_answer {
    /* DNS answer record starts with either a domain name or a pointer
     * to a name already present somewhere in the packet. */
    uint16_t type;
    uint16_t class;
    uint16_t ttl[2];
    uint16_t len;
    uint8_t ipaddr[16];
};

struct namemap {
#define STATE_UNUSED 0
#define STATE_ERROR  1
#define STATE_NEW    2
#define STATE_ASKING 3
#define STATE_DONE   4
    uint8_t state;
    uint8_t tmr;
    uint16_t id;
    uint8_t retries;
    uint8_t seqno;
#if RESOLV_SUPPORTS_RECORD_EXPIRATION
    unsigned long expiration;
#endif /* RESOLV_SUPPORTS_RECORD_EXPIRATION */
    uip_ipaddr_t ipaddr;
    uint8_t err;
    uint8_t server;
#if RESOLV_SUPPORTS_MDNS
    bool is_mdns;
    bool is_probe;
#endif
    char name[RESOLV_CONF_MAX_DOMAIN_NAME_SIZE + 1];
};

#ifndef UIP_CONF_RESOLV_ENTRIES
#define RESOLV_ENTRIES 4
#else /* UIP_CONF_RESOLV_ENTRIES */
#define RESOLV_ENTRIES UIP_CONF_RESOLV_ENTRIES
#endif /* UIP_CONF_RESOLV_ENTRIES */

static struct namemap names[RESOLV_ENTRIES];
static uint8_t seqno;
static struct uip_udp_conn *resolv_conn = NULL;
static struct etimer retry;
process_event_t resolv_event_found;

PROCESS(resolv_process,
"DNS resolver");

static void resolv_found(char *name, uip_ipaddr_t *ipaddr);

/** \internal The DNS question message structure. */
struct dns_question {
    uint16_t type;
    uint16_t class;
};

#if RESOLV_SUPPORTS_MDNS
static char resolv_hostname[RESOLV_CONF_MAX_DOMAIN_NAME_SIZE + 1];

enum {
  MDNS_STATE_WAIT_BEFORE_PROBE,
  MDNS_STATE_PROBING,
  MDNS_STATE_READY,
};

static uint8_t mdns_state;

static const uip_ipaddr_t resolv_mdns_addr =
{ { 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb } };
#include "net/ipv6/uip-ds6.h"

static int mdns_needs_host_announce;

PROCESS(mdns_probe_process, "mDNS probe");
#endif /* RESOLV_SUPPORTS_MDNS */

/*---------------------------------------------------------------------------*/
/** \internal
 * \brief Decodes a DNS name from the DNS format into the given string.
 * \return 1 upon success, 0 if the size of the name would be too large.
 *
 * \note `dest` must point to a buffer with at least
 *       `RESOLV_CONF_MAX_DOMAIN_NAME_SIZE+1` bytes large.
 */
static uint8_t
decode_name(const unsigned char *query, char *dest,
            const unsigned char *packet, size_t packet_len) {
    int dest_len = RESOLV_CONF_MAX_DOMAIN_NAME_SIZE;
    unsigned char label_len = *query++;

    LOG_DBG("decoding name: \"");

    while (dest_len && label_len) {
        if (label_len & 0xc0) {
            const uint16_t offset = query[0] + ((label_len & ~0xC0) << 8);
            if (offset >= packet_len) {
                LOG_ERR("Offset %"PRIu16" exceeds packet length %zu\n",
                        offset, packet_len);
                return 0;
            }
            LOG_DBG_("<skip-to-%d>", offset);
            query = packet + offset;
            label_len = *query++;
        }

        if (label_len == 0) {
            break;
        }

        if (query - packet + label_len > packet_len) {
            LOG_ERR("Cannot read outside the packet data\n");
            return 0;
        }

        for (; label_len; --label_len) {
            LOG_DBG_("%c", *query);

            *dest++ = *query++;

            if (--dest_len == 0) {
                *dest = 0;
                LOG_DBG_("\"\n");
                return 0;
            }
        }

        label_len = *query++;

        if (label_len > 0) {
            LOG_DBG_(".");
            *dest++ = '.';
            --dest_len;
        }
    }

    LOG_DBG_("\"\n");
    *dest = 0;
    return dest_len != 0;
}
/*---------------------------------------------------------------------------*/
/** \internal
 */
#if RESOLV_SUPPORTS_MDNS
static uint8_t
dns_name_isequal(const unsigned char *queryptr, const char *name,
                 const unsigned char *packet)
{
  unsigned char label_len = *queryptr++;

  if(*name == 0) {
    return 0;
  }

  while(label_len > 0) {
    if(label_len & 0xc0) {
      queryptr = packet + queryptr[0] + ((label_len & ~0xC0) << 8);
      label_len = *queryptr++;
    }

    for(; label_len; --label_len) {
      if(!*name) {
        return 0;
      }

      if(tolower(*name++) != tolower(*queryptr++)) {
        return 0;
      }
    }

    label_len = *queryptr++;

    if(label_len != 0 && *name++ != '.') {
      return 0;
    }
  }

  if(*name == '.') {
    ++name;
  }

  return name[0] == 0;
}
#endif /* RESOLV_SUPPORTS_MDNS */
/*---------------------------------------------------------------------------*/
/** \internal
 */
static unsigned char *
skip_name(unsigned char *query) {
    LOG_DBG("skip name: ");

    do {
        unsigned char n = *query;
        if (n & 0xc0) {
            LOG_DBG_("<skip-to-%d>", query[0] + ((n & ~0xC0) << 8));
            ++query;
            break;
        }

        ++query;

        while (n > 0) {
            LOG_DBG_("%c", *query);
            ++query;
            --n;
        }
        LOG_DBG_(".");
    } while (*query != 0);
    LOG_DBG_("\n");
    return query + 1;
}
/*---------------------------------------------------------------------------*/
/** \internal
 */
static unsigned char *
encode_name(unsigned char *query, const char *nameptr) {
    --nameptr;
    /* Convert hostname into suitable query format. */
    do {
        uint8_t n = 0;
        char *nptr = (char *) query;

        ++nameptr;
        ++query;
        for (n = 0; *nameptr != '.' && *nameptr != 0; ++nameptr) {
            *query = *nameptr;
            ++query;
            ++n;
        }
        *nptr = n;
    } while (*nameptr != 0);

    /* End the the name. */
    *query++ = 0;

    return query;
}
/*---------------------------------------------------------------------------*/
#if RESOLV_SUPPORTS_MDNS
/** \internal
 */
static void
mdns_announce_requested(void)
{
  mdns_needs_host_announce = 1;
}
/*---------------------------------------------------------------------------*/
/** \internal
 */
static void
start_name_collision_check(clock_time_t after)
{
  process_exit(&mdns_probe_process);
  process_start(&mdns_probe_process, (void *)&after);
}
/*---------------------------------------------------------------------------*/
/** \internal
 */
static unsigned char *
mdns_write_announce_records(unsigned char *queryptr, uint8_t *count)
{
  uint8_t i;

  for(i = 0; i < UIP_DS6_ADDR_NB; ++i) {
    if(uip_ds6_if.addr_list[i].isused
#if !RESOLV_CONF_MDNS_INCLUDE_GLOBAL_V6_ADDRS
       && uip_is_addr_linklocal(&uip_ds6_if.addr_list[i].ipaddr)
#endif
       ) {
      if(!*count) {
        queryptr = encode_name(queryptr, resolv_hostname);
      } else {
        /* Use name compression to refer back to the first name */
        *queryptr++ = 0xc0;
        *queryptr++ = sizeof(struct dns_hdr);
      }

      *queryptr++ = (uint8_t)(NATIVE_DNS_TYPE >> 8);
      *queryptr++ = (uint8_t)NATIVE_DNS_TYPE;

      *queryptr++ = (uint8_t)((DNS_CLASS_IN | 0x8000) >> 8);
      *queryptr++ = (uint8_t)(DNS_CLASS_IN | 0x8000);

      *queryptr++ = 0;
      *queryptr++ = 0;
      *queryptr++ = 0;
      *queryptr++ = 120;

      *queryptr++ = 0;
      *queryptr++ = sizeof(uip_ipaddr_t);

      uip_ipaddr_copy((uip_ipaddr_t *)queryptr,
                      &uip_ds6_if.addr_list[i].ipaddr);
      queryptr += sizeof(uip_ipaddr_t);
      ++(*count);
    }
  }
  return queryptr;
}
/*---------------------------------------------------------------------------*/
/** \internal
 * Called when we need to announce ourselves
 */
static size_t
mdns_prep_host_announce_packet(void)
{
  static const struct {
    uint16_t type;
    uint16_t class;
    uint16_t ttl[2];
    uint16_t len;
    uint8_t data[8];
  } nsec_record = {
    UIP_HTONS(DNS_TYPE_NSEC),
    UIP_HTONS(DNS_CLASS_IN | 0x8000),
    { 0, UIP_HTONS(120) },
    UIP_HTONS(8),

    {
      0xc0,
      sizeof(struct dns_hdr), /* Name compression. Re-use name of 1st record. */
      0x00,
      0x04,

      0x00,
      0x00,
      0x00,
      0x08,
    }
  };

  /* Be aware that, unless `ARCH_DOESNT_NEED_ALIGNED_STRUCTS` is set,
   * writing directly to the uint16_t members of this struct is an error. */
  struct dns_hdr *hdr = (struct dns_hdr *)uip_appdata;
  unsigned char *queryptr = (unsigned char *)uip_appdata + sizeof(*hdr);
  uint8_t total_answers = 0;

  /* Zero out the header */
  memset((void *)hdr, 0, sizeof(*hdr));
  hdr->flags1 |= DNS_FLAG1_RESPONSE | DNS_FLAG1_AUTHORATIVE;

  queryptr = mdns_write_announce_records(queryptr, &total_answers);

  /* We now need to add an NSEC record to indicate
   * that this is all there is.
   */
  if(!total_answers) {
    queryptr = encode_name(queryptr, resolv_hostname);
  } else {
    /* Name compression. Re-using the name of first record. */
    *queryptr++ = 0xc0;
    *queryptr++ = sizeof(*hdr);
  }

  memcpy((void *)queryptr, (void *)&nsec_record, sizeof(nsec_record));

  queryptr += sizeof(nsec_record);

  /* This platform might be picky about alignment. To avoid the possibility
   * of doing an unaligned write, we are going to do this manually. */
  ((uint8_t *)&hdr->numanswers)[1] = total_answers;
  ((uint8_t *)&hdr->numextrarr)[1] = 1;

  return queryptr - (unsigned char *)uip_appdata;
}
#endif /* RESOLV_SUPPORTS_MDNS */

/*---------------------------------------------------------------------------*/
static char
try_next_server(struct namemap *namemapptr) {
    namemapptr->server++;
    if (uip_nameserver_get(namemapptr->server) != NULL) {
        LOG_DBG("Using server ");
        LOG_DBG_6ADDR(uip_nameserver_get(namemapptr->server));
        LOG_DBG_(", num %u\n", namemapptr->server);
        namemapptr->retries = 0;
        return 1;
    }
    LOG_DBG("No nameserver, num %u\n", namemapptr->server);
    namemapptr->server = 0;
    return 0;
}
/*---------------------------------------------------------------------------*/
/** \internal
 * Runs through the list of names to see if there are any that have
 * not yet been queried and, if so, sends out a query.
 */
static void
check_entries(void) {
    uint8_t i;

    for (i = 0; i < RESOLV_ENTRIES; ++i) {
        struct namemap *namemapptr = &names[i];
        if (namemapptr->state == STATE_NEW || namemapptr->state == STATE_ASKING) {
            etimer_set(&retry, CLOCK_SECOND / 4);
            if (namemapptr->state == STATE_ASKING) {
                if (namemapptr->tmr == 0 || --namemapptr->tmr == 0) {
#if RESOLV_SUPPORTS_MDNS
                    if(++namemapptr->retries ==
                       (namemapptr->is_mdns ? RESOLV_CONF_MAX_MDNS_RETRIES :
                        RESOLV_CONF_MAX_RETRIES))
#else /* RESOLV_SUPPORTS_MDNS */
                    if (++namemapptr->retries == RESOLV_CONF_MAX_RETRIES)
#endif /* RESOLV_SUPPORTS_MDNS */
                    {
                        /* Try the next server (if possible) before failing. Otherwise
                           simply mark the entry as failed. */
                        if (try_next_server(namemapptr) == 0) {
                            /* STATE_ERROR basically means "not found". */
                            namemapptr->state = STATE_ERROR;

#if RESOLV_SUPPORTS_RECORD_EXPIRATION
                            /* Keep the "not found" error valid for 30 seconds */
                            namemapptr->expiration = clock_seconds() + 30;
#endif /* RESOLV_SUPPORTS_RECORD_EXPIRATION */

                            resolv_found(namemapptr->name, NULL);
                            continue;
                        }
                    }
                    namemapptr->tmr = namemapptr->retries * namemapptr->retries * 3;

#if RESOLV_SUPPORTS_MDNS
                    if(namemapptr->is_probe) {
                      /* Probing retries are much more aggressive, 250ms */
                      namemapptr->tmr = 2;
                    }
#endif /* RESOLV_SUPPORTS_MDNS */
                } else {
                    /* Its timer has not run out, so we move on to next entry. */
                    continue;
                }
            } else {
                namemapptr->state = STATE_ASKING;
                namemapptr->tmr = 1;
                namemapptr->retries = 0;
            }

            struct dns_hdr *hdr = (struct dns_hdr *) uip_appdata;
            memset(hdr, 0, sizeof(struct dns_hdr));
            hdr->id = random_rand();
            namemapptr->id = hdr->id;

#if RESOLV_SUPPORTS_MDNS
            if(!namemapptr->is_mdns || namemapptr->is_probe) {
              hdr->flags1 = DNS_FLAG1_RD;
            }
            if(namemapptr->is_mdns) {
              hdr->id = 0;
            }
#else /* RESOLV_SUPPORTS_MDNS */
            hdr->flags1 = DNS_FLAG1_RD;
#endif /* RESOLV_SUPPORTS_MDNS */

            hdr->numquestions = UIP_HTONS(1);
            uint8_t *query = (unsigned char *) uip_appdata + sizeof(*hdr);
            query = encode_name(query, namemapptr->name);

#if RESOLV_SUPPORTS_MDNS
            if(namemapptr->is_probe) {
              *query++ = (uint8_t)((DNS_TYPE_ANY) >> 8);
              *query++ = (uint8_t)((DNS_TYPE_ANY));
            } else
#endif /* RESOLV_SUPPORTS_MDNS */
            {
                *query++ = (uint8_t)(NATIVE_DNS_TYPE >> 8);
                *query++ = (uint8_t) NATIVE_DNS_TYPE;
            }
            *query++ = (uint8_t)(DNS_CLASS_IN >> 8);
            *query++ = (uint8_t) DNS_CLASS_IN;
#if RESOLV_SUPPORTS_MDNS
            if(namemapptr->is_mdns) {
              if(namemapptr->is_probe) {
                /* This is our conflict detection request.
                 * In order to be in compliance with the MDNS
                 * spec, we need to add the records we are proposing
                 * to the rrauth section.
                 */
                uint8_t count = 0;

                query = mdns_write_announce_records(query, &count);
                hdr->numauthrr = UIP_HTONS(count);
              }
              uip_udp_packet_sendto(resolv_conn, uip_appdata,
                                    (query - (uint8_t *)uip_appdata),
                                    &resolv_mdns_addr, UIP_HTONS(MDNS_PORT));

              LOG_DBG("(i=%d) Sent MDNS %s for \"%s\"\n", i,
                      namemapptr->is_probe ? "probe" : "request", namemapptr->name);
            } else {
              uip_udp_packet_sendto(resolv_conn, uip_appdata,
                                    (query - (uint8_t *)uip_appdata),
                                    (const uip_ipaddr_t *)
                                    uip_nameserver_get(namemapptr->server),
                                    UIP_HTONS(DNS_PORT));

              LOG_DBG("(i=%d) Sent DNS request for \"%s\"\n", i,
                      namemapptr->name);
            }
#else /* RESOLV_SUPPORTS_MDNS */
            uip_udp_packet_sendto(resolv_conn, uip_appdata,
                                  (query - (uint8_t *) uip_appdata),
                                  uip_nameserver_get(namemapptr->server),
                                  UIP_HTONS(DNS_PORT));
            LOG_DBG("(i=%d) Sent DNS request for \"%s\"\n", i,
                    namemapptr->name);
#endif /* RESOLV_SUPPORTS_MDNS */
            break;
        }
    }
}
/*---------------------------------------------------------------------------*/
/** \internal
 * Called when new UDP data arrives.
 */
static void
newdata(void) {
    int8_t i = 0;
    struct dns_hdr const *hdr = (struct dns_hdr *) uip_appdata;
    unsigned char *queryptr = (unsigned char *) hdr + sizeof(*hdr);
    const uint8_t is_request = (hdr->flags1 & ~1) == 0 && hdr->flags2 == 0;

    /* We only care about the question(s) and the answers. The authrr
     * and the extrarr are simply discarded.
     */
    uint8_t nquestions = (uint8_t) uip_ntohs(hdr->numquestions);
    uint8_t nanswers = (uint8_t) uip_ntohs(hdr->numanswers);

    queryptr = (unsigned char *) hdr + sizeof(*hdr);
    i = 0;

    LOG_DBG("flags1=0x%02X flags2=0x%02X nquestions=%d, nanswers=%d, " \
          "nauthrr=%d, nextrarr=%d\n",
            hdr->flags1, hdr->flags2, (uint8_t) nquestions, (uint8_t) nanswers,
            (uint8_t) uip_ntohs(hdr->numauthrr),
            (uint8_t) uip_ntohs(hdr->numextrarr));

    if (is_request && nquestions == 0) {
        /* Skip requests with no questions. */
        LOG_DBG("Skipping request with no questions\n");
        return;
    }

/** QUESTION HANDLING SECTION ************************************************/

    for (; nquestions > 0;
           queryptr = skip_name(queryptr) + sizeof(struct dns_question),
                   --nquestions
            ) {
#if RESOLV_SUPPORTS_MDNS
        if(!is_request) {
          /* If this isn't a request, we don't need to bother
           * looking at the individual questions. For the most
           * part, this loop to just used to skip past them.
           */
          continue;
        }

        {
          struct dns_question *question =
            (struct dns_question *)skip_name(queryptr);

#if !ARCH_DOESNT_NEED_ALIGNED_STRUCTS
          static struct dns_question aligned;
          memcpy(&aligned, question, sizeof(aligned));
          question = &aligned;
#endif /* !ARCH_DOESNT_NEED_ALIGNED_STRUCTS */

          LOG_DBG("Question %d: type=%d class=%d\n", ++i,
                  uip_htons(question->type), uip_htons(question->class));

          if((uip_ntohs(question->class) & 0x7FFF) != DNS_CLASS_IN ||
             (question->type != UIP_HTONS(DNS_TYPE_ANY) &&
              question->type != UIP_HTONS(NATIVE_DNS_TYPE))) {
            /* Skip unrecognised records. */
            continue;
          }

          if(!dns_name_isequal(queryptr, resolv_hostname, uip_appdata)) {
        continue;
          }

          LOG_DBG("Received MDNS request for us\n");

          if(mdns_state == MDNS_STATE_READY) {
            /* We only send immediately if this isn't an MDNS request.
             * Otherwise, we schedule ourselves to send later.
             */
            if(UIP_UDP_BUF->srcport == UIP_HTONS(MDNS_PORT)) {
              mdns_announce_requested();
            } else {
              uip_udp_packet_sendto(resolv_conn, uip_appdata,
                                    mdns_prep_host_announce_packet(),
                                    &UIP_IP_BUF->srcipaddr,
                                    UIP_UDP_BUF->srcport);
            }
            return;
          } else {
            uint8_t nauthrr;

            LOG_DBG("But we are still probing. Waiting...\n");

            /* We are still probing. We need to do the mDNS
             * probe race condition check here and make sure
             * we don't need to delay probing for a second.
             */
            nauthrr = (uint8_t)uip_ntohs(hdr->numauthrr);

            /* For now, we will always restart the collision check if
             * there are *any* authority records present.
             * In the future we should follow the spec more closely,
             * but this should eventually converge to something reasonable.
             */
            if(nauthrr) {
              start_name_collision_check(CLOCK_SECOND);
            }
          }
        }
#endif /* RESOLV_SUPPORTS_MDNS */
    }

/** ANSWER HANDLING SECTION **************************************************/
    struct namemap *namemapptr = NULL;

#if RESOLV_SUPPORTS_MDNS
    if(UIP_UDP_BUF->srcport == UIP_HTONS(MDNS_PORT) && hdr->id == 0) {
      /* OK, this was from MDNS. Things get a little weird here,
       * because we can't use the `id` field. We will look up the
       * appropriate request in a later step. */

      i = -1;
      namemapptr = NULL;
    } else
#endif /* RESOLV_SUPPORTS_MDNS */
    {
        for (i = 0; i < RESOLV_ENTRIES; ++i) {
            namemapptr = &names[i];
            if (namemapptr->state == STATE_ASKING &&
                namemapptr->id == hdr->id) {
                break;
            }
        }

        if (i >= RESOLV_ENTRIES || i < 0 || namemapptr->state != STATE_ASKING) {
            LOG_DBG("DNS response has bad ID (%04X)\n", uip_ntohs(hdr->id));
            return;
        }

        LOG_DBG("Incoming response for \"%s\"\n", namemapptr->name);

        /* We'll change this to DONE when we find the record. */
        namemapptr->state = STATE_ERROR;
        namemapptr->err = hdr->flags2 & DNS_FLAG2_ERR_MASK;

#if RESOLV_SUPPORTS_RECORD_EXPIRATION
        /* If we remain in the error state, keep it cached for 30 seconds. */
        namemapptr->expiration = clock_seconds() + 30;
#endif /* RESOLV_SUPPORTS_RECORD_EXPIRATION */

        /* Check for error. If so, call callback to inform. */
        if (namemapptr->err != 0) {
            namemapptr->state = STATE_ERROR;
            resolv_found(namemapptr->name, NULL);
            return;
        }
    }

    i = 0;

    /* Answer parsing loop */
    while (nanswers > 0) {
        struct dns_answer *ans = (struct dns_answer *) skip_name(queryptr);

#if !ARCH_DOESNT_NEED_ALIGNED_STRUCTS
        {
            static struct dns_answer aligned;
            memcpy(&aligned, ans, sizeof(aligned));
            ans = &aligned;
        }
#endif /* !ARCH_DOESNT_NEED_ALIGNED_STRUCTS */

        if (LOG_DBG_ENABLED) {
            char debug_name[RESOLV_CONF_MAX_DOMAIN_NAME_SIZE + 1];
            decode_name(queryptr, debug_name, uip_appdata, uip_datalen());
            LOG_DBG("Answer %d: \"%s\", type %d, class %d, ttl %"PRIu32", length %d\n",
                    ++i, debug_name, uip_ntohs(ans->type),
                    uip_ntohs(ans->class) & 0x7FFF,
                    (uint32_t)((uint32_t) uip_ntohs(ans->ttl[0]) << 16) |
                    (uint32_t) uip_ntohs(ans->ttl[1]), uip_ntohs(ans->len));
        }

        /* Check the class and length of the answer to make sure
         * it matches what we are expecting
         */
        if ((uip_ntohs(ans->class) & 0x7FFF) != DNS_CLASS_IN ||
            ans->len != UIP_HTONS(sizeof(uip_ipaddr_t))) {
            goto skip_to_next_answer;
        }

        if (ans->type != UIP_HTONS(NATIVE_DNS_TYPE)) {
            goto skip_to_next_answer;
        }

#if RESOLV_SUPPORTS_MDNS
        if(UIP_UDP_BUF->srcport == UIP_HTONS(MDNS_PORT) && hdr->id == 0) {
          int8_t available_i = RESOLV_ENTRIES;

          LOG_DBG("MDNS query\n");

          /* For MDNS, we need to actually look up the name we
           * are looking for.
           */
          for(i = 0; i < RESOLV_ENTRIES; ++i) {
            namemapptr = &names[i];
            if(dns_name_isequal(queryptr, namemapptr->name, uip_appdata)) {
              break;
            }
            if((namemapptr->state == STATE_UNUSED)
#if RESOLV_SUPPORTS_RECORD_EXPIRATION
               || (namemapptr->state == STATE_DONE &&
                   clock_seconds() > namemapptr->expiration)
#endif /* RESOLV_SUPPORTS_RECORD_EXPIRATION */
               ) {
              available_i = i;
            }
          }
          if(i == RESOLV_ENTRIES) {
            LOG_DBG("Unsolicited MDNS response\n");
            i = available_i;
            namemapptr = &names[i];
            if(!decode_name(queryptr, namemapptr->name,
                uip_appdata, uip_datalen())) {
              LOG_DBG("MDNS name too big to cache\n");
              namemapptr = NULL;
              goto skip_to_next_answer;
            }
          }
          if(i == RESOLV_ENTRIES) {
            LOG_DBG("Not enough room to keep track of unsolicited MDNS answer\n");

            if(dns_name_isequal(queryptr, resolv_hostname, uip_appdata)) {
              /* Oh snap, they say they are us! We had better report them... */
              resolv_found(resolv_hostname, (uip_ipaddr_t *)ans->ipaddr);
            }
            namemapptr = NULL;
            goto skip_to_next_answer;
          }
          namemapptr = &names[i];
        } else
#endif /* RESOLV_SUPPORTS_MDNS */
        {
            /* This will force us to stop even if there are more answers. */
            nanswers = 1;
        }

/*  This is disabled for now, so that we don't fail on CNAME records.
 #if RESOLV_VERIFY_ANSWER_NAMES
    if(namemapptr && !dns_name_isequal(queryptr, namemapptr->name, uip_appdata)) {
      LOG_DBG("Answer name doesn't match the query!\n");
      goto skip_to_next_answer;
    }
 #endif
 */

        LOG_DBG("Answer for \"%s\" is usable\n", namemapptr->name);

        namemapptr->state = STATE_DONE;
#if RESOLV_SUPPORTS_RECORD_EXPIRATION
        namemapptr->expiration = (uint32_t) uip_ntohs(ans->ttl[0]) << 16 |
                                 (uint32_t) uip_ntohs(ans->ttl[1]);
        LOG_DBG("Expires in %lu seconds\n", namemapptr->expiration);

        namemapptr->expiration += clock_seconds();
#endif /* RESOLV_SUPPORTS_RECORD_EXPIRATION */

        uip_ipaddr_copy(&namemapptr->ipaddr, (uip_ipaddr_t *) ans->ipaddr);

        resolv_found(namemapptr->name, &namemapptr->ipaddr);
        break;

        skip_to_next_answer:
        queryptr = (unsigned char *) skip_name(queryptr) + 10 + uip_htons(ans->len);
        --nanswers;
    }

    /* Got to this point there's no answer, try next nameserver if available
       since this one doesn't know the answer */
#if RESOLV_SUPPORTS_MDNS
    if(nanswers == 0 && UIP_UDP_BUF->srcport != UIP_HTONS(MDNS_PORT)
       && hdr->id != 0)
#else
    if (nanswers == 0)
#endif
    {
        if (try_next_server(namemapptr)) {
            namemapptr->state = STATE_ASKING;
            process_post(&resolv_process, PROCESS_EVENT_TIMER, NULL);
        }
    }
}
/*---------------------------------------------------------------------------*/
#if RESOLV_SUPPORTS_MDNS
/**
 * \brief           Changes the local hostname advertised by MDNS.
 * \param hostname  The new hostname to advertise.
 */
void
resolv_set_hostname(const char *hostname)
{
  strncpy(resolv_hostname, hostname, RESOLV_CONF_MAX_DOMAIN_NAME_SIZE);

  /* Add the .local suffix if it isn't already there */
  if(strlen(resolv_hostname) < 7 ||
     strcasecmp(resolv_hostname + strlen(resolv_hostname) - 6, ".local") != 0) {
    strncat(resolv_hostname, ".local",
            RESOLV_CONF_MAX_DOMAIN_NAME_SIZE - strlen(resolv_hostname));
  }

  LOG_DBG("hostname changed to \"%s\"\n", resolv_hostname);

  start_name_collision_check(0);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Returns the local hostname being advertised via MDNS.
 * \return     C-string containing the local hostname.
 */
const char *
resolv_get_hostname(void)
{
  return resolv_hostname;
}
/*---------------------------------------------------------------------------*/
/** \internal
 * Process for probing for name conflicts.
 */
PROCESS_THREAD(mdns_probe_process, ev, data)
{
  static struct etimer delay;

  PROCESS_BEGIN();
  mdns_state = MDNS_STATE_WAIT_BEFORE_PROBE;

  LOG_DBG("mdns-probe: Process (re)started\n");

  /* Wait extra time if specified in data */
  if(NULL != data) {
    LOG_DBG("mdns-probe: Probing will begin in %ld clocks\n",
            (long)*(clock_time_t *)data);
    etimer_set(&delay, *(clock_time_t *)data);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
  }

  /* We need to wait a random (0-250ms) period of time before
   * probing to be in compliance with the MDNS spec. */
  etimer_set(&delay, CLOCK_SECOND * (random_rand() & 0xFF) / 1024);
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

  /* Begin searching for our name. */
  mdns_state = MDNS_STATE_PROBING;
  resolv_query(resolv_hostname);

  do {
    PROCESS_WAIT_EVENT_UNTIL(ev == resolv_event_found);
  } while(strcasecmp(resolv_hostname, data) != 0);

  mdns_state = MDNS_STATE_READY;
  mdns_announce_requested();

  LOG_DBG("mdns-probe: Finished probing\n");

  PROCESS_END();
}
#endif /* RESOLV_SUPPORTS_MDNS */
/*---------------------------------------------------------------------------*/
/** \internal
 * The main UDP function.
 */
PROCESS_THREAD(resolv_process, ev, data
)
{
PROCESS_BEGIN();

memset(names,
0, sizeof(names));

resolv_event_found = process_alloc_event();

LOG_DBG("Process started\n");

resolv_conn = udp_new(NULL, 0, NULL);
if(resolv_conn == NULL) {
LOG_ERR("No UDP connection available, exiting the process!\n");

PROCESS_EXIT();

}

#if RESOLV_SUPPORTS_MDNS
LOG_DBG("Supports MDNS\n");
uip_udp_bind(resolv_conn, UIP_HTONS(MDNS_PORT));

uip_ds6_maddr_add(&resolv_mdns_addr);

resolv_set_hostname(CONTIKI_CONF_DEFAULT_HOSTNAME);
#endif /* RESOLV_SUPPORTS_MDNS */

while(1) {
PROCESS_WAIT_EVENT();

if(ev == PROCESS_EVENT_TIMER) {
tcpip_poll_udp(resolv_conn);
} else if(ev == tcpip_event && uip_udp_conn == resolv_conn) {
if(uip_newdata()) {
newdata();

}
if(uip_poll()) {
#if RESOLV_SUPPORTS_MDNS
if(mdns_needs_host_announce) {
  size_t len;

  LOG_DBG("Announcing that we are \"%s\"\n",
      resolv_hostname);

  memset(uip_appdata, 0, sizeof(struct dns_hdr));
  len = mdns_prep_host_announce_packet();
  uip_udp_packet_sendto(resolv_conn, uip_appdata,
            len, &resolv_mdns_addr, UIP_HTONS(MDNS_PORT));
  mdns_needs_host_announce = 0;

  /*
   * Poll again in case this fires at the same time that
   * the event timer did.
   */
  tcpip_poll_udp(resolv_conn);
} else
#endif /* RESOLV_SUPPORTS_MDNS */
{
check_entries();

}
}
}

#if RESOLV_SUPPORTS_MDNS
if(mdns_needs_host_announce) {
  tcpip_poll_udp(resolv_conn);
}
#endif /* RESOLV_SUPPORTS_MDNS */
}

PROCESS_END();

}

/*---------------------------------------------------------------------------*/
static void
init(void) {
    static uint8_t initialized = 0;
    if (!initialized) {
        process_start(&resolv_process, NULL);
        initialized = 1;
    }
}
/*---------------------------------------------------------------------------*/
#if RESOLV_AUTO_REMOVE_TRAILING_DOTS
static const char *
remove_trailing_dots(const char *name)
{
  static char dns_name_without_dots[RESOLV_CONF_MAX_DOMAIN_NAME_SIZE + 1];
  size_t len = strlen(name);

  if(len && name[len - 1] == '.') {
    strncpy(dns_name_without_dots, name, RESOLV_CONF_MAX_DOMAIN_NAME_SIZE);
    while(len && (dns_name_without_dots[len - 1] == '.')) {
      dns_name_without_dots[--len] = 0;
    }
    name = dns_name_without_dots;
  }
  return name;
}
#else /* RESOLV_AUTO_REMOVE_TRAILING_DOTS */
#define remove_trailing_dots(x) (x)
#endif /* RESOLV_AUTO_REMOVE_TRAILING_DOTS */
/*---------------------------------------------------------------------------*/
/**
 * Queues a name so that a question for the name will be sent out.
 *
 * \param name The hostname that is to be queried.
 */
void
resolv_query(const char *name) {
    uint8_t lseqi = 0, lseq = 0, i = 0;
    struct namemap *nameptr = 0;

    init();

    /* Remove trailing dots, if present. */
    name = remove_trailing_dots(name);

    for (i = 0; i < RESOLV_ENTRIES; ++i) {
        nameptr = &names[i];
        if (0 == strcasecmp(nameptr->name, name)) {
            break;
        }
        if ((nameptr->state == STATE_UNUSED)
            #if RESOLV_SUPPORTS_RECORD_EXPIRATION
            || (nameptr->state == STATE_DONE && clock_seconds() > nameptr->expiration)
#endif /* RESOLV_SUPPORTS_RECORD_EXPIRATION */
                ) {
            lseqi = i;
            lseq = 255;
        } else if (seqno - nameptr->seqno > lseq) {
            lseq = seqno - nameptr->seqno;
            lseqi = i;
        }
    }

    if (i == RESOLV_ENTRIES) {
        i = lseqi;
        nameptr = &names[i];
    }

    LOG_DBG("Starting query for \"%s\"\n", name);

    memset(nameptr, 0, sizeof(*nameptr));

    strncpy(nameptr->name, name, sizeof(nameptr->name) - 1);
    nameptr->state = STATE_NEW;
    nameptr->seqno = seqno;
    ++seqno;

#if RESOLV_SUPPORTS_MDNS
    {
      size_t name_len = strlen(name);
      const char local_suffix[] = "local";

      if(name_len > (sizeof(local_suffix) - 1) &&
         strcasecmp(name + name_len - (sizeof(local_suffix) - 1),
            local_suffix) == 0) {
        LOG_DBG("Using MDNS to look up \"%s\"\n", name);
        nameptr->is_mdns = true;
      } else {
        nameptr->is_mdns = false;
      }
    }
    nameptr->is_probe = mdns_state == MDNS_STATE_PROBING &&
      strcmp(nameptr->name, resolv_hostname) == 0;
#endif /* RESOLV_SUPPORTS_MDNS */

    /* Force check_entires() to run on our process. */
    process_post(&resolv_process, PROCESS_EVENT_TIMER, 0);
}
/*---------------------------------------------------------------------------*/
/**
 * Look up a hostname in the array of known hostnames.
 *
 * \note This function only looks in the internal array of known
 * hostnames, it does not send out a query for the hostname if none
 * was found. The function resolv_query() can be used to send a query
 * for a hostname.
 *
 */
resolv_status_t
resolv_lookup(const char *name, uip_ipaddr_t **ipaddr) {
    resolv_status_t ret = RESOLV_STATUS_UNCACHED;

    /* Remove trailing dots, if present. */
    name = remove_trailing_dots(name);

#if UIP_CONF_LOOPBACK_INTERFACE
    if(strcmp(name, "localhost") == 0) {
      static uip_ipaddr_t loopback =
      { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 } };
      if(ipaddr) {
        *ipaddr = &loopback;
      }
      ret = RESOLV_STATUS_CACHED;
    }
#endif /* UIP_CONF_LOOPBACK_INTERFACE */

    /* Walk through the list to see if the name is in there. */
    uint8_t i;
    for (i = 0; i < RESOLV_ENTRIES; ++i) {
        struct namemap *nameptr = &names[i];

        if (strcasecmp(name, nameptr->name) == 0) {
            switch (nameptr->state) {
                case STATE_DONE:
                    ret = RESOLV_STATUS_CACHED;
#if RESOLV_SUPPORTS_RECORD_EXPIRATION
                    if (clock_seconds() > nameptr->expiration) {
                        ret = RESOLV_STATUS_EXPIRED;
                    }
#endif /* RESOLV_SUPPORTS_RECORD_EXPIRATION */
                    break;
                case STATE_NEW:
                case STATE_ASKING:
                    ret = RESOLV_STATUS_RESOLVING;
                    break;
                    /* Almost certainly a not-found error from server */
                case STATE_ERROR:
                    ret = RESOLV_STATUS_NOT_FOUND;
#if RESOLV_SUPPORTS_RECORD_EXPIRATION
                    if (clock_seconds() > nameptr->expiration) {
                        ret = RESOLV_STATUS_UNCACHED;
                    }
#endif /* RESOLV_SUPPORTS_RECORD_EXPIRATION */
                    break;
            }

            if (ipaddr) {
                *ipaddr = &nameptr->ipaddr;
            }

            /* Break out of for loop. */
            break;
        }
    }

    if (LOG_DBG_ENABLED) {
        switch (ret) {
            case RESOLV_STATUS_CACHED:
                if (ipaddr) {
                    LOG_DBG("Found \"%s\" in cache => ", name);
                    const uip_ipaddr_t *addr = *ipaddr;
                    LOG_DBG_6ADDR(addr);
                    LOG_DBG_("\n");
                    break;
                }
            default:
                LOG_DBG("\"%s\" is NOT cached\n", name);
                break;
        }
    }

    return ret;
}
/*---------------------------------------------------------------------------*/
/** \internal
 * Callback function which is called when a hostname is found.
 *
 */
static void
resolv_found(char *name, uip_ipaddr_t *ipaddr) {
#if RESOLV_SUPPORTS_MDNS
    if(strncasecmp(resolv_hostname, name, strlen(resolv_hostname)) == 0 &&
       ipaddr && !uip_ds6_is_my_addr(ipaddr)) {
      uint8_t i;

      if(mdns_state == MDNS_STATE_PROBING) {
        /* We found this new name while probing.
         * We must now rename ourselves.
         */
        LOG_DBG("Name collision detected for \"%s\"\n", name);

        /* Remove the ".local" suffix. */
        resolv_hostname[strlen(resolv_hostname) - 6] = 0;

        /* Append the last three hex parts of the link-level address. */
        for(i = 0; i < 3; ++i) {
          uint8_t val = uip_lladdr.addr[(UIP_LLADDR_LEN - 3) + i];

          char append_str[4] = "-XX";

          append_str[2] = (((val & 0xF) > 9) ? 'a' : '0') + (val & 0xF);
          val >>= 4;
          append_str[1] = (((val & 0xF) > 9) ? 'a' : '0') + (val & 0xF);
          /* -1 in order to fit the terminating null byte. */
          strncat(resolv_hostname, append_str,
                  sizeof(resolv_hostname) - strlen(resolv_hostname) - 1);
        }

        /* Re-add the .local suffix */
        strncat(resolv_hostname, ".local",
                RESOLV_CONF_MAX_DOMAIN_NAME_SIZE - strlen(resolv_hostname));

        start_name_collision_check(CLOCK_SECOND * 5);
      } else if(mdns_state == MDNS_STATE_READY) {
        /* We found a collision after we had already asserted
         * that we owned this name. We need to immediately
         * and explicitly begin probing.
         */
        LOG_DBG("Possible name collision, probing...\n");
        start_name_collision_check(0);
      }
    } else
#endif /* RESOLV_SUPPORTS_MDNS */
    if (ipaddr) {
        LOG_DBG("Found address for \"%s\" => ", name);
        LOG_DBG_6ADDR(ipaddr);
        LOG_DBG_("\n");
    } else {
        LOG_DBG("Unable to retrieve address for \"%s\"\n", name);
    }
    process_post(PROCESS_BROADCAST, resolv_event_found, name);
}
/*---------------------------------------------------------------------------*/
#endif /* UIP_UDP */

/** @} */
/** @} */
