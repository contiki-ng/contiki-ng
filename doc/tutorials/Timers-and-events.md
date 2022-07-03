# Timers and events

This tutorial will show how to make use of timers in Contiki-NG. It will also give a basic into to events. For more extensive documentation on these aspects, see [doc:timers].

## The etimer
The etimer (Event timer) will post an event when the timer is expiring. Since it posts an event you will
need to have a process around it to handle the event.

```c
PROCESS_THREAD(etimer_process, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();
  /* set timer to expire after 5 seconds */
  etimer_set(&et, CLOCK_SECOND * 5);
  while(1) {
    PROCESS_WAIT_EVENT(); /* Same thing as PROCESS_YIELD */
    if(etimer_expired(&et)) {
      /* Do the work here and restart timer to get it periodic !!! */
      printf("etimer expired.\n"); 
      etimer_restart(&et);
    }
  }
  PROCESS_END();
}
```

In the above code the process is defined by `PROCESS_THREAD` which takes a name, an event variable
and an event data variable as the arguments. When an etimer expires the event variable will be set
to `PROCESS_EVENT_TIMER` and the event data variable will be set to point to the specific timer.
Using that information the above example could also look like:

```c
PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && data == &et);
/* Do the work here and restart timer to get it periodic! */
printf("etimer expired.\n"); 
etimer_restart(&et);
```
 
But when waiting for this event in this way -- all other events will be ignored so
the first approach is more flexible as this will enable handling multiple types of
events more easily. 

## The ctimer 
The ctimer (callback timer) is very similar to the etimer but instead of posting an event it will call a callback
function. This means that there is no need to set-up a process.

First -- somewhere in the code -- just set the ctimer's timeout and its callback (plus a pointer to
data if you have any data that the timer needs a pointer to when being called -- `NULL` -- in the below example):
     
```c
ctimer_set(&timer_ctimer, CLOCK_SECOND, ctimer_callback, NULL);
```

Then somewhere in the code you also need to define the callback function:

```c
void
ctimer_callback(void *ptr)
{
  /* rearm the ctimer */
  ctimer_reset(&timer_ctimer);
  printf("CTimer callback called\n");
}
```

This example function will just print that the timer callback was called and reset the timer - which will
set it to trigger again after the same timeout as before -- e.g. one second (`CLOCK_SECOND`).

## Running the timer example
To try out all the timers available in Contiki-NG you can go to the `examples/libs/timers` directory and
build the example. If you build it for the native platform you will be able to run it on your local laptop
or desktop computer.

```bash
$ cd examples/libs/timers
$ make
$ ./all-timers.native
[INFO: Main      ] Starting Contiki-NG-4.0
[INFO: Main      ]  Net: tun6
[INFO: Main      ]  MAC: nullmac
[WARN: Tun6      ] Failed to open tun device (you may be lacking permission). Running without network.
[INFO: Main      ] Link-layer address 0102.0304.0506.0708
[INFO: Main      ] Tentative link-local IPv6 address fe80::302:304:506:708
RTimer callback called
CTimer callback called
RTimer callback called
RTimer callback called
```

[doc:timers]: /doc/programming/Timers