#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6.h"
#include "net/netstack.h"
#include "net/routing/routing.h"
#include "sys/log.h"
#include "sys/node-id.h"

#include <inttypes.h>

#define LOG_MODULE "Delivery"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_PORT 61618

/* Which node sends the test packets */
#ifndef TEST_CONF_TX_NODE_ID
#define TEST_CONF_TX_NODE_ID 1
#endif

/* Which node receives and counts them */
#ifndef TEST_CONF_RX_NODE_ID
#define TEST_CONF_RX_NODE_ID 3
#endif

/* Send to the destination's link-local address instead of its global address.
 * This produces a direct layer-2 unicast to the neighbor, bypassing RPL
 * routing, so it exercises the scheduler's handling of a neighbor that is
 * neither our RPL parent nor one of our children. */
#ifndef TEST_CONF_USE_LINKLOCAL
#define TEST_CONF_USE_LINKLOCAL 0
#endif

#ifndef TEST_CONF_TX_START_SECONDS
#define TEST_CONF_TX_START_SECONDS 360
#endif

#ifndef TEST_CONF_TX_INTERVAL_SECONDS
#define TEST_CONF_TX_INTERVAL_SECONDS 10
#endif

#ifndef TEST_CONF_REQUIRED_RX_COUNT
#define TEST_CONF_REQUIRED_RX_COUNT 10
#endif

static struct simple_udp_connection udp_connection;

PROCESS(node_process, "Orchestra delivery test");
AUTOSTART_PROCESSES(&node_process);

static void
receiver(struct simple_udp_connection *connection,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  static uint32_t rx_count;

  if(node_id != TEST_CONF_RX_NODE_ID) {
    return;
  }

  rx_count++;
  LOG_INFO("Received test packet %" PRIu32 " of %u\n",
           rx_count, TEST_CONF_REQUIRED_RX_COUNT);
  if(rx_count == TEST_CONF_REQUIRED_RX_COUNT) {
    LOG_INFO("Received all test packets\n");
  }
}

static void
build_node_addr(uip_ipaddr_t *addr, unsigned int nid)
{
  /* A Cooja mote's link-layer address repeats its node id, so its interface
   * identifier is 0200+id:id:id:id (the U/L bit is flipped in the first byte). */
#if TEST_CONF_USE_LINKLOCAL
  uip_ip6addr(addr, 0xfe80, 0, 0, 0, 0x0200 + nid, nid, nid, nid);
#else
  const uip_ipaddr_t *prefix = uip_ds6_default_prefix();
  uip_ip6addr_copy(addr, prefix);
  addr->u16[4] = UIP_HTONS(0x200 + nid);
  addr->u16[5] = UIP_HTONS(nid);
  addr->u16[6] = UIP_HTONS(nid);
  addr->u16[7] = UIP_HTONS(nid);
#endif
}

PROCESS_THREAD(node_process, event, data)
{
  static struct etimer timer;
  static uint32_t sequence;

  PROCESS_BEGIN();

  if(node_id == 1) {
    NETSTACK_ROUTING.root_start();
  }
  NETSTACK_MAC.on();

  simple_udp_register(&udp_connection, UDP_PORT, NULL, UDP_PORT, receiver);

  etimer_set(&timer, TEST_CONF_TX_START_SECONDS * CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  while(1) {
    if(node_id == TEST_CONF_TX_NODE_ID) {
      uip_ipaddr_t destination;

      build_node_addr(&destination, TEST_CONF_RX_NODE_ID);
      sequence++;
      LOG_INFO("Sending test packet %" PRIu32 " to node %u\n", sequence,
               (unsigned)TEST_CONF_RX_NODE_ID);
      simple_udp_sendto(&udp_connection, &sequence, sizeof(sequence),
                        &destination);
    }

    etimer_set(&timer, TEST_CONF_TX_INTERVAL_SECONDS * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  }

  PROCESS_END();
}
