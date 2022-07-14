# Synchronization primitives

Contiki-NG code can be categorised based on the context within which it is being executed:
* Code always running outside an interrupt context; let's call this the "main" (or "main thread") context,
* Code that (always or sometimes) runs inside an interrupt context.

When a shared resource is only ever accessed from within the main thread context, developers do not need to rely on any synchronization primitives since read/write operations to the resource will not be unexpectedly interrupted by the scheduler.

Things are more complicated when a shared resource can be accessed from within as well as from outside an interrupt context. In this case, developers need to make sure that the state of the resource remains consistent. To this end, Contiki-NG provides some fundamental synchronization primitives documented here.

More details about Contiki-NG's multitasking model, process scheduler and contexts in [doc:multitasking].

## Global Interrupt Manipulation
Contiki-NG provides an API that allows enabling/disabling/restoring the global interrupt. Disabling the global interrupt should be avoided where possible, and in any case should only ever be done for very brief periods of time.

To disable the global interrupt, use the function below:

```C
int_master_status_t int_master_read_and_disable(void);
```

The return value of this function indicates the state of the global interrupt before it was disabled. The semantics of this return value are platform-dependent and its value should not be used to interpret the state of the global interrupt in a cross-platform fashion. This return value should be stored in a variable which should then be used to restore the global interrupt to its previous state by using this function:

```C
void int_master_status_set(int_master_status_t status);
```

## Data Memory Barriers
Contiki-NG provides an API for the insertion of compiler/CPU memory barriers (`memory_barrier()`), which can then be used to implement critical sections. It is the CPU code developer's responsibility to implement insertion of memory barriers.

## Critical sections
Contiki-NG provides platform/CPU-independent functions for entering / exiting critical code sections. These functions rely on the implementation of the global interrupt manipulation API above.

## Mutexes
Contiki-NG features a platform-independent API for the manipulation of mutexes.

Mutexes can be implemented using CPU-specific synchronisation instructions, which is for example done for all supported Cortex-M3/M4-based CPUs. Where a CPU-specific implementation is not provided, the library will fall back to a default implementation which manipulates a mutex within a critical section and which therefore relies on disabling the global interrupt for a brief period.

## Atomic compare-and-swap
Contiki-NG provides an API for atomic compare-and-swap (CAS) operations on 8-bit variables. Atomic 8-bit CAS can be implemented using CPU-specific synchronisation instructions in a fashion very similar to mutexes. Where a CPU-specific implementation is not provided, the library will fall back to a default implementation which performs the CAS operation within a critical section and which therefore relies on disabling the global interrupt for a brief period.

[doc:multitasking]: /doc/programming/Multitasking-and-scheduling
