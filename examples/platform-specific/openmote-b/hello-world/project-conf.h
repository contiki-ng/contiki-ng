#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Configure OpenMote B to use Atmel's AT86RF215 radio */
#define OPENMOTEB_CONF_USE_ATMEL_RADIO              (1)

/* Additional configuration of Atmel's Radio */
#define LOG_CONF_LEVEL_AT86RF215                    LOG_LEVEL_INFO

/* Define log levels of other modules */
#define LOG_CONF_LEVEL_MAIN                         LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_IPV6                         LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_RPL                          LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_6LOWPAN                      LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_TCPIP                        LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC                          LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_FRAMER                       LOG_LEVEL_WARN
#define TSCH_LOG_CONF_PER_SLOT                      (1)

#endif