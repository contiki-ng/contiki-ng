#ifndef RTIMER_ARCH_H_
#define RTIMER_ARCH_H_

#include "sys/rtimer.h"

/*
If radio is on ISMTV board: rtimer has 65.533 kHz --> RTIMER_ARCH_SECOND = 65533
Drift calculation:
    Slot length 10000 usec, which gives us 655 ticks (from macro US_TO_RTIMERTICKS)
    Tick duration: 15.2594875864 us
    Real slot duration is then : 9994.96436 usec
    Target - real duration = 5.03564 us
    RTIMER_ARCH_DRIFT_PPM = 504
 
If radio is not on ISMTV board: rtimer has 65.503 kHz --> RTIMER_ARCH_SECOND = 65503
Drift calculation:
    Slot length 10000 usec, which gives us 655 ticks (from macro US_TO_RTIMERTICKS)
    Tick duration: 15.2664763445 us
    Real slot duration is then : 9999.54200 usec
    Target - real duration = 0.4580 us
    RTIMER_ARCH_DRIFT_PPM = 46
 */

#if (AT86RF2XX_BOARD_ISMTV_V1_0 || AT86RF2XX_BOARD_ISMTV_V1_1)
    #define RTIMER_ARCH_SECOND      (65533)
    #define RTIMER_ARCH_DRIFT_PPM   (503) 
#else
    #define RTIMER_ARCH_SECOND		(65503)
    #define RTIMER_ARCH_DRIFT_PPM   (46) 
#endif


// Converts micro seconds to rtimer ticks (65536 ticks/s ---> 1us = 0.065536 tick)
// Eqn.: T = (us * 0.065536) +- 1/2
#define US_TO_RTIMERTICKS(us)   ((us) >= 0 ? \
                                (uint32_t)(((((int64_t)(us)) * (RTIMER_ARCH_SECOND)) + 500000) / 1000000L) : \
                                (uint32_t)(((((int64_t)(us)) * (RTIMER_ARCH_SECOND)) - 500000) / 1000000L)) 

// Converts rtimer ticks to micro seconds (1/65536 Hz ---> 1 tick ~ 15.25879 us)
// Eqn.: us = (T * 15.258) +- 1/2
#define RTIMERTICKS_TO_US(t)    ((t) >= 0? \
                                ((((int32_t)(t)) * 1000000L + ((RTIMER_ARCH_SECOND) / 2)) / (RTIMER_ARCH_SECOND)) : \
                                ((((int32_t)(t)) * 1000000L - ((RTIMER_ARCH_SECOND) / 2)) / (RTIMER_ARCH_SECOND)))

// Converts rtimer ticks to micro-seconds (64-bit version) because the 32-bit one cannot handle T >= 4294 ticks.
// Intended only for positive values of t
#define RTIMERTICKS_TO_US_64(t) ((uint32_t)(((int64_t)(t)) * 1000000L + ((RTIMER_ARCH_SECOND) / 2)) / (RTIMER_ARCH_SECOND)) 


void contiki_rtimer_isr(void);
rtimer_clock_t rtimer_arch_now(void);

#endif
