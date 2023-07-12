#define LOG_CONF_LEVEL_IPV6                        LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_RPL                         LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_6LOWPAN                     LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_TCPIP                       LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_DBG
/* Do not enable LOG_CONF_LEVEL_FRAMER on SimpleLink,
   that will cause it to print from an interrupt context. */
#ifndef CONTIKI_TARGET_SIMPLELINK
#define LOG_CONF_LEVEL_FRAMER                      LOG_LEVEL_DBG
#endif
