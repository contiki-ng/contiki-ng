#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

/* include the project config */
#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif /* PROJECT_CONF_PATH */
/*---------------------------------------------------------------------------*/
#include "z1-def.h"
#include "msp430-def.h"
/*---------------------------------------------------------------------------*/

/* Configure radio driver */
#ifndef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO   cc2420_driver
#endif /* NETSTACK_CONF_RADIO */

/* Symbol for the TSCH 15ms timeslot timing template */
#define TSCH_CONF_ARCH_HDR_PATH "dev/radio/cc2420/cc2420-tsch-15ms.h"

/* The TSCH default slot length of 10ms is a bit too short for this platform,
 * use 15ms instead. */
#ifndef TSCH_CONF_DEFAULT_TIMESLOT_TIMING
#define TSCH_CONF_DEFAULT_TIMESLOT_TIMING tsch_timeslot_timing_us_15000
#endif /* TSCH_CONF_DEFAULT_TIMESLOT_TIMING */

/* Save RAM through a smaller uIP buffer */
#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE		140
#endif

#define PROCESS_CONF_NUMEVENTS       8
#define PROCESS_CONF_STATS           1
/*#define PROCESS_CONF_FASTPOLL      4*/

/* So far, printfs without interrupt. */
#define UART0_CONF_TX_WITH_INTERRUPT 0
/* This does not work in Cooja. */
#define UART0_CONF_RX_WITH_DMA       0

/* Handle 8 neighbors */
#ifndef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS    8
#endif

/* Handle 8 routes    */
#ifndef NETSTACK_MAX_ROUTE_ENTRIES
#define NETSTACK_MAX_ROUTE_ENTRIES      8
#endif

#ifndef UIP_CONF_UDP_CONNS
#define UIP_CONF_UDP_CONNS 2
#endif

#ifndef TSCH_CONF_MAX_INCOMING_PACKETS
#define TSCH_CONF_MAX_INCOMING_PACKETS    2
#endif

#ifndef TSCH_QUEUE_CONF_NUM_PER_NEIGHBOR
#define TSCH_QUEUE_CONF_NUM_PER_NEIGHBOR  4
#endif

#ifndef TSCH_LOG_CONF_QUEUE_LEN
#define TSCH_LOG_CONF_QUEUE_LEN 4
#endif

#define TSCH_CONF_HW_FRAME_FILTERING 0

/* Platform-specific (H/W) AES implementation */
#ifndef AES_128_CONF
#define AES_128_CONF cc2420_aes_128_driver
#endif /* AES_128_CONF */

/*---------------------------------------------------------------------------*/
#include "msp430-conf.h"
/*---------------------------------------------------------------------------*/
#endif /* CONTIKI_CONF_H */
