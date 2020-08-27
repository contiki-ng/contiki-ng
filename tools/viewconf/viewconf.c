#include "contiki.h"
#include "os/net/mac/framer/frame802154.h"
#include "os/net/mac/tsch/tsch.h"
#include "os/net/ipv6/uip-nd6.h"
#include "os/net/ipv6/uipopt.h"
#include "os/net/queuebuf.h"
#include "os/net/nbr-table.h"
#include "os/sys/log-conf.h"
#include "os/sys/energest.h"

#ifdef PROJECT_CONF_PATH
##### "PROJECT_CONF_PATH": _____________________ == PROJECT_CONF_PATH
#else
##### "PROJECT_CONF_PATH": _____________________ ><
#endif

##### "CONTIKI_VERSION_STRING": ________________ == CONTIKI_VERSION_STRING

#undef FRAME802154_IEEE802154_2003
#undef FRAME802154_IEEE802154_2006
#undef FRAME802154_IEEE802154_2015

#ifdef FRAME802154_CONF_VERSION
##### "FRAME802154_CONF_VERSION":_______________ == FRAME802154_CONF_VERSION
#else
##### "FRAME802154_CONF_VERSION":_______________ == FRAME802154_VERSION
#endif

#ifdef IEEE802154_CONF_PANID
##### "IEEE802154_CONF_PANID":__________________ == IEEE802154_CONF_PANID
#else
##### "IEEE802154_CONF_PANID":__________________ == IEEE802154_PANID
#endif

#if MAC_CONF_WITH_TSCH

#ifdef TSCH_CONF_DEFAULT_HOPPING_SEQUENCE
##### "TSCH_CONF_DEFAULT_HOPPING_SEQUENCE": ____ == TSCH_CONF_DEFAULT_HOPPING_SEQUENCE
#else
##### "TSCH_CONF_DEFAULT_HOPPING_SEQUENCE": ____ -> TSCH_DEFAULT_HOPPING_SEQUENCE
#endif

#ifdef TSCH_CONF_JOIN_HOPPING_SEQUENCE
##### "TSCH_CONF_JOIN_HOPPING_SEQUENCE": _______ == TSCH_CONF_JOIN_HOPPING_SEQUENCE
#else
##### "TSCH_CONF_JOIN_HOPPING_SEQUENCE": _______ -> TSCH_JOIN_HOPPING_SEQUENCE
#endif

#ifdef TSCH_CONF_EB_PERIOD
##### "TSCH_CONF_EB_PERIOD": ___________________ == TSCH_CONF_EB_PERIOD
#else
##### "TSCH_CONF_EB_PERIOD": ___________________ -> TSCH_EB_PERIOD
#endif

#ifdef TSCH_CONF_MAX_EB_PERIOD
##### "TSCH_CONF_MAX_EB_PERIOD": _______________ == TSCH_CONF_MAX_EB_PERIOD
#else
##### "TSCH_CONF_MAX_EB_PERIOD": _______________ -> TSCH_MAX_EB_PERIOD
#endif

#ifdef TSCH_CONF_DEFAULT_TIMESLOT_TIMING
##### "TSCH_CONF_DEFAULT_TIMESLOT_TIMING": _____ == TSCH_CONF_DEFAULT_TIMESLOT_TIMING
#else
##### "TSCH_CONF_DEFAULT_TIMESLOT_TIMING": _____ -> TSCH_DEFAULT_TIMESLOT_TIMING
#endif

#ifdef TSCH_SCHEDULE_CONF_DEFAULT_LENGTH
##### "TSCH_SCHEDULE_CONF_DEFAULT_LENGTH": _____ == TSCH_SCHEDULE_CONF_DEFAULT_LENGTH
#else
##### "TSCH_SCHEDULE_CONF_DEFAULT_LENGTH": _____ -> TSCH_SCHEDULE_DEFAULT_LENGTH
#endif

#else /* MAC_CONF_WITH_TSCH */

#ifdef IEEE802154_CONF_DEFAULT_CHANNEL
##### "IEEE802154_CONF_DEFAULT_CHANNEL": _______ == IEEE802154_CONF_DEFAULT_CHANNEL
#else
##### "IEEE802154_CONF_DEFAULT_CHANNEL": _______ -> IEEE802154_DEFAULT_CHANNEL
#endif

#endif /*MAC_CONF_WITH_TSCH */

#if ROUTING_CONF_RPL_LITE || ROUTING_CONF_RPL_CLASSIC

#undef RPL_MOP_NO_DOWNWARD_ROUTES
#undef RPL_MOP_NON_STORING
#undef RPL_MOP_STORING_NO_MULTICAST
#undef RPL_MOP_STORING_MULTICAST

#ifdef RPL_CONF_MOP
##### "RPL_CONF_MOP": __________________________ == RPL_CONF_MOP
#else
##### "RPL_CONF_MOP": __________________________ -> RPL_MOP_DEFAULT
#endif

#endif /* RPL routing */

#ifdef QUEUEBUF_CONF_NUM
##### "QUEUEBUF_CONF_NUM": _____________________ == QUEUEBUF_CONF_NUM
#else
##### "QUEUEBUF_CONF_NUM": _____________________ -> QUEUEBUF_NUM
#endif

#ifdef NBR_TABLE_CONF_MAX_NEIGHBORS
##### "NBR_TABLE_CONF_MAX_NEIGHBORS": __________ == NBR_TABLE_CONF_MAX_NEIGHBORS
#else
##### "NBR_TABLE_CONF_MAX_NEIGHBORS": __________ -> NBR_TABLE_MAX_NEIGHBORS
#endif

##### "NETSTACK_MAX_ROUTE_ENTRIES": ____________ == NETSTACK_MAX_ROUTE_ENTRIES
##### "UIP_CONF_BUFFER_SIZE": __________________ == UIP_CONF_BUFFER_SIZE
##### "UIP_CONF_UDP": __________________________ == UIP_CONF_UDP

#ifdef UIP_CONF_UDP_CONNS
##### "UIP_CONF_UDP_CONNS": ____________________ == UIP_CONF_UDP_CONNS
#else
##### "UIP_CONF_UDP_CONNS": ____________________ -> UIP_UDP_CONNS
#endif

##### "UIP_CONF_TCP": __________________________ == UIP_CONF_TCP

#ifdef UIP_CONF_TCP_CONNS
##### "UIP_CONF_TCP_CONNS": ____________________ == UIP_CONF_TCP_CONNS
#else
##### "UIP_CONF_TCP_CONNS": ____________________ -> UIP_TCP_CONNS
#endif

#ifdef UIP_CONF_ND6_SEND_RA
##### "UIP_CONF_ND6_SEND_RA": __________________ == UIP_CONF_ND6_SEND_RA
#else
##### "UIP_CONF_ND6_SEND_RA": __________________ -> UIP_ND6_SEND_RA
#endif

#ifdef UIP_CONF_ND6_SEND_NS
##### "UIP_CONF_ND6_SEND_NS": __________________ == UIP_CONF_ND6_SEND_NS
#else
##### "UIP_CONF_ND6_SEND_NS": __________________ -> UIP_ND6_SEND_NS
#endif

#ifdef UIP_CONF_ND6_SEND_NA
##### "UIP_CONF_ND6_SEND_NA": __________________ == UIP_CONF_ND6_SEND_NA
#else
##### "UIP_CONF_ND6_SEND_NA": __________________ -> UIP_ND6_SEND_NA
#endif

#ifdef UIP_CONF_ND6_AUTOFILL_NBR_CACHE
##### "UIP_CONF_ND6_AUTOFILL_NBR_CACHE": _______ == UIP_CONF_ND6_AUTOFILL_NBR_CACHE
#else
##### "UIP_CONF_ND6_AUTOFILL_NBR_CACHE": _______ -> UIP_ND6_AUTOFILL_NBR_CACHE
#endif

##### "SICSLOWPAN_CONF_FRAG": __________________ == SICSLOWPAN_CONF_FRAG

#ifdef SICSLOWPAN_CONF_COMPRESSION
##### "SICSLOWPAN_CONF_COMPRESSION": ___________ == SICSLOWPAN_CONF_COMPRESSION
#else
##### "SICSLOWPAN_CONF_COMPRESSION": ___________ -> SICSLOWPAN_COMPRESSION
#endif

##### "ENERGEST_CONF_ON": ______________________ == ENERGEST_CONF_ON

##### "LOG_CONF_LEVEL_RPL": ____________________ == LOG_CONF_LEVEL_RPL
##### "LOG_CONF_LEVEL_TCPIP": __________________ == LOG_CONF_LEVEL_TCPIP
##### "LOG_CONF_LEVEL_IPV6": ___________________ == LOG_CONF_LEVEL_IPV6
##### "LOG_CONF_LEVEL_6LOWPAN": ________________ == LOG_CONF_LEVEL_6LOWPAN
##### "LOG_CONF_LEVEL_NULLNET": ________________ == LOG_CONF_LEVEL_NULLNET
##### "LOG_CONF_LEVEL_MAC": ____________________ == LOG_CONF_LEVEL_MAC
##### "LOG_CONF_LEVEL_FRAMER": _________________ == LOG_CONF_LEVEL_FRAMER
##### "LOG_CONF_LEVEL_6TOP": ___________________ == LOG_CONF_LEVEL_6TOP
##### "LOG_CONF_LEVEL_COAP": ___________________ == LOG_CONF_LEVEL_COAP
##### "LOG_CONF_LEVEL_SNMP": ___________________ == LOG_CONF_LEVEL_SNMP
##### "LOG_CONF_LEVEL_LWM2M": __________________ == LOG_CONF_LEVEL_LWM2M
##### "LOG_CONF_LEVEL_MAIN": ___________________ == LOG_CONF_LEVEL_MAIN
