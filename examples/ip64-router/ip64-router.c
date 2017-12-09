#include "contiki.h"
#include "contiki-net.h"
#include "ip64/ip64.h"
#include "net/netstack.h"
#include "net/routing/routing.h"
#include "sys/autostart.h"

/*---------------------------------------------------------------------------*/
PROCESS(router_node_process, "Router node");
AUTOSTART_PROCESSES(&router_node_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(router_node_process, ev, data)
{
  PROCESS_BEGIN();

  /* Set us up as a RPL root node. */
  NETSTACK_ROUTING.root_start();

  /* Initialize the IP64 module so we'll start translating packets */
  ip64_init();

  /* ... and do nothing more. */
  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
