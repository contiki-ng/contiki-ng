# MQTT

This tutorial will help you understand and get started with Contiki-NG's MQTT client functionality. To achieve this, the tutorial will use:

* The `mqtt-client` example under `examples/mqtt-client`.
* The [Mosquitto MQTT broker][mosquitto] and accompanying clients.

The tutorial assumes you have basic understanding of [MQTT][mqtt-3-1] (also see [doc:mqtt]).

Firstly, make sure you have mosquitto correctly set up on your system by following [the instructions][doc:setup] that correspond to your system.

## On your Linux / OS X device

### Run your mosquitto broker
```
$ mosquitto -v
1524857657: mosquitto version 1.3.4 (build date 2014-10-21 21:42:35+0100) starting
1524857657: Using default config.
1524857657: Opening ipv6 listen socket on port 1883.
1524857657: Opening ipv4 listen socket on port 1883.
```

### Subscribe to an MQTT topic and publish to it

Open a new terminal window and run:
```
$ mosquitto_sub -h localhost -t "#"
```

This will subscribe to all topics.
On the terminal window where you ran the MQTT broker. You should see information about the incoming subscription:
```
1524857760: New connection from ::1 on port 1883.
1524857760: New client connected from ::1 as mosqsub/5439-IT000519.l (c1, k60).
1524857760: Sending CONNACK to mosqsub/5439-IT000519.l (0)
1524857760: Received SUBSCRIBE from mosqsub/5439-IT000519.l
1524857760: 	# (QoS 0)
1524857760: mosqsub/5439-IT000519.l 0 ##
1524857760: Sending SUBACK to mosqsub/5439-IT000519.l
```

Open yet another terminal window and send an MQTT publish message:
```
mosquitto_pub -h localhost -t "iot-2/evt/status/fmt/json" -m "Test publish message"
```

On the terminal window where you ran the MQTT broker. You should see information about the incoming publish:
```
1524857792: New connection from ::1 on port 1883.
1524857792: New client connected from ::1 as mosqpub/5442-IT000519.l (c1, k60).
1524857792: Sending CONNACK to mosqpub/5442-IT000519.l (0)
1524857792: Received PUBLISH from mosqpub/5442-IT000519.l (d0, q0, r0, m0, 'iot-2/evt/status/fmt/json', ... (20 bytes))
1524857792: Sending PUBLISH to mosqsub/5439-IT000519.l (d0, q0, r0, m0, 'iot-2/evt/status/fmt/json', ... (20 bytes))
1524857792: Received DISCONNECT from mosqpub/5442-IT000519.l
```

See how the broker has received the message and sent it over to subscribers. The terminal window where you ran the subscriber should have received the publish message and it should have printed its payload:

```
Test publish message
```

### Build and run the mqtt-client for platform native
Open a new terminal window (or use the one where you earlier ran `mosquitto_pub`), navigate to `examples/mqtt-client` and build the example for platform native by running `make TARGET=native`.

Run the example:
```
$ sudo ./mqtt-client.native 
[INFO: Main      ] Starting Contiki-NG-develop/v4.1-87-g1ffcaa8
[INFO: Main      ]  Net: tun6
[INFO: Main      ]  MAC: nullmac
opened tun device ``/dev/tun0''
net.inet.ip.forwarding: 1 -> 1
tun0: flags=8851<UP,POINTOPOINT,RUNNING,SIMPLEX,MULTICAST> mtu 1500
	inet6 fe80::82e6:50ff:fe24:89e6%tun0 prefixlen 64 optimistic scopeid 0xe 
	inet6 fd00::1 prefixlen 64 tentative 
	nd6 options=201<PERFORMNUD,DAD>
	open (pid 6766)
[INFO: Main      ] Link-layer address 0102.0304.0506.0708
[INFO: Main      ] Tentative link-local IPv6 address fe80::302:304:506:708
[INFO: Native    ] Added global IPv6 address fd00::302:304:506:708
MQTT Client Process
```

The mqtt-client will connect to the MQTT broker and will start publishing periodically.

Switch to the terminal where your subscriber is running. After a little while (expect 8-40s), messages will start appearing (and repeat at a 30s interval).
```
{"d":{"Platform":"native","Seq #":1,"Uptime (sec)":2004730,"Def Route":"fd00::1","RSSI (dBm)":0}}
{"d":{"Platform":"native","Seq #":2,"Uptime (sec)":2004760,"Def Route":"fd00::1","RSSI (dBm)":0}}
{"d":{"Platform":"native","Seq #":3,"Uptime (sec)":2004790,"Def Route":"fd00::1","RSSI (dBm)":0}}
{"d":{"Platform":"native","Seq #":4,"Uptime (sec)":2004820,"Def Route":"fd00::1","RSSI (dBm)":0}}
```

Switch to the mosquitto broker console. The broker's debugging output will show you corresponding information:
```
1524858389: New connection from fd00::302:304:506:708 on port 1883.
1524858389: New client connected from fd00::302:304:506:708 as d:contiki-ng:mqtt-client:010203060708 (c1, k24464, uuse-token-auth).
1524858389: Sending CONNACK to d:contiki-ng:mqtt-client:010203060708 (0)
1524858389: Received SUBSCRIBE from d:contiki-ng:mqtt-client:010203060708
1524858389: 	iot-2/cmd/+/fmt/json (QoS 0)
1524858389: d:contiki-ng:mqtt-client:010203060708 0 iot-2/cmd/+/fmt/json
1524858389: Sending SUBACK to d:contiki-ng:mqtt-client:010203060708
1524858419: Received PUBLISH from d:contiki-ng:mqtt-client:010203060708 (d0, q0, r0, m0, 'iot-2/evt/status/fmt/json', ... (97 bytes))
1524858419: Sending PUBLISH to mosqsub/5439-IT000519.l (d0, q0, r0, m0, 'iot-2/evt/status/fmt/json', ... (97 bytes))
```
## Run on an embedded device
You can run the same example on various embedded platforms too, such as zoul- or CC26x0/CC13x0-based ones. The example will work off-the-shelf for those platforms.

Firstly, you will need establish network communication between your 6LoWPAN mesh and the device that hosts your mosquitto MQTT broker. You will need a border router to achieve this, see the [corresponding guide][tutorial:rpl-br] if uncertain how to achieve this.

Build the example for your device and program your device with the corresponding firmware. Once the device has connected to the mesh network, it will try to connect with an MQTT broker running at address `fd00::1`. If you have correctly set up your border router, your device should already have an interface with this address. Once the MQTT connection has been established, the device will start publishing periodically, in a fashion similar to what is shown above.

## Control your device using MQTT subscriptions
This functionality only supports MQTT QoS 0.

The MQTT client will also subscribe to topic `iot-2/cmd/leds/fmt/json`.

Using a terminal window, you can use `mosquitto_pub` to send a publish message to this topic in order to control your device.

```
mosquitto_pub -h localhost -t iot-2/cmd/leds/fmt/json -m "1"
```

If your device has leds, the red one will blink! If you built the example with logging level set to debug (in `mqtt-client.c`), you will see something like this in the device's output:

```
[DBG : mqtt-client] Application received publish for topic 'iot-2/cmd/leds/fmt/json'. Payload size is 1 bytes.
[DBG : mqtt-client] Pub Handler: topic='iot-2/cmd/leds/fmt/json' (len=23), chunk_len=1
```

## Use with the IBM Watson IoT platform
The example is written such that it will also work with the [Watson IoT platform](https://www.ibm.com/watson/). Firstly you will need to make sure your device can communicate with the IPv4 Internet. To achieve this, you will likely need to set up NAT64 [as per this guide][nat64].

You will then need to configure the example such that it attempts to use the Watson functionality. Edit the example's `project-conf.h` and delete the definition of `MQTT_CLIENT_CONF_BROKER_IP_ADDR`.

```
/* If undefined, the demo will attempt to connect to IBM's quickstart */
#define MQTT_CLIENT_CONF_BROKER_IP_ADDR "fd00::1"
```

By default, the example will attempt to use the quickstart service. In this mode of operation, the device will _not_ subscribe to any topic. It is possible to get the device to use the full version of the IoT platform, but at the time of writing this guide this can only be done by changing the example's source code. Note that you will need to configure your Organisation / Registered device on Watson such that TLS is optional. Note that your device's authentication token will be transported in cleartext!

[mqtt-3-1]: http://public.dhe.ibm.com/software/dw/webservices/ws-mqtt/mqtt-v3r1.html
[doc:setup]: /doc/Home.md
[doc:mqtt]: /doc/programming/MQTT.md
[tutorial:rpl-br]: /doc/tutorials/RPL-border-router.md
[mosquitto]: https://mosquitto.org/
[nat64]: /doc/getting-started/NAT64-for-Contiki-NG.md
