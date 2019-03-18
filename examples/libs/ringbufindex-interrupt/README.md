# ringbufindex-interrupt example

This example demonstrates and tests interrupt-safety of
lib/ringbufindex.

## What it does

It prepares a message queue managed by ringbufindex. Two producers
put some messages into the queue. One producer runs in normal
context, and the other in interrupt context (using rtimer). At the
same time, two consumers (in normal and interrupt contexts,
respectively) get messages from the queue and stores them in
dedicated memory regions.

                          put             get
       NORMAL (Producer) --+               +--> (Consumer) --> [store]
                           |               |
                           +-> --------- >-+
                               | queue |
                           +-> --------- >-+
                           |               |
    INTERRUPT (Producer) --+               +--> (Consumer) --> [store]

After all activities are finished, it checks the content of the two
message stores. If the stores do not contain the exact messages put
by the producers, it prints error messages.

After the check, it resets itself.

## Configuration

In project-conf, you can configure number of messages two produers
put to the queue and two consumers get from the queue. You can also
configure whether the program uses "peek" or "atomic" APIs to access
the queue. See ringbufindex's doc for detail about those APIs.

## Expectations

This example should run without error in the following four cases.

- The "put interrupts get" case

        NORMAL_PUT_NUM    == 0
        INTERRUPT_PUT_NUM  > 0
        NORMAL_GET_NUM     > 0
        INTERRUPT_GET_NUM == 0
        PUT_USE_PEEK      == 1
        GET_USE_PEEK      == 1

- The "get interrupts put" case

        NORMAL_PUT_NUM     > 0
        INTERRUPT_PUT_NUM == 0
        NORMAL_GET_NUM    == 0
        INTERRUPT_GET_NUM  > 0
        PUT_USE_PEEK      == 1
        GET_USE_PEEK      == 1

- The "put interrupts put/get" case

        NORMAL_PUT_NUM     > 0
        INTERRUPT_PUT_NUM  > 0
        NORMAL_GET_NUM     > 0
        INTERRUPT_GET_NUM == 0
        PUT_USE_PEEK      == 0
        GET_USE_PEEK      == 1

- The "get interrupts put/get" case

        NORMAL_PUT_NUM     > 0
        INTERRUPT_PUT_NUM == 0
        NORMAL_GET_NUM     > 0
        INTERRUPT_GET_NUM  > 0
        PUT_USE_PEEK      == 1
        GET_USE_PEEK      == 0

## Author

Toshio Ito <toshio9.ito@toshiba.co.jp>
