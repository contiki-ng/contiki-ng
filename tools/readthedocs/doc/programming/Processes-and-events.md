# Processes and events

Applications in Contiki-NG are typically written by using the _Process_ abstraction. Processes are built on top of a lightweight threading library called [Protothreads](http://dl.acm.org/citation.cfm?id=1182811).

## Process Definition

A process is first declared at the top of a source file. The `PROCESS()` macro takes two arguments: one is the variable with which we identify the process, and the other is the name of the process. On the second line, we tell Contiki-NG that this process should be automatically started directly after the system has booted up. Multiple processes can be specified here by separating them with commas. If the `AUTOSTART_PROCESSES()` line does not include an existing process, then that process has to be started manually by using the `process_start()` function.

```c
#include "contiki.h" /* Main include file for OS-specific modules. */
#include <stdio.h> /* For printf. */

PROCESS(test_proc, "Test process");
AUTOSTART_PROCESSES(&test_proc);
```

A basic process can then be implemented as follows.

```c
PROCESS_THREAD(test_proc, ev, data)
{
  PROCESS_BEGIN();

  printf("Hello, world!\n");

  PROCESS_END();
}
```

The `PROCESS_THREAD()` macro first takes the identifier of the processes specified in the `PROCESS()` call. The `ev` and `data` arguments contain the value of an incoming event, and an optional pointer to an event argument object. The `PROCESS_BEGIN()` marks the place where the process execution will start. In most cases, programmers should avoid placing code above this statement in the `PROCESS_THREAD()` body, except for variable definitions.

For our basic process implementation, we do not have to be concerned with any event handling, as we are only executing a single statement before implicitly exiting the process by letting it reach the `PROCESS_END()` call.

## Events and Scheduling

Contiki-NG is built on an event-based execution model, where processes typically perform chunks of work before telling the scheduler that they are waiting for an event, and thereby suspend execution. Such events can be the expiration of a timer, an incoming network packet, or a serial line message being delivered.

Processes are scheduled _cooperatively_, which means that each process is responsible for voluntarily yielding control back to the operating system without executing for too long. Hence, the application developer must make sure that long-running operations are split into multiple process schedulings, allowing such operations to resume at the point where they last stopped.

### Waiting for events

By calling `PROCESS_WAIT_EVENT_UNTIL()` in the area separated by the `PROCESS_BEGIN()` and `PROCESS_END()` calls, one can yield control to the scheduler, and only resume execution when an event is delivered. A condition is given as an argument to `PROCESS_WAIT_EVENT_UNTIL()`, and this condition must be fulfilled for the processes to continue execution after the call to `PROCESS_WAIT_EVENT_UNTIL()`. If the condition is not fulfilled, the process yields control back to the OS until a new event is delivered.

```c
PROCESS_THREAD(test, ev, data)
{
  /* An event-timer variable. Note that this variable must be static
     in order to preserve the value across yielding. */
  static struct etimer et;

  PROCESS_BEGIN();

  etimer_set(&et, CLOCK_SECOND); /* Trigger a timer after 1 second. */
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    etimer_reset(&et);
  }

  PROCESS_END();
}

```

### Pausing and yielding

To voluntarily release control the scheduler, one can call `PROCESS_PAUSE()`, as in the example below. The scheduler will then deliver any queued events, and then immediately schedule the paused process.

```c
PROCESS_THREAD(test_proc, ev, data)
{
  PROCESS_BEGIN();

  while(have_operations_to_do()) {
    do_some_operations();
    PROCESS_PAUSE();
  }

  PROCESS_END();
}
```

By contrast, `PROCESS_YIELD()` will yield control back to the scheduler without expecting to be scheduled again shortly thereafter. Instead, it will wait for an incoming event, similar to `PROCESS_WAIT_EVENT_UNTIL()`, but without a required condition argument. A process that has yielded can be polled by an external process or module by calling `process_poll()`.
To poll a process declared with the variable `test_proc`, one can call `process_poll(&test_proc);`. The polled process will be scheduled immediately, and a `PROCESS_EVENT_POLL` event will be delivered to it.

### Stopping processes

A process can be stopped in three ways:
1. The process implicitly exits by allowing the `PROCESS_END()` statement to be reached and executed.
2. The process explicitly exits by calling `PROCESS_EXIT()` in the `PROCESS_THREAD` body.
3. Another process kills the process by calling `process_exit()`.

After stopping a process, it can be restarted  from the beginning by calling `process_start()`.

### System-defined Events

Contiki-NG uses a based of system-defined events for common operations, as listed below.

|Event                         | ID   | Description                                            |
|------------------------------|------|--------------------------------------------------------|
|PROCESS_EVENT_NONE            | 0x80 | No event.                                              |
|PROCESS_EVENT_INIT            | 0x81 | Delivered to a process when it is started.             |
|PROCESS_EVENT_POLL            | 0x82 | Delivered to a process being polled.                   |
|PROCESS_EVENT_EXIT            | 0x83 | Delivered to an exiting a process.                     |
|PROCESS_EVENT_SERVICE_REMOVED | 0x84 | Unused.                                                |
|PROCESS_EVENT_CONTINUE        | 0x85 | Delivered to a paused process when resuming execution. |
|PROCESS_EVENT_MSG             | 0x86 | Delivered to a process upon a sensor event.            |
|PROCESS_EVENT_EXITED          | 0x87 | Delivered to all processes about an exited process.    |
|PROCESS_EVENT_TIMER           | 0x88 | Delivered to a process when one of its timers expired. |
|PROCESS_EVENT_COM             | 0x89 | Unused.                                                |
|PROCESS_EVENT_MAX             | 0x8a | The maximum number of the system-defined events.       |

### User-defined Events

One can also specify new events that are not covered in the system-defined events. The event variable should be defined and declared with an appropriate scope, so that all expected users of the event can access it. The basic case is to have it used only within a single module, and then it can be defined as `static` at the top of the module.

```c
static process_event_t my_app_event;
[...]
my_app_event = process_alloc_event();
```

Note that there is no support for deallocating events.
