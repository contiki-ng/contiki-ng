# Multitasking and scheduling

Contiki-NG adopts the original Contiki's event-driven model with a cooperative multitasking style. Support for preemptive multithreading has been removed, which makes Contiki-NG strictly non-preemptive.

## Processes and Events
Applications in Contiki-NG are typically written by using the _Process_ abstraction. Processes are built on top of a lightweight threading library called [Protothreads](http://dl.acm.org/citation.cfm?id=1182811).

In a typical execution scenario, a process (or protothread) will idle until it receives an event. Upon event reception, the process will consume it by performing a corresponding chunk of work and it will then suspend its own execution until it receives the next event. Some example events are:

* The expiration of a timer,
* The reception of an incoming network packet,
* The reception of a message over the serial line,
* A user action, such as a button press,
* A sensor driver indicating that a new reading is available,
* A user-defined event.

Since the Contiki-NG scheduler is cooperative, it will never force a context switch between a running process (or protothread) to another. Therefore, for things to work correctly each running protothread needs to voluntarily return control to the scheduler once it has completed its task. It is thus important for developers to make sure that processes do not keep control for too much time and that long operations are split into multiple process schedulings, allowing such operations to resume at the point where they last stopped.

More information on how to implement processes and events can be found in [doc:processes].

## The Contiki-NG Scheduler and Event Dispatch

The Contiki-NG model uses two types of events: Asynchronous and Synchronous.

* Asynchronous events are put in a queue and get dispatched to the receiving process(es) in a round-robin fashion. An asynchronous event can be posted to a specific process, or it can be broadcast. In the latter case, the kernel will dispatch the event to all processes. There is no way to specify the order in which processes will receive a broadcast event.
* Synchronous events cause the receiving process to get scheduled immediately. This will normally cause the posting process' execution to stop until the receiving process has finished consuming the event, at which point the posting process resumes. Synchronous events cannot be broadcast.

Contiki-NG also supports a polling mechanism. A _poll_ is a specific type of high-priority, asynchronous event dispatched to a single receiving process (cannot be broadcast).

The kernel will schedule all polled processes before, as well as in-between asynchronous event dispatches. In the case of a broadcast event, polling will also happen between the dispatch of the event to two consecutive processes.

## Interrupts and contexts
The non-preemptive, cooperative nature of the Contiki-NG multitasking model means that a process will never get interrupted unexpectedly by the scheduler. However, the execution of a process can (and very often will) get interrupted unexpectedly by hardware interrupt handlers. An interrupt handler itself can be pre-empted by the handler of an interrupt of a higher priority.

With this in mind, Contiki-NG code can be categorised based on the context within which it is being executed:

* Code always running outside an interrupt context; let's call this the "main" (or "main thread") context,
* Code that (always or sometimes) runs inside an interrupt context.

Code that always runs in the main context is much simpler to implement, for example functions do not need to be re-entrant.

### Access to Shared Resources
Synchronising access to shared resources heavily depends on which parts of the code access them.

When a shared resource is only ever accessed from within the main thread context, developers do not need to rely on any synchronization primitives since read/write operations to the resource will not be unexpectedly interrupted by the scheduler.

Things are more complicated when a shared resource can be accessed from within as well as from outside an interrupt context. In this case, developers need to make sure that the state of the resource remains consistent. To this end, Contiki-NG provides some fundamental synchronization primitives (such as mutexes and critical sections) documented in detail in [doc:synch-primitives] as well as in the [online API documentation](https://contiki-ng.readthedocs.io/).

### Writing interrupt handlers
With all of the above in mind, extra care needs to be taken when developing code that may run inside an interrupt context. Interrupt handler developers need to keep in mind that the following Contiki-NG system and library functions are *not* safe to run within an interrupt context:

* Posting events: `process_post()` and `process_post_synch()` are not safe to call within an interrupt context. Where an interrupt handler needs to result in a process being scheduled, it should use the polling mechanism instead by calling `process_poll()`.
* Some data structure manipulation libraries are not safe to use within an interrupt context. This includes:
  * The main linked list library (`list.[ch]`),
  * The queue library (`queue.h`),
  * The stack library (`stack.h`),
  * The circular linked list library (`circular-list.[ch]`),
  * The doubly-linked list library (`dbl-list.[ch]`),
  * The circular, doubly-linked list library (`dbl-circ-list.[ch]`).
* Scheduling timers: The event timer (`etimer`), callback timer (`ctimer`) and trickle timer (`trickle-timer`) libraries rely on list manipulation. For that reason, manipulating event and callback timers within an interrupt context is not safe.
* Neighbour table manipulation: All functions in `nbr.[hc]`
* Watchdog timers should never be refreshed within an interrupt context: Calling `watchdog_periodic()` from inside an interrupt could result in a situation whereby a device never recovers from a firmware crash because its WDT is being refreshed frequently enough within an interrupt.

[doc:processes]: /doc/programming/Processes-and-events
[doc:synch-primitives]: /doc/programming/Synchronization-primitives
