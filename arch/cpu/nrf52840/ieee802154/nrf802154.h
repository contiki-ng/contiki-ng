#ifndef NRF52_H_
#define NRF52_H_

#include "dev/radio.h"

extern const struct radio_driver nrf52840_driver;

// TSCH DEFINES
/* 1 len byte, 2 bytes CRC */
#define RADIO_PHY_OVERHEAD         3
/* 250kbps data rate. One byte = 32us */
#define RADIO_BYTE_AIR_TIME       32
/* Delay between GO signal and SFD */
#define NRF52_DELAY_BEFORE_TX ((unsigned)US_TO_RTIMERTICKS(/*RADIO_PHY_HEADER_LEN * RADIO_BYTE_AIR_TIME +*/ 4))
/* Delay between GO signal and start listening.
 * This value is so small because the radio is constantly on within each timeslot. */
#define NRF52_DELAY_BEFORE_RX ((unsigned)US_TO_RTIMERTICKS(4))
/* Delay between the SFD finishes arriving and it is detected in software. */
#define NRF52_DELAY_BEFORE_DETECT ((unsigned)US_TO_RTIMERTICKS(4))

#endif /* NRF52_H_ */
