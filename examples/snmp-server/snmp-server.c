/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "snmp.h"

#include <string.h>
#include <strings.h>

/*---------------------------------------------------------------------------*/
PROCESS_NAME(snmp_server_process);
AUTOSTART_PROCESSES(&snmp_server_process);
/*---------------------------------------------------------------------------*/

PROCESS(snmp_server_process, "SNMP Server");

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(snmp_server_process, ev, data)
{

  PROCESS_BEGIN();

  snmp_init();

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
