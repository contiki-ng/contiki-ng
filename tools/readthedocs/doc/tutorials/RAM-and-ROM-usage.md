# RAM and ROM usage

We will use the example `hello-world` again here (see [tutorial:hello-world]), with target `zoul`.

## Binary file inspection

First, build the example:
```
$ make TARGET=zoul savetarget
$ make
```

To check the size of the resulting binary, try the standard command `size` on the ELF file:
```
$ size hello-world.zoul
   text    data     bss     dec     hex filename
  38758     412   10880   50050    c382 hello-world.zoul
```

`text` shows the size of the code section in bytes -- this will typically be in ROM. `data` and `bss` show sections that contain variables, stored in RAM. The Remote board has 32 KB RAM and 512 KB ROM (see [platform:remote]), so we have some free space here. You can also try other standard commands for ELF file inspection, such as `nm` and `objdump`.

Contiki-NG provides, in addition, the targets `.flashprof` and `.ramprof`, which show the list of largest symbols in ROM and RAM, respectively. For instance:
```
$ make hello-world.ramprof
...
00000248 uip_ds6_if
00000256 received_seqnos
00000256 uip_udp_conns
00000288 _ds6_neighbors_mem
00000352 curr_instance
00000368 frag_info
00000384 events
00000384 nodememb_memb_mem
00001280 uip_aligned_buf
00001356 frag_buf
00001360 buframmem_memb_mem
00002048 stack
```

Shows you that the pre-allocated runtime stack consumed 2048 bytes, the queuebuf module consumes 1360 bytes (through the `memb` array `buframmem_memb_mem`), etc.

Now let's take a look at the ROM usage:
```
$ make hello-world.flashprof
...
00000580 dio_input
00000628 ns_input
00000636 rpl_ext_header_update
00000642 __udivdi3
00000652 vectors
00000700 __divdi3
00001104 uip_process
00001736 format_str_v
00001752 input
00001820 output
```

This might not seem extremely readable at first, but `nm` will help us find where the symbols come from. For instance, the largest functions are `output` and `input`. Let's look them up as follows:
```
$ nm -oStd obj_zoul/*.o | grep " output$"
obj_zoul/sicslowpan.o:00000001 00001820 t output
$ nm -oStd obj_zoul/*.o | grep " input$"
obj_zoul/sicslowpan.o:00000001 00001752 t input
```

These two functions are from the `sicslowpan` module. Such troubleshooting can help identify where log levels need to be reduced, or what features should be removed.

## Saving RAM and ROM

As you enable more features or edit your configuration files, you might saturate your memory. The linker will then fail to produce a final firmware, and issue a message like:
```
  LD        hello-world.elf
/usr/lib/gcc/arm-none-eabi/4.8.2/../../../arm-none-eabi/bin/ld: hello-world.elf section `.bss' will not fit in region `FRSRAM'
/usr/lib/gcc/arm-none-eabi/4.8.2/../../../arm-none-eabi/bin/ld: region `FRSRAM' overflowed by 144 bytes
collect2: error: ld returned 1 exit status
make: *** [hello-world.elf] Error 1
```

In this case, use `viewconf` (see [doc:configuration]) to inspect your configuration:
```
$ make viewconf
----------------- Make variables: --------------
##### "TARGET": ________________________________ zoul
##### "BOARD": _________________________________ remote-revb
##### "MAKE_MAC": ______________________________ MAKE_MAC_CSMA
##### "MAKE_NET": ______________________________ MAKE_NET_IPV6
##### "MAKE_ROUTING": __________________________ MAKE_ROUTING_RPL_LITE
----------------- C variables: -----------------
##### "PROJECT_CONF_PATH": _____________________ ><
##### "CONTIKI_VERSION_STRING": ________________ == "Contiki-NG-4.0"
##### "IEEE802154_CONF_PANID":__________________ == 0xabcd
##### "FRAME802154_CONF_VERSION":_______________ == (0x01)
##### "RF_CHANNEL": ____________________________ ><
##### "QUEUEBUF_CONF_NUM": _____________________ == 8
##### "NBR_TABLE_CONF_MAX_NEIGHBORS": __________ == 16
##### "NETSTACK_MAX_ROUTE_ENTRIES": ____________ == 16
##### "UIP_CONF_BUFFER_SIZE": __________________ == 1280
##### "UIP_CONF_UDP": __________________________ == 1
##### "UIP_CONF_UDP_CONNS": ____________________ == 8
##### "UIP_CONF_TCP": __________________________ == 0
##### "UIP_CONF_TCP_CONNS": ____________________ == 0
##### "UIP_CONF_ND6_SEND_RA": __________________ == 0
##### "UIP_CONF_ND6_SEND_NS": __________________ == 0
##### "UIP_CONF_ND6_SEND_NA": __________________ == 1
##### "UIP_CONF_ND6_AUTOFILL_NBR_CACHE": _______ == 1
##### "SICSLOWPAN_CONF_FRAG": __________________ == 1
##### "SICSLOWPAN_CONF_COMPRESSION": ___________ == SICSLOWPAN_COMPRESSION_IPHC
##### "LOG_CONF_LEVEL_RPL": ____________________ == LOG_LEVEL_NONE
##### "LOG_CONF_LEVEL_TCPIP": __________________ == LOG_LEVEL_NONE
##### "LOG_CONF_LEVEL_IPV6": ___________________ == LOG_LEVEL_NONE
##### "LOG_CONF_LEVEL_6LOWPAN": ________________ == LOG_LEVEL_NONE
##### "LOG_CONF_LEVEL_NULLNET": ________________ == LOG_LEVEL_NONE
##### "LOG_CONF_LEVEL_MAC": ____________________ == LOG_LEVEL_NONE
##### "LOG_CONF_LEVEL_FRAMER": _________________ == LOG_LEVEL_NONE
##### "LOG_CONF_LEVEL_6TOP": ___________________ == LOG_LEVEL_NONE
------------------------------------------------
'==' Means the flag is set to a given a value
'->' Means the flag is unset, but will default to a given value
'><' Means the flag is unset and has no default value
To view more Make variables, edit ../../Makefile.include, rule 'viewconf'
To view more C variables, edit ../../tools/viewconf.c
```

This shows you the current value of many important configuration parameters. To override any of these, simply define them from your `project-conf.h` file (see [doc:configuration]).

If you need to save RAM, you might consider reducing:
* `QUEUEBUF_CONF_NUM`: the number of packets in the link-layer queue. 4 is probably a lower bound for reasonable operation. As the traffic load increases, e.g. more frequent traffic or larger datagrams, you will need to increase this parameter.
* `NBR_TABLE_CONF_MAX_NEIGHBORS`: the number of entries in the neighbor table. A value greater than the maximum network density is safe. A value lower than that will also work, as the neighbor table will automatically focus on relevant neighbors. But too low values will result in degraded performance.
* `NETSTACK_MAX_ROUTE_ENTRIES`: the number of routing entries, i.e., in RPL non-storing mode, the number of links in the routing graph, and in storing mode, the number of routing table elements. At the network root, this must be set to the maximum network size. In non-storing mode, other nodes can set this parameter to 0. In storing mode, it is recommended for all nodes to also provision enough entries for each node in the network.
* `UIP_CONF_BUFFER_SIZE`: the size of the IPv6 buffer. The minimum value for interoperability is 1280. In closed systems, where no large datagrams are used, lowering this to e.g. 140 may be sensible.
* `SICSLOWPAN_CONF_FRAG`: Enables/disables 6LoWPAN fragmentation. Disable this if all your traffic fits a single link-layer packet. Note that this will also save some significant ROM.

If you need to save ROM, you can consider the following:
* `UIP_CONF_TCP`: Enables/disables TCP. Make sure this is disabled when TCP is unused.
* `UIP_CONF_UDP`: Enables/disables UDP. Make sure this is disabled when UDP is unused.
* `SICSLOWPAN_CONF_FRAG`: As mentioned above. Disable if no fragmentation is needed.
* `LOG_CONF_LEVEL_*`: Logs consume a large amount of ROM. Reduce log levels to save some more.

There are many other parameters that affect RAM/ROM usage. You can inspect `os/contiki-default-conf.h` as well as platform-specific `contiki-conf.h` files for inspiration. Or use `.flashprof` and `.ramprof` to identify the hotspots.

[doc:configuration]: /doc/getting-started/The-Contiki-NG-configuration-system
[tutorial:hello-world]: /doc/tutorials/Hello,-World!
[platform:remote]: /doc/platforms/zolertia/Zolertia-RE-Mote-platform-(revision-B)
