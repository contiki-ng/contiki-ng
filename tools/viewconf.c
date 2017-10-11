#include "contiki.h"
#include "os/net/mac/framer/frame802154.h"
#include "os/net/mac/tsch/tsch.h"
#include "os/net/mac/tsch/tsch-conf.h"
#include "os/net/ipv6/uip-nd6.h"
#include "os/net/ipv6/uipopt.h"
#include "os/net/queuebuf.h"
#include "os/net/nbr-table.h"
#include "os/sys/log-conf.h"

##### "PROJECT_CONF_PATH": _____________________ PROJECT_CONF_PATH
##### "CONTIKI_VERSION_STRING": ________________ CONTIKI_VERSION_STRING
##### "IEEE802154_PANID":_______________________ IEEE802154_PANID
##### "FRAME802154_VERSION":____________________ FRAME802154_VERSION
##### "RF_CHANNEL": ____________________________ RF_CHANNEL
##### "TSCH_DEFAULT_HOPPING_SEQUENCE": _________ TSCH_DEFAULT_HOPPING_SEQUENCE
##### "TSCH_JOIN_HOPPING_SEQUENCE": ____________ TSCH_JOIN_HOPPING_SEQUENCE
##### "TSCH_CONF_DEFAULT_TIMESLOT_LENGTH": _____ TSCH_CONF_DEFAULT_TIMESLOT_LENGTH
##### "QUEUEBUF_NUM": __________________________ QUEUEBUF_CONF_NUM
##### "NBR_TABLE_MAX_NEIGHBORS": _______________ NBR_TABLE_CONF_MAX_NEIGHBORS
##### "NETSTACK_MAX_ROUTE_ENTRIES": ____________ NETSTACK_MAX_ROUTE_ENTRIES
##### "UIP_CONF_BUFFER_SIZE": __________________ UIP_CONF_BUFFER_SIZE
##### "UIP_CONF_UDP": __________________________ UIP_CONF_UDP
##### "UIP_UDP_CONNS": _________________________ UIP_UDP_CONNS
##### "UIP_CONF_TCP": __________________________ UIP_CONF_TCP
##### "UIP_TCP_CONNS": _________________________ UIP_TCP_CONNS
##### "UIP_ND6_SEND_RA": _______________________ UIP_ND6_SEND_RA
##### "UIP_ND6_SEND_NS": _______________________ UIP_ND6_SEND_NS
##### "UIP_ND6_SEND_NA": _______________________ UIP_ND6_SEND_NA
##### "UIP_ND6_AUTOFILL_NBR_CACHE": ____________ UIP_ND6_AUTOFILL_NBR_CACHE
##### "SICSLOWPAN_CONF_FRAG": __________________ SICSLOWPAN_CONF_FRAG
##### "SICSLOWPAN_COMPRESSION": ________________ SICSLOWPAN_COMPRESSION
##### "LOG_CONF_LEVEL_RPL": ____________________ LOG_CONF_LEVEL_RPL
##### "LOG_CONF_LEVEL_TCPIP": __________________ LOG_CONF_LEVEL_TCPIP
##### "LOG_CONF_LEVEL_IPV6": ___________________ LOG_CONF_LEVEL_IPV6
##### "LOG_CONF_LEVEL_6LOWPAN": ________________ LOG_CONF_LEVEL_6LOWPAN
##### "LOG_CONF_LEVEL_NULLNET": ________________ LOG_CONF_LEVEL_NULLNET
##### "LOG_CONF_LEVEL_MAC": ____________________ LOG_CONF_LEVEL_MAC
##### "LOG_CONF_LEVEL_FRAMER": _________________ LOG_CONF_LEVEL_FRAMER
##### "LOG_CONF_LEVEL_6TOP": ___________________ LOG_CONF_LEVEL_6TOP
