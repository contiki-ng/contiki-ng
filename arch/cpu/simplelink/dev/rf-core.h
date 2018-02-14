#ifndef CONTIKI_NG_ARCH_CPU_SIMPLELINK_DEV_RF_CORE_H_
#define CONTIKI_NG_ARCH_CPU_SIMPLELINK_DEV_RF_CORE_H_

/*---------------------------------------------------------------------------*/
/* Contiki API */
#include <sys/rtimer.h>
#include <dev/radio.h>
/*---------------------------------------------------------------------------*/
#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
/* Standard library */
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#ifdef RF_CORE_CONF_CHANNEL
#   define RF_CORE_CHANNEL  RF_CORE_CONF_CHANNEL
#else
#   define RF_CORE_CHANNEL  25
#endif
/*---------------------------------------------------------------------------*/
typedef enum {
    CMD_ERROR = 0,
    CMD_OK = 1,
} CmdResult;
/*---------------------------------------------------------------------------*/
typedef struct {
  radio_value_t dbm;
  uint16_t      power; /* Value for the PROP_DIV_RADIO_SETUP.txPower field */
} RF_TxPower;

#define TX_POWER_UNKNOWN  0xFFFF
/*---------------------------------------------------------------------------*/
#define RSSI_UNKNOWN      -128
/*---------------------------------------------------------------------------*/
PROCESS_NAME(RF_coreProcess);
/*---------------------------------------------------------------------------*/

#endif /* CONTIKI_NG_ARCH_CPU_SIMPLELINK_DEV_RF_CORE_H_ */
