#include <stdio.h>

#include "sys/node-id.h"
#include "lib/crc16.h"

#include "contiki-conf.h"
#include "vesna-def.h"

// required by Contiki
uint16_t node_id = 0;

void
node_id_init(void)
{
#ifdef NODEID // NodeID was provided at compile-time
	node_id = NODEID; // can be manually configured
#else
	node_id = crc16_data(STM32F1_UUID.u8, sizeof(STM32F1_UUID.u8), 0);
#endif
}

