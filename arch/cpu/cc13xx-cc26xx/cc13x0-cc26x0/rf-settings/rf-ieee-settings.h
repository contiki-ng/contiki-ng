#ifndef IEEE_SETTINGS_H_
#define IEEE_SETTINGS_H_

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_prop_cmd.h)
#include <ti/drivers/rf/RF.h>

// RF TX power table
extern RF_TxPowerTable_Entry txPowerTable[];
extern const size_t txPowerTableLen;

// TI-RTOS RF Mode Object
extern RF_Mode RF_ieeeMode;

// RF Core API commands
extern rfc_CMD_RADIO_SETUP_t RF_cmdRadioSetup;
extern rfc_CMD_FS_t RF_cmdFs;
extern rfc_CMD_IEEE_RX_t RF_cmdIeeeRx;
extern rfc_CMD_IEEE_TX_t RF_cmdIeeeTx;

// RF Core API Overrides
extern uint32_t pOverrides[];

#endif /* IEEE_SETTINGS_H_ */
