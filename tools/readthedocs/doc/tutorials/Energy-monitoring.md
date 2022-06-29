# Energy monitoring

> For getting started with Energest, see [doc:simple-energest]

This tutorial will show how to use the Energest module in Contiki-NG for energy monitoring. For more extensive documentation, see [doc:energest].

## The Energest module

The Energest module keeps track of when various components are turned on and off. By knowing how long components have been in different states and the power consumption of the components, it is possible to estimate the energy consumption.

The example below shows a simple application that every 10 seconds prints the time in seconds the CPU has been active, sleeping, and in deep sleep. It also prints the time in seconds the radio has been off, listening, or transmitting.

```c
#include "sys/energest.h"

static unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

PROCESS_THREAD(energest_example_process, ev, data)
{
  static struct etimer periodic_timer;
  PROCESS_BEGIN();

  etimer_set(&periodic_timer, CLOCK_SECOND * 10);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

    /* Update all energest times. */
    energest_flush();

    printf("\nEnergest:\n");
    printf(" CPU          %4lus LPM      %4lus DEEP LPM %4lus  Total time %lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
    printf(" Radio LISTEN %4lus TRANSMIT %4lus OFF      %4lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)));
  }
  PROCESS_END();
}
```

The Energest module is enabled by configuring `ENERGEST_CONF_ON` to 1. This is easiest done by adding a `project-conf.h` file to the product build with the following content.

```c
#define ENERGEST_CONF_ON 1
```

## Running the energest example

To test the Energest module you can go to the `examples/libs/energest` directory and build the example. It is possible to build for the native platform but since the native platform does not sleep, it is more interesting to run on an embedded device such as the Zolertia Firefly.

```bash
$ cd examples/libs/energest
$ make TARGET=zoul example-energest.upload
[...]
$ make TARGET=zoul login
connecting to /dev/tty.SLAB_USBtoUART (115200) [OK]
[INFO: Main      ] Starting Contiki-NG-4.0
[INFO: Main      ]  Net: sicslowpan
[INFO: Main      ]  MAC: CSMA
[INFO: Main      ] Link-layer address 0012.4b00.060d.b200
[INFO: Main      ] Tentative link-local IPv6 address fe80::212:4b00:60d:b200
[INFO: Zoul      ] Zolertia Firefly revision B platform

Energest:
 CPU             0s LPM         9s DEEP LPM    0s  Total time 10s
 Radio LISTEN   10s TRANSMIT    0s OFF         0s

Energest:
 CPU             0s LPM        19s DEEP LPM    0s  Total time 20s
 Radio LISTEN   20s TRANSMIT    0s OFF         0s
```

The output above shows that the CPU is sleeping for most of the time but the radio is kept always on in this example application.

[doc:energest]: /doc/programming/Energest
[doc:simple-energest]: /doc/tutorials/Instrumenting-Contiki-NG-applications-with-energy-usage-estimation
