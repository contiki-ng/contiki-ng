# Logging

Contiki-NG provides per-module, per-level logging, documented here: [doc:logging].

First, we will customize the configuration of the `hello-world` example. As described in [doc:configuration], create a file called `project-conf.h` in the project directory. In this file, define a number of log levels, such as:
```c
#define LOG_CONF_LEVEL_IPV6                        LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_RPL                         LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_6LOWPAN                     LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_TCPIP                       LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_FRAMER                      LOG_LEVEL_DBG
```

As you have added a new `project-conf.h` file, you need to clean:
```
$ make distclean
```

After compiling and logging in to the node (see [tutorial:hello-world]), you will get logs such as:
```
[INFO: RPL       ] sending a DIS to ff02::1a
[INFO: ICMPv6    ] Sending ICMPv6 packet to ::, type 155, code 0, len 2
[DBG : IPv6      ] Upper layer checksum len: 6 from: 40
[INFO: TCP/IP    ] output: sending to (NULL LL addr)
[INFO: 6LoWPAN   ] output: sending packet len 46
[INFO: 6LoWPAN   ] Compressing first header: 58
[INFO: 6LoWPAN   ] IPHC: last header could is not compressed: 58
[INFO: 6LoWPAN   ] output: header of len 4
[INFO: CSMA      ] sending to 0000.0000.0000.0000, seqno 27, queue length 1, free packets 7
[DBG : CSMA      ] scheduling transmission in 0 ticks, NB=0, BE=0
[INFO: CSMA      ] preparing packet for 0000.0000.0000.0000, seqno 27, tx 0, queue 1
[INFO: Frame 15.4] Out:  1 ffff.0000.0000.0000 14 10 (24)
[INFO: CSMA      ] tx to 0000.0000.0000.0000, seqno 27, status 0, tx 0, coll 0
[INFO: CSMA      ] packet sent to 0000.0000.0000.0000, seqno 27, status 0, tx 1, coll 0
[DBG : CSMA      ] free_queued_packet, queue length 0, free packets 8

```

This particular example shows the journey of an outgoing RPL DIS packet, down the stack. You can then edit your `project-conf.h` to set different log levels for each module (see `os/sys/log-conf.h` for a list of options).

You will see in [tutorial:shell] that it is also possible to change the log level on-the-fly using the shell.

For interpretation of the log messages, see respective module's documentation or the Contiki-NG source code. For instance, TSCH log output is described in [doc:tsch-logs].

[doc:configuration]: /doc/getting-started/The-Contiki-NG-configuration-system
[doc:logging]: /doc/getting-started/The-Contiki-NG-logging-system
[tutorial:hello-world]: /doc/tutorials/Hello,-World!
[tutorial:shell]: /doc/tutorials/Shell
[doc:tsch-logs]: /doc/programming/TSCH-and-6TiSCH
