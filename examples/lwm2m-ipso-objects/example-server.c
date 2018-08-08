/*
 * Copyright (c) 2015, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * \file
 *      IPSO Objects and OMA LWM2M example.
 * \author
 *      Joakim Eriksson, joakime@sics.se
 *      Niclas Finne, nfi@sics.se
 */

#include "contiki.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/netstack.h"
#include "net/routing/routing.h"
#include "coap-transport.h"
#include "coap-blocking-api.h"
#include "lwm2m-engine.h"
#include "lwm2m-tlv.h"
#include "dev/serial-line.h"
#include "serial-protocol.h"
#include <stdbool.h>

#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

#define REMOTE_PORT                  UIP_HTONS(COAP_DEFAULT_PORT)

#define EVENT_RUN_NOW                0

#define URL_WELL_KNOWN               ".well-known/core"
#define URL_DEVICE_MODEL             "/3/0/1"
#define URL_DEVICE_FIRMWARE_VERSION  "/3/0/3"
#define URL_LIGHT_CONTROL            "/3311/0/5850"
#define URL_POWER_CONTROL            "/3312/0/5850"

#define MAX_NODES 10

#define NODE_HAS_TYPE  (1 << 0)

struct node {
  coap_endpoint_t endpoint;
  char type[64];
  uint8_t flags;
  uint8_t retries;
};

static struct node nodes[MAX_NODES];
static uint8_t node_count;

static struct node *current_target;
static char current_uri[32] = URL_LIGHT_CONTROL;
static char current_value[32] = "1";
static int current_request = COAP_PUT;
static uint8_t fetching_type = 0;

PROCESS(router_process, "router process");
AUTOSTART_PROCESSES(&router_process);
/*---------------------------------------------------------------------------*/
static struct node *
add_node(const uip_ipaddr_t *addr)
{
  int i;
  for(i = 0; i < node_count; i++) {
    if(uip_ipaddr_cmp(&nodes[i].endpoint.ipaddr, addr)) {
      /* Node already added */
      return &nodes[i];
    }
  }
  if(node_count < MAX_NODES) {
    memset(&nodes[node_count].endpoint, 0, sizeof(coap_endpoint_t));
    uip_ipaddr_copy(&nodes[node_count].endpoint.ipaddr, addr);
    nodes[node_count].endpoint.port = REMOTE_PORT;
    return &nodes[node_count++];
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
set_value(const uip_ipaddr_t *addr, char *uri, char *value)
{
  int i;
  printf("#set value ");
  uip_debug_ipaddr_print(addr);
  printf(" URI: %s Value: %s\n", uri, value);

  for(i = 0; i < node_count; i++) {
    if(uip_ipaddr_cmp(&nodes[i].endpoint.ipaddr, addr)) {
      /* setup command */
      current_target = &nodes[i];
      current_request = COAP_PUT;
      strncpy(current_uri, uri, sizeof(current_uri) - 1);
      strncpy(current_value, value, sizeof(current_value) - 1);
      process_post(&router_process, EVENT_RUN_NOW, NULL);
      break;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
get_value(const uip_ipaddr_t *addr, char *uri)
{
  int i;
  printf("#get value ");
  uip_debug_ipaddr_print(addr);
  printf(" URI: %s\n", uri);

  for(i = 0; i < node_count; i++) {
    if(uip_ipaddr_cmp(&nodes[i].endpoint.ipaddr, addr)) {
      /* setup command */
      current_target = &nodes[i];
      current_request = COAP_GET;
      strncpy(current_uri, uri, sizeof(current_uri) - 1);
      current_value[0] = 0;
      process_post(&router_process, EVENT_RUN_NOW, NULL);
      break;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
print_node_list(void)
{
  int i;
  int out = 0;
  for(i = 0; i < node_count; i++) {
    if(nodes[i].flags & NODE_HAS_TYPE) {
      if(out++) {
        printf(";");
      }
      printf("%s,", nodes[i].type);
      uip_debug_ipaddr_print(&nodes[i].endpoint.ipaddr);
    }
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
/**
 * This function is will be passed to COAP_BLOCKING_REQUEST() to
 * handle responses.
 */
static void
client_chunk_handler(coap_message_t *response)
{
  const uint8_t *chunk;
  unsigned int format;
  int len = coap_get_payload(response, &chunk);
  coap_get_header_content_format(response, &format);

  /* if(len > 0) { */
  /*   printf("|%.*s (%d,%d)", len, (char *)chunk, len, format); */
  /* } */
  if(response->code >= BAD_REQUEST_4_00) {
    PRINTF("\nReceived error %u: %.*s\n", response->code, len, (char *)chunk);
  } else if(current_target != NULL && fetching_type) {
    if(len > sizeof(current_target->type) - 1) {
      len = sizeof(current_target->type) - 1;
    }
    memcpy(current_target->type, chunk, len);
    current_target->type[len] = 0;
    current_target->flags |= NODE_HAS_TYPE;

    PRINTF("\nNODE ");
    PRINT6ADDR(&current_target->endpoint.ipaddr);
    PRINTF(" HAS TYPE %s\n", current_target->type);
  } else {
    /* otherwise update the current value */
    if(format == LWM2M_TLV) {
      lwm2m_tlv_t tlv;
      /* we can only read int32 for now ? */
      if(lwm2m_tlv_read(&tlv, chunk, len) > 0) {
        /* printf("TLV.type=%d len=%d id=%d value[0]=%d\n", */
        /*        tlv.type, tlv.length, tlv.id, tlv.value[0]); */

        int value = lwm2m_tlv_get_int32(&tlv);
        snprintf(current_value, sizeof(current_value) - 1, "%d", value);
      } else {
        PRINTF("Failed to parse LWM2M TLV\n");
      }
    } else {
      if(len > sizeof(current_value) - 1) {
        len = sizeof(current_value) - 1;
      }
      memcpy(current_value, chunk, len);
      current_value[len] = 0;
    }
  }
}
/*---------------------------------------------------------------------------*/
#if UIP_CONF_IPV6_RPL
static bool
check_rpl_routes(void)
{
  uip_sr_node_t *link;
  uip_ipaddr_t child_ipaddr;
  uip_ipaddr_t parent_ipaddr;

  /* Our routing links */
  for(link = uip_sr_node_head(); link != NULL; link = uip_sr_node_next(link)) {
    NETSTACK_ROUTING.get_sr_node_ipaddr(&child_ipaddr, link);

    if(link->parent == NULL) {
      /* Igore the DAG root */
      continue;
    }

    current_target = add_node(&child_ipaddr);
    if(current_target == NULL ||
       (current_target->flags & NODE_HAS_TYPE) != 0 ||
       current_target->retries > 5) {
      continue;
    }

    NETSTACK_ROUTING.get_sr_node_ipaddr(&parent_ipaddr, link->parent);
    PRINTF("  ");
    PRINT6ADDR(&child_ipaddr);
    PRINTF("  ->  ");
    PRINT6ADDR(&parent_ipaddr);
    PRINTF("\n");
    return true;
  }
  return false;
}
#endif /* UIP_CONF_IPV6_RPL */
/*---------------------------------------------------------------------------*/
#if (UIP_MAX_ROUTES != 0)
static bool
check_routes(void)
{
  uip_ds6_route_t *r;

  for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
    current_target = add_node(&r->ipaddr);
    if(current_target == NULL ||
       (current_target->flags & NODE_HAS_TYPE) != 0 ||
       current_target->retries > 5) {
      continue;
    }
    PRINTF("  ");
    PRINT6ADDR(&r->ipaddr);
    PRINTF("  ->  ");
    nexthop = uip_ds6_route_nexthop(r);
    if(nexthop != NULL) {
      PRINT6ADDR(nexthop);
    } else {
      PRINTF("-");
    }
    PRINTF("\n");
    return true;
  }
  return false;
}
#endif /* (UIP_MAX_ROUTES != 0) */
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(router_process, ev, data)
{
  /* This way the message can be treated as pointer as usual. */
  static coap_message_t request[1];
  static struct etimer timer;

  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  while(1) {
    etimer_set(&timer, CLOCK_SECOND * 5);
    PROCESS_YIELD();

    /* Handle serial line input */
    if(ev == serial_line_event_message) {
      serial_protocol_input((char *) data);
    }

    if(etimer_expired(&timer)) {
      current_target = NULL;
#if UIP_CONF_IPV6_RPL
      check_rpl_routes();
#endif /* UIP_CONF_IPV6_RPL */

#if (UIP_MAX_ROUTES != 0)
      if(current_target == NULL) {
        check_routes();
      }
#endif /* (UIP_MAX_ROUTES != 0) */
    }

    /* This is a node type discovery */
    if(current_target != NULL &&
       (current_target->flags & NODE_HAS_TYPE) == 0 &&
       current_target->retries < 6) {

      /* prepare request, TID is set by COAP_BLOCKING_REQUEST() */
      coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
      coap_set_header_uri_path(request, URL_DEVICE_MODEL);

      current_target->retries++;

      PRINTF("CoAP request to [");
      PRINT6ADDR(&current_target->endpoint.ipaddr);
      PRINTF("]:%u (%u tx)\n", UIP_HTONS(current_target->endpoint.port),
             current_target->retries);

      fetching_type = 1;
      COAP_BLOCKING_REQUEST(&current_target->endpoint, request,
                            client_chunk_handler);
      fetching_type = 0;
      strncpy(current_uri, URL_LIGHT_CONTROL, sizeof(current_uri) - 1);
      printf("\n--Done--\n");
    }

    /* If having a type this is another type of request */
    if(current_target != NULL &&
       (current_target->flags & NODE_HAS_TYPE) && strlen(current_uri) > 0) {
      /* prepare request, TID is set by COAP_BLOCKING_REQUEST() */
      coap_init_message(request, COAP_TYPE_CON, current_request, 0);
      coap_set_header_uri_path(request, current_uri);

      if(strlen(current_value) > 0) {
        coap_set_payload(request, (uint8_t *)current_value,
                         strlen(current_value));
      }

      PRINTF("CoAP request to [");
      PRINT6ADDR(&current_target->endpoint.ipaddr);
      PRINTF("]:%u %s\n", UIP_HTONS(current_target->endpoint.port),
             current_uri);

      COAP_BLOCKING_REQUEST(&current_target->endpoint, request,
                            client_chunk_handler);

      /* print out result of command */
      if(current_request == COAP_PUT) {
        printf("s ");
      } else {
        printf("g ");
      }
      uip_debug_ipaddr_print(&current_target->endpoint.ipaddr);
      printf(" %s %s\n", current_uri, current_value);

      current_target = NULL;
      current_uri[0] = 0;
      current_value[0] = 0;
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
