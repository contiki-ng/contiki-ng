#include "contiki.h"
#include "net/mac/tsch/tsch.h"


const tsch_timeslot_timing_usec tsch_timeslot_timing_rf2xx_10000us_250kbps = {
   1800, // CCAOffset 
    128, // CCA 
   2120, // TxOffset 
  (2120 - (TSCH_CONF_RX_WAIT / 2)), // RxOffset 
    800, // RxAckDelay 
   1600, // TxAckDelay (default: 1000)
  TSCH_CONF_RX_WAIT, // RxWait 
   2600, // AckWait (default: 400)
    192, // RxTx 
   3000, // MaxAck (default: 2400)
   4256, // MaxTx 
  10000, // TimeslotLength 
};

