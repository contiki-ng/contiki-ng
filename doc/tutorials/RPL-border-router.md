# RPL border router

> _See [tutorial:cooja-border-router] instead if you want connect to a network of nodes simulated in Cooja._

This tutorial assumes you have already run [tutorials:rpl].
We will now set up a node as border router, acting not only as DAG root but also as Internet access point for all nodes in the network.

First, program one or several nodes with `hello-world` (which has RPL enabled). Then, go to `examples/rpl-border-router`, compile and upload the example to the node you want to use as border router. The Makefile in this examples includes a number of targets that help setting up the border router on the host (e.g. linux) side.

The connection with the border router is achieved using a tool called `tunslip6`. In this tutorial, we will use the make target `connect-router`. This make target will invoke `tunslip6` using some default parameters, including a default serial port name (`/dev/ttyUSB0`). If your device has enumerated using a different serial port name (which will almost certainly be the case on Mac OS), you will have to call `tunslip6` manually and provide the correct serial port name using the `-s` argument. For example, you may need to run:

```
sudo ../../tools/serial-io/tunslip6 -s /dev/tty.usbmodemL1001111 fd00::1/64
```

Using the `connect-router` target:
```
$ make TARGET=zoul connect-router 
sudo ../../tools/serial-io/tunslip6 fd00::1/64
********SLIP started on ``/dev/ttyUSB0''
opened tun device ``/dev/tun0''
ifconfig tun0 inet `hostname` mtu 1500 up
ifconfig tun0 add fd00::1/64
ifconfig tun0 add fe80::0:0:0:1/64
ifconfig tun0

tun0      Link encap:UNSPEC  HWaddr 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  
          inet addr:127.0.1.1  P-t-P:127.0.1.1  Mask:255.255.255.255
          inet6 addr: fd00::1/64 Scope:Global
          inet6 addr: fe80::1/64 Scope:Link
          UP POINTOPOINT RUNNING NOARP MULTICAST  MTU:1500  Metric:1
          RX packets:0 errors:0 dropped:0 overruns:0 frame:0
          TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:500 
          RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)

[INFO: BR        ] Waiting for prefix
*** Address:fd00::1 => fd00:0000:0000:0000
[INFO: BR        ] Waiting for prefix
[INFO: BR        ] Server IPv6 addresses:
[INFO: BR        ]   fd00::212:4b00:616:fcc
[INFO: BR        ]   fe80::212:4b00:616:fcc
```

This runs the program `tools/serial-io/tunslip6`, which bridges the Contiki-NG border router to the host (here, Linux) via a tun interface (here, `tun0`). The default prefix is `fd00::1/64` (can be configured via the make `PREFIX` variable). Your host is assigned address `fd00::1`. The RPL border router, running on the node, has an auto-configured address which you can find out from the above logs. Let's try and ping it:
```
$ ping6  fd00::212:4b00:616:fcc
PING fd00::212:4b00:616:fcc(fd00::212:4b00:616:fcc) 56 data bytes
64 bytes from fd00::212:4b00:616:fcc: icmp_seq=1 ttl=64 time=154 ms
64 bytes from fd00::212:4b00:616:fcc: icmp_seq=2 ttl=64 time=25.7 ms
```

To access nodes inside the RPL network, we first need to find out their global IPv6 address. To help with this, the RPL border router runs an HTTP server -- request the index page with:
```
$ wget -6 "http://[fd00::212:4b00:616:fcc]/"
--2017-10-12 10:04:55--  http://[fd00::212:4b00:616:fcc]/
Connecting to [fd00::212:4b00:616:fcc]:80... connected.
HTTP request sent, awaiting response... 200 OK
Length: unspecified [text/html]
Saving to: ‘index.html’

    [ <=>                                                                                                                                                                ] 121         --.-K/s   in 0.09s   

2017-10-12 10:04:56 (1.34 KB/s) - ‘index.html’ saved [121]
```

The page contains a list of routes (in storing mode) or links (in non-storing mode), indicating the addresses of all nodes in the network (if no node shows up, give the network some extra seconds and request the HTTP server again):
```
$ cat index.html
<html><head><title>ContikiRPL</title></head><body>
Neighbors<pre>fe80::212:4b00:616:fc4
</pre>Routes<pre>
</pre>Links<pre>
fd00::212:4b00:616:fc4 (parent: fd00::212:4b00:616:fcc) 1800s
</pre></body></html>
```

You should now be able to ping any node in the network, e.g.:
```
$ ping6 fd00::212:4b00:616:fc4
PING fd00::212:4b00:616:fc4(fd00::212:4b00:616:fc4) 56 data bytes
64 bytes from fd00::212:4b00:616:fc4: icmp_seq=1 ttl=63 time=142 ms
64 bytes from fd00::212:4b00:616:fc4: icmp_seq=2 ttl=63 time=38.7 ms
```

## Native Border Router

In the example above, we used the so-called "embedded border router".
This means the border router (BR) runs on the constrained device, and the host computer runs `tunslip6`.
Alternatively, it is possible to run the BR directly on the host computer, and have the constrained device just act as a radio dongle.
To do this, follow these two steps:
1. Program your node with `slip-radio` instead of `rpl-border-router`.
1. On the host computer, go to `examples/rpl-border-router`
    * Build the example for the native platform `make TARGET=native`
    * Run `sudo ./border-router.native fd00::1/64`. If the USB serial port is not found, specify it via option `-s`.

This approach has the advantage of running the BR on an unconstrained device.
The main downside, however, is that it separates the low layers (radio and MAC) from the rest of the stack (NET and up).
In networks with TSCH, this is a problem as we do not have yet a way to communicate schedules to the `slip-radio` over the serial interface.
Currently only usable with CSMA or TSCH with 6TiSCH minimal schedule.

## Custom Border Router

Note that you do not have to use the `rpl-border-router` example to build a border router.
In Contiki-NG, you can simply turn any project into a border router by adding to your Makefile the following module (and as always, `make distclean`):
```
MODULES += os/services/rpl-border-router
```

Projects with this module can be compiled to either a native border router (use `TARGET=native`) or an embedded border router (any other target).
If you take a closer look to `examples/rpl-border-router`, it basically uses the border router module, with a Web server on top for servicing the list of routes over HTTP.

[tutorials:rpl]: /doc/tutorials/RPL

[tutorial:cooja-border-router]: /doc/tutorials/Cooja-simulating-a-border-router
