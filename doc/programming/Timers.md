# Timers

Contiki-NG provides a set of timer libraries that are used both by applications and by the OS itself. The timer libraries contain functionality for checking if a time period has passed, waking up the system from low power mode at scheduled times, and real-time tasks scheduling.

All the timers build on the `clock` module, in charge of basic system time:
* `timer`: a simple timer, without built-in notification (caller must check if expired). Safe from interrupt.
* `stimer`: same as `timer`, but in seconds and with significantly longer wrapping period. Safe from interrupt.
* `etimer`: schedules events to Contiki-NG processes. Unsafe from interrupt.
* `ctimer`: schedules calls to a callback function. Unsafe from interrupt.
* `rtimer`: real-time task scheduling, with execution from ISR. Safe from interrupt.

## The Clock Module

The clock module provides functions for handling system time. The API for the clock module is shown in the following table.

| Function                              | Purpose                                      |
|---------------------------------------|----------------------------------------------|
|`clock_time_t clock_time()`            | Get the system time in `clock_time_t` ticks. |
|`unsigned long clock_seconds()`        | Get the system time in seconds.              |
|`void clock_wait(int delay)`           | Delay the CPU for a number of clock ticks.   |
|`void clock_init(void)`                | Initialize the clock module.                 |
|`CLOCK_SECOND`                         | The number of ticks per second.              |

The system time is specified as the platform dependent type `clock_time_t` and in most platforms this is a limited unsigned value which wraps around when getting to large. The system time starts from zero at boot.
In addition, `clock_wait()` blocks the CPU for a specified number of clock ticks.

## The Timer Library

The Contiki-NG timer library provides functions for setting, resetting and restarting timers, and for checking if a timer has expired. A timer is declared as a `struct timer` and all access to the timer is made by a pointer to the declared timer. The API for the Contiki-NG timer library is shown in the following table.

| Function                                                | Purpose                                          |
|---------------------------------------------------------|--------------------------------------------------|
|`void timer_set(struct timer *t, clock_time_t interval)` | Start the timer.                                 |
|`void timer_reset(struct timer *t)`                      | Restart the timer from the previous expire time. |
|`void timer_restart(struct timer *t)`                    | Restart the timer from current time.             |
|`int timer_expired(struct timer *t)`                     | Check if the timer has expired.                  |
|`clock_time_t timer_remaining(struct timer *t)`          | Get the time until the timer expires.            |

A timer is always initialized by a call to `timer_set()` which sets the timer to expire the specified delay from current time and also stores the time interval. Áll the other function operate on this delay.
The following example shows how a timer can be used to detect timeouts in an interrupt.

```c
#include "sys/timer.h"

static struct timer rxtimer;

void init(void) {
  timer_set(&rxtimer, CLOCK_SECOND / 2);
}

interrupt(UART1RX_VECTOR)
uart1_rx_interrupt(void)
{
  if(timer_expired(&rxtimer)) {
    /* Timeout */
    ...
  }
  timer_restart(&rxtimer);
  ...
}
```

## The Stimer Library

The Contiki-NG stimer library provides a timer mechanism similar to the timer library but uses time values in seconds, allowing much longer expiration times. The stimer library use `clock_seconds()` in the clock module to get the current system time in seconds.

The API for the stimer library is shown below. It is similar to the timer library, but the difference is that times are specified as seconds instead of clock ticks.

| Function                                                   | Purpose                                           |
|------------------------------------------------------------|---------------------------------------------------|
|`void stimer_set(struct stimer *t, unsigned long interval)` | Start the timer.                                  |
|`void stimer_reset(struct stimer *t)`                       | Restart the stimer from the previous expire time. |
|`void stimer_restart(struct stimer *t)`                     | Restart the stimer from current time.             |
|`int stimer_expired(struct stimer *t)`                      | Check if the stimer has expired.                  |
|`unsigned long stimer_remaining(struct stimer *t)`          | Get the time until the timer expires.             |

The stimer library can safely be used from interrupts.

## The Etimer Library

The Contiki-NG etimer library provides a timer mechanism that generate timed events. An event timer will post the event `PROCESS_EVENT_TIMER` to the process that set the timer when the event timer expires. The etimer library use `clock_time` in the clock module to get the current system time.

An event timer is declared as a `struct etimer` and all access to the event timer is made by a pointer to the declared event timer.

The API for the etimer library is shown in the following table.

| Function                                                  | Purpose                                                      |
|-----------------------------------------------------------|--------------------------------------------------------------|
|`void etimer_set(struct etimer *t, clock_time_t interval)` | Start the timer.                                             |
|`void etimer_reset(struct etimer *t)`                      | Restart the timer from the previous expire time.             |
|`void etimer_restart(struct etimer *t)`                    | Restart the timer from current time.                         |
|`void etimer_stop(struct etimer *t)`                       | Stop the timer.                                              |
|`int etimer_expired(struct etimer *t)`                     | Check if the timer has expired.                              |
|`int etimer_pending()`                                     | Check if there are any non-expired event timers.             |
|`clock_time_t etimer_next_expiration_time()`               | Get the next event timer expiration time.                    |
|`void etimer_request_poll()`                               | Inform the etimer library that the system clock has changed. |

Note that the timer event is sent to the Contiki-NG process used to schedule the event timer. If an event timer should be scheduled from a callback function or another process, `PROCESS_CONTEXT_BEGIN()` and `PROCESS_CONTEXT_END()` can be used to temporary change the process context.

The following example shows how an etimer can be used to schedule a process to run once per second.

```c
#include "sys/etimer.h"

PROCESS_THREAD(example_process, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();

  /* Delay 1 second */
  etimer_set(&et, CLOCK_SECOND);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    /* Reset the etimer to trig again in 1 second */
    etimer_reset(&et);
    ...
  }
  PROCESS_END();
}
```

## The Ctimer Library

The Contiki-NG ctimer library provides a timer mechanism that calls a specified function when a callback timer expires. The ctimer library use `clock_time()` in the clock module to get the current system time.

The API for the ctimer library is shown below.

|Function                                | Purpose                                                                   |
|----------------------------------------|---------------------------------------------------------------------------|
|`void ctimer_set(struct ctimer *t, clock_time_t interval, void (*callback)(void *), void *ptr)` | Start the timer. |
|`void ctimer_reset(struct ctimer *t)`   | Restart the timer from the previous expire time.                          |
|`void ctimer_restart(struct ctimer *t)` | Restart the timer from current time.                                      |
|`void ctimer_stop(struct ctimer *t)`    | Stop the timer.                                                           |
|`int ctimer_expired(struct ctimer *t)`  | Check if the timer has expired.                                           |

This API is similar to the etimer library, with the main difference being that `ctimer_set()` takes a callback function pointer and a data pointer as arguments. When a ctimer expires, it will call the callback function with the data pointer as argument.

The example below shows a how a ctimer can be used to schedule a callback to a function once per second.

```c
#include "sys/ctimer.h"
static struct ctimer timer;

static void
callback(void *ptr)
{
  ctimer_reset(&timer);
  ...
}

void
init(void)
{
  ctimer_set(&timer, CLOCK_SECOND, callback, NULL);
}
```

Note that although the callback timers are calling a specified callback function, the process context for the callback is set to the process used to schedule the ctimer. Do not assume any specific process context in the callback unless you are sure about how the callback timers are scheduled.

## The Rtimer Library

The Contiki-NG rtimer library provides scheduling and execution of real-time tasks. The rtimer library uses its own clock module for scheduling to allow higher clock resolution. The macro `RTIMER_NOW()` is used to get the current system time in ticks and `RTIMER_SECOND` specifies the number of ticks per second.

Unlike the other timer libraries in Contiki-NG, the real-time tasks pre-empt normal execution for the task to execute immediately. This sets some constraints for what can be done in real-time tasks because most functions do not handle preemption. Interrupt-safe functions such as `process_poll()` are always safe to use in real-time tasks but anything that might conflict with normal execution must be synchronized.

**Contiki-NG currently supports only one active rtimer.** Among other things it means that if you use system functionality that has its own rtimer (for example, the TSCH stack), you will not be able to have rtimers at the application level.

The API for the rtimer library is shown in the following table.

| Function | Purpose |
|------------------------------------------------|-------------------------------------------------------------------------|
|`void rtimer_set(struct rtimer *task, timer_clock_t time, rtimer_clock_t duration, rtimer_callback_t func, void *ptr)` | Setup a real-time task. |
|`RTIMER_NOW()`                                  | Get the current time. |
|`RTIMER_CLOCK_LT(t0,t1)`                        | Check if the time `t0` is less than the time `t1`. |
|`RTIMER_SECOND`                                 | The number of ticks per second. |
|`void rtimer_init(void)`                        | Initialize the rtimer library. |
|`void rtimer_run_next(void)`                    | Called by the rtimer scheduler to run next real-time task.              |

A real time task is always initialized by a call to the function `rtimer_set()`, which sets the delay (`time`) callback function pointer, and data pointer. The `duration` field is currently unused.
The following example shows how a real-time task can be setup to execute four times per second.

```c
#include "sys/rtimer.h"

static struct rtimer task;

static void
callback(struct rtimer *t, void *ptr, int status)
{
  if(rtimer_reschedule(&task, RTIMER_SECOND / 4, DURATION) != RTIMER_OK) {
    /* Failed to reschedule timer. Recover by rescheduling from current time. */
    rtimer_schedule(&task, RTIMER_SECOND / 4, DURATION);
  }
  ...
}

void
init(void)
{
  rtimer_setup(&task, RTIMER_HARD, callback, NULL);
  rtimer_schedule(&task, RTIMER_SECOND / 4, DURATION);
}
```
