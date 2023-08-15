# Hello, World!

A number of example applications for Contiki-NG are provided under `examples` in the root directory. In this tutorial, we will show how the `hello-world` example works. This classic programming example will just print "Hello, world" to the serial port periodically. Take a look at the `hello-world.c` file to see the source code. This example illustrates how processes are defined, and how to make sure that the operating system automatically starts a process after booting up.

## Running the example as a native node

Contiki-NG can be compiled as a native Unix process.
This is useful for instance to implement the RPL native Border Router, and in general for testing: the networking stack of native nodes is reachable through a tun interface they create.
One can use this interface to e.g. ping the node (see [tutorial:ping]) or send CoAP requests (see [tutorial:coap]).

To run `hello-world` in native mode, type:
```shell
$ cd examples/hello-world
$ make TARGET=native
$ ./hello-world.native
```

You first see the Contiki-NG boot messages and then a line with the string "Hello, world". This line will repeat periodically. Contiki-NG debugging messages will also be printed. Note that the warning regarding opening the tun device can be ignored, as this example does not depend on the networking functionality.

```
[INFO: Main      ] Starting Contiki-NG-4.0
[INFO: Main      ]  Net: tun6
[INFO: Main      ]  MAC: nullmac
[WARN: Tun6      ] Failed to open tun device (you may be lacking permission). Running without network.
[INFO: Main      ] Link-layer address 0102.0304.0506.0708
[INFO: Main      ] Tentative link-local IPv6 address fe80::302:304:506:708
Hello, world
Hello, world
```

## Running the example on a real device
First, make sure the necessary toolchains are installed: For Linux see [doc:toolchain-installation-linux]. For Mac OS, see [doc:toolchain-installation-macos].

If you're in a virtual machine, make sure to make the USB device visible inside the VM, through the device sharing options of your virtual machine software. Identify the USB port of your device by using `make motelist-all`. It might be for instance `/dev/ttyUSB0` or `/dev/ttyUSB1`.

To run the same project on an IoT device such as the Zolertia RE-Mote, you can run the following shell command:

```shell
$ make TARGET=zoul BOARD=firefly PORT=<your-device-usb-port> hello-world.upload
$ make TARGET=zoul BOARD=firefly PORT=<your-device-usb-port> login
```

This will first program the IoT device with the compiled system firmware, and the connect to the first available serial port. Note that you may have to make arrangements in your OS to give permission to your user to access a particular serial port (on Linux, try `sudo adduser <username> dialout`, and the log off your session and in again. See [doc:toolchain-installation-linux]).

You should get the following output (boot messages might not show up, unless you press the reset button on the device while the `login` command is already running):
```
[INFO: Main      ] Starting Contiki-NG-4.0
[INFO: Main      ]  Net: sicslowpan
[INFO: Main      ]  MAC: CSMA
[INFO: Main      ] Link-layer address 0012.4b00.060d.b200
[INFO: Main      ] Tentative link-local IPv6 address fe80::212:4b00:60d:b200
[INFO: Zoul      ] Zolertia Firefly revision B platform
Hello, world
Hello, world
```

## Running the example in Cooja

Cooja is a very useful development tool, allowing fine-grained simulation/emulation of Contiki-NG networks.
To run this, or any other example, in Cooja, follow: [tutorial:cooja].

## Tips

Note that you can use `savetarget` so save a given target and board, so you do no longer have to specify it every single time you call make (see doc:build-system):
```
$ make TARGET=zoul BOARD=remote-revb savetarget
```

Next times you type use make, you should see the message:
```
using saved target 'zoul'
```

You can also check your configuration, target, board etc. with `viewconf`:
```
$ make viewconf
using saved target 'zoul'
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

[doc:toolchain-installation-linux]: /doc/getting-started/Toolchain-installation-on-Linux
[doc:toolchain-installation-macos]: /doc/getting-started/Toolchain-installation-on-macOS
[doc:build-system]: /doc/getting-started/The-Contiki-NG-build-system
[tutorial:cooja]: /doc/tutorials/Running-Contiki-NG-in-Cooja
[tutorial:ping]: /doc/tutorials/IPv6-ping
[tutorial:coap]: /doc/tutorials/CoAP
