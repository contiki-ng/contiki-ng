#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/routing/routing.h"
#include "node-id.h"
#include <stdint.h>
#include <inttypes.h>

/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
/*---------------------------------------------------------------------------*/

#define UDP_PORT      61618
#define PAYLOAD_SIZE  100
#define TX_INTERVAL   (10 * CLOCK_SECOND)
#define TX_WAIT       (60 * CLOCK_SECOND)

#ifdef SIMPLE_CONF_TX_COUNT_MAX
#define SIMPLE_TX_COUNT_MAX  SIMPLE_CONF_TX_COUNT_MAX
#else
#define SIMPLE_TX_COUNT_MAX  1000
#endif

#ifdef SIMPLE_CONF_NODE_TX_ID
#define SIMPLE_NODE_TX_ID    SIMPLE_CONF_NODE_TX_ID
#else
#define SIMPLE_NODE_TX_ID    2
#endif

#ifdef SIMPLE_CONF_NODE_RX_ID
#define SIMPLE_NODE_RX_ID    SIMPLE_CONF_NODE_RX_ID
#else
#define SIMPLE_NODE_RX_ID    1
#endif

static struct simple_udp_connection unicast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(udp_process, "UDP unicast process");
AUTOSTART_PROCESSES(&udp_process);
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  static uint32_t rx_count = 0;
  rx_count++;
  LOG_INFO("Data %" PRIu32 " received on port %" PRIu16 " from "
           "port %" PRIu16 " with length %" PRIu16 "\n",
           rx_count, receiver_port, sender_port, datalen);

  if(rx_count == SIMPLE_TX_COUNT_MAX) {
    LOG_INFO("Received all packets\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_process, ev, data)
{
  static struct etimer periodic_timer;
  static bool is_transmitter = false;
  static uint32_t tx_count = 0;

  PROCESS_BEGIN();

  if(node_id == 1) {
    NETSTACK_ROUTING.root_start();
  }

  if(node_id == SIMPLE_NODE_TX_ID) {
    is_transmitter = true;
    LOG_INFO("Transmitter TX in %ld sec., interval %ld sec., count %d\n",
             TX_WAIT / CLOCK_SECOND, TX_INTERVAL / CLOCK_SECOND,
             SIMPLE_TX_COUNT_MAX);
  }

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  etimer_set(&periodic_timer, TX_WAIT);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

  while(1) {
    if(is_transmitter && tx_count < SIMPLE_TX_COUNT_MAX) {
      LOG_INFO("Sending unicast\n");
      const uip_ipaddr_t *default_prefix = uip_ds6_default_prefix();
      uint8_t buf[PAYLOAD_SIZE] = {0};
      uip_ipaddr_t dest_addr;
      uip_ip6addr_copy(&dest_addr, default_prefix);
      dest_addr.u16[4] = UIP_HTONS(0x200 + SIMPLE_NODE_RX_ID);
      dest_addr.u16[5] = UIP_HTONS(SIMPLE_NODE_RX_ID);
      dest_addr.u16[6] = UIP_HTONS(SIMPLE_NODE_RX_ID);
      dest_addr.u16[7] = UIP_HTONS(SIMPLE_NODE_RX_ID);
      LOG_INFO("Sending unicast to ");
      LOG_INFO_6ADDR(&dest_addr);
      LOG_INFO_("\n");
      simple_udp_sendto(&unicast_connection, buf, sizeof(buf), &dest_addr);
      tx_count++;
    }

    etimer_set(&periodic_timer, TX_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
