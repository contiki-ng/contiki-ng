#ifndef NRF52_H_
#define NRF52_H_

#include "dev/radio.h"

extern const struct radio_driver nrf52840_driver;

// TODO TSCH
/* Delay between GO signal and SFD */
#define RADIO_DELAY_BEFORE_TX ((unsigned)US_TO_RTIMERTICKS(RADIO_PHY_HEADER_LEN * RADIO_BYTE_AIR_TIME))
/* Delay between GO signal and start listening.
 * This value is so small because the radio is constantly on within each timeslot. */
#define RADIO_DELAY_BEFORE_RX ((unsigned)US_TO_RTIMERTICKS(15))
/* Delay between the SFD finishes arriving and it is detected in software. */
#define RADIO_DELAY_BEFORE_DETECT ((unsigned)US_TO_RTIMERTICKS(352))

// CHECK RTIMERTICS_TO_US and US_TO_RTIMERTICKS

#endif /* NRF52_H_ */
