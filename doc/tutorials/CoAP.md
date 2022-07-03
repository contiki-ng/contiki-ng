# CoAP

This tutorial will show you how to set up a Contiki-NG node that runs a CoAP server (see [doc:coap]).

We will need a CoAP client on the Unix side.
This tutorial uses `coap-cli`.
It is readily available in the Docker image (see [doc:docker]).
For a native install, see [doc:install-linux] or [doc:install-osx].

Let us use the example under `examples/coap/coap-examples-server`.
We will try it on a native server here, but the same thing can be done on a device, as described in [tutorial:ping].
```bash
$ make && sudo ./coap-example-server.native
```

You should get:
```
opened tun device ``/dev/tun0''
net.inet.ip.forwarding: 1 -> 1
tun0: flags=8851<UP,POINTOPOINT,RUNNING,SIMPLEX,MULTICAST> mtu 1500
	inet6 fe80::aede:48ff:fe00:1122%tun0 prefixlen 64 optimistic scopeid 0x14 
	inet6 fd00::1 prefixlen 64 tentative 
	nd6 options=201<PERFORMNUD,DAD>
	open (pid 19168)
[INFO: Main      ] Starting Contiki-NG-develop/v4.2-30-g98f35fe09
[INFO: Main      ] - Routing: RPL Lite
[INFO: Main      ] - Net: tun6
[INFO: Main      ] - MAC: nullmac
[INFO: Main      ] - 802.15.4 PANID: 0xabcd
[INFO: Main      ] - 802.15.4 Default channel: 26
[INFO: Main      ] Node ID: 1800
[INFO: Main      ] Link-layer address: 0102.0304.0506.0708
[INFO: Main      ] Tentative link-local IPv6 address: fe80::302:304:506:708
[INFO: Native    ] Added global IPv6 address fd00::302:304:506:708
[INFO: App       ] Starting Erbium Example Server
```

From another terminal, let's query the resource `.well-known/core`:
```
$ coap get coap://[fd00::302:304:506:708]/.well-known/core
(2.05)  </.well-known/core>;ct=40,</test/hello>;title="Hello world: ?len=0..";rt="Text",</debug/mirror>;title="Returns your decoded message";rt="Debug",</test/chunks>;title="Blockwise demo";rt="Data",</test/separate>;title="Separate demo",</test/push>;title="Periodic demo";obs,</test/sub>;title="Sub-resource demo",</test/b1sepb2>;title="Block1 + Separate + Block2 demo"
```

The client outputs the resource content to `stdin` and the CoAP status code (here `(2.05)`) to `stderr`.
You can then try and query other resources listed in `.well-known/core`.

## Build your own CoAP application

To build your own CoAP application, add the following to your `Makefile`:
```
# Include the CoAP implementation
MODULES += os/net/app-layer/coap
```

Then, provide an implementation for your CoAP resources, as exemplified in `examples/coap/coap-example-server/resources`.
From your main process, activate your resources with `coap_activate_resource`.

## CoAP client

We have no tutorial for a Contiki-NG CoAP client yet, but we do provide an example firmware under `examples/coap/coap-example-client`.

[tutorial:ping]: /doc/tutorials/IPv6-ping
[doc:coap]: /doc/programming/CoAP
[doc:docker]: /doc/getting-started/Docker
[doc:install-linux]: /doc/getting-started/Toolchain-installation-on-Linux
[doc:install-osx]: /doc/getting-started/Toolchain-installation-on-macOS
