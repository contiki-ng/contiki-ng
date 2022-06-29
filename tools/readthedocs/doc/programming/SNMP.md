# SNMP


## Documentation: SNMP

### SNMP in Contiki-NG

The SNMP implementation in Contiki-NG was designed to have a small code footprint, have well defined modules and be easy to use.

 - ```snmp-api.{c,h}``` Contains the functions that are to be used by the application. These functions are not supposed to change and if they do change there has to be a very good reason.
 - ```snmp-ber.{c,h}``` Contains the basic BER encoding functions. 
 - ```snmp-conf.h``` Contains the defines that can be overwritten in the project-conf.h
 - ```snmp-engine.{c,h}``` Contains the protocol engine. It is responsible for handling the incoming request and generating the correct response.
 - ```snmp-message.{c,h}``` Contains the message. It is responsible for encoding and decoding the buffer to or from the struct.
 - ```snmp-mib.{c,h}``` Contains the mib. It is responsible for all the function related to the mib structure.
 - ```snmp-oid.{c,h}``` Contains the oid. It is responsible for encoding an oid from the struct into a buffer or vice versa.
 - ```snmp.{c,h}``` Contains the contiki process and the udp handler function.

### Versions
The current implementation of the SNMP engine only supports versions 1 and 2c of the protocol. The implementation only supports SNMP GET requests.

### Usage
The SNMP module was implemented to be extremely easy to use without knowing the internals. The first step is to enable the module in the Makefile.
```
MODULES += os/net/app-layer/snmp
```

A very simple project that exports the sysDescrt mib can be seen below.

```c
#include "contiki.h"
#include "snmp-api.h"

/*----------------------- SNMP Resource -------------------------------------*/
static void
sysDescr_handler(snmp_varbind_t *varbind, uint32_t *oid);

MIB_RESOURCE(sysDescr, sysDescr_handler, 1, 3, 6, 1, 2, 1, 1, 1, 0);

static void
sysDescr_handler(snmp_varbind_t *varbind, uint32_t *oid)
{
	snmp_api_set_string(varbind, oid, CONTIKI_VERSION_STRING);
}
/*---------------------------------------------------------------------------*/

/*------------------------ SNMP Server --------------------------------------*/
PROCESS_NAME(snmp_server_process);
AUTOSTART_PROCESSES(&snmp_server_process);
PROCESS(snmp_server_process, "SNMP Server");

PROCESS_THREAD(snmp_server_process, ev, data)
{
	PROCESS_BEGIN();

	snmp_api_add_resource(&sysDescr);

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
```


### Todo
- Add a define to enable or disable a certain version
- Add more BER types
- Add a way to enable or disable BER types to reduce code footprint
- Add version 3

Author: @yagoor
