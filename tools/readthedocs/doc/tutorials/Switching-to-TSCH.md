# Switching to TSCH

By default, Contiki-NG examples use the CSMA always-on MAC protocol. This tutorial lists the steps necessary to upgrade an example application to the [TSCH](/doc/programming/TSCH-and-6TiSCH) MAC protocol.

We assume that you are familiar with the basics on Contiki-NG application development, having followed the [Hello world](/doc/tutorials/Hello,-World!) and [Contiki-NG shell](/doc/tutorials/Shell) tutorials. This tutorial is an alternative version of the [IPv6 ping](/doc/tutorials/IPv6-ping) tutorial that uses CSMA instead of TSCH.

## Configuration

Navigate to an example application. Here I'm going to use the "Hello world" example.

    $ cd examples/hello-world

Edit the application `Makefile` to add support for TSCH and the Contiki-NG shell:

    MAKE_MAC = MAKE_MAC_TSCH
    MODULES += os/services/shell

We suggest also creating a `project-conf.h` file and configuring more verbose logging than enabled by default. This is not strictly necessary, but will help with monitoring and debugging. Add the following lines to the file and save it as `examples/hello-world/project-conf.h`:
```
#define LOG_CONF_LEVEL_RPL                         LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_TCPIP                       LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_IPV6                        LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_6LOWPAN                     LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_FRAMER                      LOG_LEVEL_WARN
#define TSCH_LOG_CONF_PER_SLOT                     0
```

Now select the platform and board to use. This depends on your hardware; for the sake of the example, let's use the `cc26x0-cc13x0` platform and Texas Instruments CC1310 Launchpad as the board:

    $ make savetarget TARGET=cc26x0-cc13x0 BOARD=launchpad/cc1310

Clean, build, and flash the application:

    $ make clean
    $ make -j
    $ make hello-world.upload PORT=/dev/ttyACM0

Log in to one node and set it as RPL root by typing the command `rpl-set-root 1`. This will automatically set is as TSCH coordinator as well:
```
$ make login PORT=/dev/ttyACM0
[INFO: TSCH      ] scanning on channel 15
[INFO: TSCH      ] scanning on channel 25
[WARN: TSCH      ] Cannot compute max payload size: not associated
[WARN: 6LoWPAN   ] output: failed to calculate payload size - dropping packet
[INFO: TSCH      ] scanning on channel 26
[INFO: TSCH      ] scanning on channel 20
rpl-set-root 1
[INFO: TSCH      ] scanning on channel 15
Setting as DAG root with prefix fd00::/64
#0012.4b00.0e07.d5a5> [INFO: TSCH Sched] add_slotframe 0 7
[INFO: TSCH Sched] add_link sf=0 opt=Tx|Rx|Sh type=ADV ts=0 ch=0 addr=ffff.ffff.ffff.ffff
[INFO: TSCH      ] starting as coordinator, PAN ID abcd, asn-0.0
[INFO: TSCH      ] TSCH: enqueue EB packet 35 16
[INFO: TSCH      ] packet sent to 0000.0000.0000.0000, seqno 186, status 0, tx 1
```

To make it easier to notice important things, you can now reduce the logging level by using the `log` command:
```
#0012.4b00.0e07.d5a5> log mac 2
Log levels:
-- rpl       : 2 (Warnings)
-- tcpip     : 2 (Warnings)
-- ipv6      : 2 (Warnings)
-- 6lowpan   : 2 (Warnings)
-- nullnet   : 0 (None)
-- mac       : 2 (Warnings)
-- framer    : 2 (Warnings)
-- 6top      : 0 (None)
-- coap      : 0 (None)
-- snmp      : 0 (None)
-- lwm2m     : 0 (None)
-- main      : 3 (Info)
```

Find the IPv6 address of the node:
```
#0012.4b00.0e07.d5a5> ip-addr
Node IPv6 addresses:
-- fd00::212:4b00:e07:d5a5
-- fe80::212:4b00:e07:d5a5
```

Now upload the modified application to another node. After a minute or so, it should join the TSCH network and after that the RPL routing DAG.

Log in to the other node and try pinging the IPv6 address of the first node (the coordinator). Pinging `fe80::212:4b00:e07:d5a5` should always work as long as the node has joined the TSCH network, as its a link-local address and does not require any routing. Pinging `fd00::212:4b00:e07:d5a5` will work only if the node has joined the RPL DAG.

```
#0012.4b00.08fb.22d3> ping fe80::212:4b00:e07:d5a5
Pinging fe80::212:4b00:e07:d5a5
[INFO: TSCH      ] send packet to 0012.4b00.0e07.d5a5 with seqno 17, queue 1/8 1/8, len 21 32
[INFO: TSCH      ] packet sent to 0012.4b00.0e07.d5a5, seqno 17, status 0, tx 2
[INFO: TSCH      ] received from 0012.4b00.0e07.d5a5 with seqno 198
Received ping reply from fe80::212:4b00:e07:d5a5, len 4, ttl 64, delay 656 ms
```

## Reducing the memory usage

TSCH uses more memory than CSMA because each TSCH neighbor gets its own packet queue, while there is a single global packet queue in the CSMA module. This may cause problems for some platforms. Let's try to build the same example for the Texas Instruments CC1310 Launchpad, but now using the `simplelink` platform:

```
$ make savetarget TARGET=simplelink BOARD=launchpad/cc1310
$ make -j
...
  LD        build/simplelink/launchpad/cc1310/hello-world.elf
/opt/gcc-arm-none-eabi-5_2-2015q4/bin/../lib/gcc/arm-none-eabi/5.2.1/../../../../arm-none-eabi/bin/ld: Error: No room left for the stack
collect2: error: ld returned 1 exit status
make: *** [../../arch/cpu/arm/cortex-m/Makefile.cortex-m:26: build/simplelink/launchpad/cc1310/hello-world.elf] Error 1
rm hello-world.o build/simplelink/launchpad/cc1310/obj/startup_cc13xx_cc26xx_gcc.o
```

There are several quick ways how to significantly reduce memory usage. Here they are listed in order of preference; for more detailed coverage, see [this tutorial](/doc/tutorials/RAM-and-ROM-usage).

1) Reduce IP buffer size. By default, up to 1280 bytes long packets (IPv6 MTU) are supported, as required by the IPv6 protocol specification. In low-power wireless networks, this is usually an overkill. If you are confident that the devices will never generate more than a couple hundred bytes long IPv6 packets, reduce `UIP_CONF_BUFFER_SIZE`. For example, add to `project-conf.h`:
```
#define UIP_CONF_BUFFER_SIZE 200
```
2) Reduce the number of network neighbors. By default, 16 neighbors are supported. It can be changed by selecting a different value of `NBR_TABLE_CONF_MAX_NEIGHBORS`. This number applies both to IPv6 (network layer) and TSCH (MAC layer) neighbors. Be careful, as reduced number of neighbors adversely impact the network performance! To set a different value, add to `project-conf.h`:
```
#define NBR_TABLE_CONF_MAX_NEIGHBORS 12
```
3) Reduce the queue size on each neighbor. The default is 8 packets, and the number has to be a power of 2, so there is not much room for manoeuvre. We recommend leaving this setting as is or even increasing to 16 for better network performance. Still, to reduce it, add to `project-conf.h`:
```
#define QUEUEBUF_CONF_NUM 4
```

For more /doc/tutorials/RAM-and-ROM-usage

## See also 

* [Contiki-NG TSCH example applications](/doc/programming/TSCH-example-applications)
* [Contiki-NG hello world tutorial](/doc/tutorials/Hello,-World!)
* [Contiki-NG shell tutorial](/doc/tutorials/Shell)
* [IPv6 ping tutorial](/doc/tutorials/IPv6-ping)