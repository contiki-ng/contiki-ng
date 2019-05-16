#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uipbuf.h"
#include "net/ipv6/uip-ds6.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/*---------------------------------------------------------------------------*/
PROCESS(ipv6_hooks_process, "IPv6 Hooks");
AUTOSTART_PROCESSES(&ipv6_hooks_process);
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
PROCESS_THREAD(ipv6_hooks_process, ev, data)
{
  PROCESS_BEGIN();

  /* Register packet processor */
  netstack_ip_packet_processor_add(&packet_processor);

  /* Do nothing */

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
