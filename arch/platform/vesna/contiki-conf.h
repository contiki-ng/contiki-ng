#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

#include <stdint.h>
#include <string.h>

#define CCIF
#define CLIF

#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif


#include "vesna-def.h"


#define CLOCK_CONF_SECOND	(1000)



// Platform specific timer bit-size
// STM32F103 has 16-bit counter
#define RTIMER_CONF_CLOCK_SIZE  (2)
#define RTIMER_CONF_GUARD_TIME  (7)

typedef uint64_t clock_time_t;
typedef uint32_t uip_stats_t;

#define PLATFORM_SUPPORTS_BUTTON_HAL  (1)

// Enable/disable stack check.
// TODO: linker script doesn't define proper pointers to enable this feature
#define STACK_CHECK_CONF_ENABLED	(0)

#ifdef AT86RF2XX
#include "rf2xx_arch.h"

#define rf2xx_driver_max_payload_len    RF2XX_MAX_PAYLOAD_SIZE

//#define MAC_CONF_WITH_NULLMAC	1
#define NETSTACK_CONF_RADIO 	rf2xx_driver


#define TSCH_CONF_BASE_DRIFT_PPM    RF2XX_BASE_DRIFT_PPM    //(before it was 1000)
#define RADIO_DELAY_BEFORE_RX       RF2XX_DELAY_BEFORE_RX
#define RADIO_DELAY_BEFORE_TX       RF2XX_DELAY_BEFORE_TX
#define RADIO_DELAY_BEFORE_DETECT   RF2XX_DELAY_BEFORE_DETECT
#define RADIO_BYTE_AIR_TIME         RF2XX_BYTE_AIR_TIME
#define RADIO_PHY_OVERHEAD          RF2XX_PHY_OVERHEAD

#ifndef TSCH_CONF_DEFAULT_TIMESLOT_TIMING
#define TSCH_CONF_DEFAULT_TIMESLOT_TIMING RF2XX_CONF_DEFAULT_TIMESLOT_TIMING
#endif

//#define NULLRDC_802154_AUTOACK		1
//#define NULLRDC_802154_AUTOACK_HW	1

// Extended mode allows automatic retransmission
//#define RDC_CONF_HARDWARE_CSMA		1

// Extended mode does automatic acknowledgements
//#define RDC_CONF_HARDWARE_ACK		1

#endif

#endif
