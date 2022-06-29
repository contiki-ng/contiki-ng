# The Contiki‐NG configuration system

Contiki-NG has plenty of configuration knobs, to tailor the system to the needs of any given project and platform. Most of the configuration is done via `.h` files. Some configuration (netstack configuration, module inclusion) is done via the project `Makefile`.

## Modules
Projects can include their own modules, that is, sub-directories of the top-level `os`, via the Makefile variable `MODULES`. For instance, `MODULES = os/net/app-layer/coap` includes the CoAP protocol. To understand the order of inclusion of Makefiles, see [doc:build-system].

The networking stack has two main layers: the MAC (Medium Access Control) layer and the NET (Network) layer. These are also configured from the project Makefile, as described next.

Note: whenever adding or modifying a module, networking stack, or editing the Makefile in general, do not forget to clean your build artifacts with `make distclean`.

## Network Stack: MAC Layer
To select a MAC layer, set the Make variable `MAKE_MAC` with one of the following values:
* `MAKE_MAC_NULLMAC`: A MAC layer that does nothing. No packet transmission nor reception.
* `MAKE_MAC_CSMA` (default): The IEEE 802.15.4 non-beacon-enabled mode, which uses CSMA on always-on radios.
* `MAKE_MAC_TSCH`: The IEEE 802.15.4 TSCH (TimeSlotted Channel Hopping) mode. This is a globally-synchronized, scheduled, frequency-hopping MAC (see [doc:6tisch])
* `MAKE_MAC_BLE`: An experimental MAC layer for devices with a BLE radio. Can be used to enable IPv6 over BLE. Currently only available for CC2650 devices. See the respective example under `examples/platform-specific/cc26xx/ble-ipv6/`
* `MAKE_MAC_OTHER`: None of the above. Useful to specify a different, custom MAC.

Regardless of the flag above, the `.h` file can specify their own implementation of the MAC via the flag `NETSTACK_CONF_MAC`. This enables alternative implementations of NullMAC, CSMA or TSCH. When `MAKE_MAC_OTHER` is set, `NETSTACK_CONF_MAC` is mandatory.

## Network Stack: NET Layer
To select a NET layer, set the Make variable `MAKE_NET` with one of the following values:
* `MAKE_NET_NULLNET`: A NET layer that does nothing. Packets are relayed up/down the stack, unmodified. See [doc:nullnet]
* `MAKE_NET_IPV6` (default): The uIP low-power IPv6 stack, with 6LoWPAN and RPL. By default, RPL is enabled and more specifically the RPL-lite version. The next subsection details routing protocol configuration
* `MAKE_NET_OTHER`: None of the above. Useful to specify a different, custom MAC.

Regardless of the flag above, the `.h` file can specify their own implementation of the NET via the flag `NETSTACK_CONF_NETWORK`. This enables alternative implementations of NullNet or IPv6. When `MAKE_NET_OTHER` is set, `NETSTACK_CONF_NETWORK` is mandatory.

## Routing protocol
In the IPv6 case, you can select one of the three routing configurations by setting `MAKE_ROUTING` as:
* `MAKE_ROUTING_NULLROUTING`: No routing protocol.
* `MAKE_ROUTING_RPL_LITE` (default): The RPL-Lite implementation of RPL (see [doc:rpl]).
* `MAKE_ROUTING_RPL_CLASSIC`: The RPL-Classic implementation of RPL (see [doc:rpl]).

## System configuration
All other configuration parameters are set from the `.h` files. To set project-specific configurations, create a `project-conf.h` file inside the project directory. For instance, to enable TCP, set `#define UIP_CONF_TCP 1`. A list of commonly used configuration flags is shown in `os/contiki-default-conf.h`. After adding a `project-conf.h`, make sure to clean all build artifacts with `make distclean`.

There are a number of different `.h` files included in the following order:
* `module-macros.h`: For each module included in the build, a set of macros that are included first. Used to define constants values rather than to configure other Contiki-NG module parameters.
* `project-conf.h`: Project-specific configuration. Used to configure various Contiki-NG modules.
* `contiki-conf.h`: This is a platform-specific configuration file. Often also includes CPU-specific `.h` files. Sets a number of platform-specific flags that match what the project needs.
* `contiki-default-conf.h`: This defines defaults for a number of Contiki-NG parameters. This is also useful as a reference of common configuration flags.
* Module-specific `.h` files: These, such as for instance `uip.h`, come last, i.e., they set default values for parameters not defined in any of the above files.

It is sometimes easy to lose track of which flags are set or not. To inspect your current configuration, try target `viewconf`:
```
$ make TARGET=zoul viewconf
----------------- Make variables: --------------
##### "TARGET": ________________________________ zoul
##### "BOARD": _________________________________ remote-revb
##### "MAKE_MAC": ______________________________ MAKE_MAC_CSMA
##### "MAKE_NET": ______________________________ MAKE_NET_IPV6
##### "MAKE_ROUTING": __________________________ MAKE_ROUTING_RPL_LITE
----------------- C variables: -----------------
##### "PROJECT_CONF_PATH": _____________________ ><
##### "CONTIKI_VERSION_STRING": ________________ == "Contiki-NG-release/v4.2"
##### "FRAME802154_CONF_VERSION":_______________ == (0x01)
##### "IEEE802154_CONF_PANID":__________________ == 0xabcd
##### "IEEE802154_CONF_DEFAULT_CHANNEL": _______ == 26
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
##### "ENERGEST_CONF_ON": ______________________ == 0
##### "LOG_CONF_LEVEL_RPL": ____________________ == 0
##### "LOG_CONF_LEVEL_TCPIP": __________________ == 0
##### "LOG_CONF_LEVEL_IPV6": ___________________ == 0
##### "LOG_CONF_LEVEL_6LOWPAN": ________________ == 0
##### "LOG_CONF_LEVEL_NULLNET": ________________ == 0
##### "LOG_CONF_LEVEL_MAC": ____________________ == 0
##### "LOG_CONF_LEVEL_FRAMER": _________________ == 0
##### "LOG_CONF_LEVEL_6TOP": ___________________ == 0
##### "LOG_CONF_LEVEL_COAP": ___________________ == 0
##### "LOG_CONF_LEVEL_LWM2M": __________________ == 0
##### "LOG_CONF_LEVEL_MAIN": ___________________ == 3
------------------------------------------------
'==' Means the flag is set to a given a value
'->' Means the flag is unset, but will default to a given value
'><' Means the flag is unset and has no default value
To view more Make variables, edit ../../Makefile.include, rule 'viewconf'
To view more C variables, edit ../../tools/viewconf/viewconf.c
```

[doc:build-system]: The-Contiki-NG-build-system.md
[doc:6tisch]: /doc/programming/TSCH-and-6TiSCH
[doc:ipv6]: /doc/programming/IPv6
[doc:rpl]: /doc/programming/RPL
[doc:nullnet]: /doc/programming/NullNet
