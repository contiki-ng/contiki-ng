#ifndef TZ_RPL_UDP_NORMAL_PROJECT_CONF_H_
#define TZ_RPL_UDP_NORMAL_PROJECT_CONF_H_

#include "../../../../../rpl-udp/project-conf.h"

#ifndef WATCHDOG_CONF_ENABLE
#define WATCHDOG_CONF_ENABLE 0
#endif

#define STACK_CHECK_CONF_PERIODIC_CHECKS 0
#define TZ_DEBUG_KEEP_SECURE_UART 1

#ifndef TZ_RPL_UDP_DEBUG
#define TZ_RPL_UDP_DEBUG 0
#endif

#endif /* TZ_RPL_UDP_NORMAL_PROJECT_CONF_H_ */
