#include "contiki.h"
#include "rpl.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uipbuf.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

static struct simple_udp_connection udp_conn;

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL		  (60 * CLOCK_SECOND)

#define UIP_IP_BUF       ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

static struct simple_udp_connection udp_conn;
static uip_ipaddr_t server_ipaddr;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client -- packet debug");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static enum netstack_ip_action
ip_input(void)
{
  uint8_t proto = 0;
  uipbuf_get_last_header(uip_buf, uip_len, &proto);
  LOG_INFO("Incoming packet proto: %d from ", proto);
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_("\n");
  return NETSTACK_IP_PROCESS;
}
/*---------------------------------------------------------------------------*/
static enum netstack_ip_action
ip_output(const linkaddr_t *localdest)
{
  uint8_t proto;
  uint8_t is_me = 0;
  uipbuf_get_last_header(uip_buf, uip_len, &proto);
  is_me =  uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr);
  LOG_INFO("Outgoing packet (%s) proto: %d to ", is_me ? "send" : "fwd ", proto);
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_("\n");
  return NETSTACK_IP_PROCESS;
}
/*---------------------------------------------------------------------------*/
struct netstack_ip_packet_processor packet_processor = {
  .process_input = ip_input,
  .process_output = ip_output
};
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  unsigned count = *(unsigned *)data;
  LOG_INFO("Received response %u from ", count);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count;

  PROCESS_BEGIN();

  INIT_SERVER_IPADDR(server_ipaddr);
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  /* register packet processor */
  netstack_ip_packet_processor_add(&packet_processor);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(rpl_is_reachable()) {
      LOG_INFO("Sending request %u to ", count);
      LOG_INFO_6ADDR(&server_ipaddr);
      LOG_INFO_("\n");
      simple_udp_sendto(&udp_conn, &count, sizeof(count), &server_ipaddr);
      count++;
    } else {
      LOG_INFO("Not reachable yet %p\n", rpl_get_any_dag());
    }

    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
