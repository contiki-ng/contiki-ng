# Energest

> See also: getting started with the Simple Energest system service at [tutorial:simple-energest]

The [Energest](https://dl.acm.org/citation.cfm?id=1278979) module can be used to implement lightweight, software-based energy estimation approach for resource-constrained IoT devices. By tracking the time various hardware components such as the radio is turned on, and by knowing the power consumption of the component, it is possible to estimate the energy consumption.

## The Energest Module

The Energest module provides functions for keeping track when components are on, off or in different modes. The API for the Energest module is shown in the following table.

| Function                                           | Purpose                                                                   |
|----------------------------------------------------|---------------------------------------------------------------------------|
|`ENERGEST_TIME_T ENERGEST_CURRENT_TIME()`           | Get the system time from the Energest time source.                        |
|`ENERGEST_SECOND`                                   | The number of ticks per second from the Energest time source.             |
|`ENERGEST_ON(type)`                                 | Set the specified type to on and start tracking the time.       |
|`ENERGEST_OFF(type)`                                | Set the specified type to off and update the total time.        |
|`ENERGEST_SWITCH(type_off, type_on)`                | Switch from tracking time for the first type to the second type. |
|`ENERGEST_GET_TOTAL_TIME()`                         | Get the total time Energest has been tracking.                  |
|`void energest_flush(void)`                         | Flush the Energest times for all components that are currently turned on. |
|`uint64_t energest_type_time(energest_type_t type)` | Read the total time the specified type has been turned on.                |
|`void energest_init(void)`                          | Initialize the Energest module.                                           |

The macro `ENERGEST_CURRENT_TIME` returns a platform dependent system time for use by the Energest module and the constant `ENERGEST_SECOND` specifies the number of ticks per second.

The macros `ENERGEST_ON`, `ENERGEST_OFF`, and `ENERGEST_SWITCH` are used to tell the Energest module when a component changes mode and is normally only called by platforms and device drivers. When a component switches from one mode to another, it is recommended to use `ENERGEST_SWITCH` as it provides better accuracy.

The macro `ENERGEST_GET_TOTAL_TIME()` returns the total time the Energest module has been tracking. This is normally the same as `CPU + LPM + DEEP_LPM` but can be overridden by platforms that provide additional Energest types.

The function `energest_flush` updates the total time for all types that are currently turned on.

The function `energest_type_time` returns the total time the specified type has been turned on. `energy_flush` should be called before reading the total time to ensure the total time has been updated.

The function `energest_init()` is called by the system during the boot-up procedure to initialize the Energest module.


## The Energest types

There are five Energest types predefined that all Contiki-NG platforms should support.

| Type                    | Purpose                            |
|-------------------------|------------------------------------|
|`ENERGEST_TYPE_CPU`      | The CPU is active.                 |
|`ENERGEST_TYPE_LPM`      | The CPU is in low power mode.      |
|`ENERGEST_TYPE_DEEP_LPM` | The CPU is in deep low power mode. |
|`ENERGEST_TYPE_TRANSMIT` | The radio is transmitting.         |
|`ENERGEST_TYPE_LISTEN`   | The radio is listening.            |

The first three are used to track the different modes of the CPU. Not all CPUs support both LPM and deep LPM and in that case, the unused type will always report 0 as time. The latter two types are used to track when the radio is turned on and the state of the radio.

### Adding new Energest types in a platform

A platform can provide additional Energest types by defining `ENERGEST_CONF_PLATFORM_ADDITIONS` in `contiki-conf.h`. It is up to the platform to ensure that `ENERGEST_ON`/`ENERGEST_OFF`/`ENERGEST_SWITCH` is called at appropriate times for the new Energest types.

```c
#define ENERGEST_CONF_PLATFORM_ADDITIONS ENERGEST_TYPE_COMPONENT1,ENERGEST_TYPE_COMPONENT2
```

### Adding new Energest types in an application

An application can provide additional Energest types by defining `ENERGEST_CONF_ADDITIONS` in `project-conf.h`. It is up to the application to ensure that `ENERGEST_ON`/`ENERGEST_OFF`/`ENERGEST_SWITCH` is called at appropriate times for the new Energest types.

```c
#define ENERGEST_CONF_ADDITIONS ENERGEST_TYPE_COMPONENT3,ENERGEST_TYPE_COMPONENT4
```

## Platform configuration for Energest

By default, the Energest module will use the [the Rtimer Library](Timers.md#the-rtimer-library) clock as time source but a platform might provide a different time source. The example below configures the Energest module to use the Contiki-NG system clock. Note that if the time source has too low resolution, it might not be able to track quick mode changes that happens faster than a few clock ticks.

```c
#define ENERGEST_CONF_CURRENT_TIME clock_time
#define ENERGEST_CONF_TIME_T clock_time_t
#define ENERGEST_CONF_SECOND CLOCK_SECOND
```

## Using the Energest Module

For an example how to use the Contiki-NG system service called use Simple Energest, see [tutorial:simple-energest].

If need more features than the Simple Energest can provide, see [tutorial:energest] for an example how to directly use Energest from an application.

[tutorial:simple-energest]: /doc/tutorials/Energy-monitoring
[tutorial:energest]: /doc/tutorials/Energy-monitoring
