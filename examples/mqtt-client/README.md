MQTT Client Example
===================
The MQTT client can be used to:

* Publish sensor readings to an MQTT broker.
* Subscribe to a topic and receive commands from an MQTT broker

The demo will give some visual feedback with a LED (configurable):
* Very fast blinking: Searching for a network
* Fast blinking: Connecting to broker
* Slow, long blinking: Sending a publish message

This example is known to work with all platforms that support the new button
API.

This example can operate in two modes: A default mode to be used with the
mosquitto MQTT broker and a second mode to be used with the IBM Watson IoT
platform.

To enable Watson mode, define `MQTT_CLIENT_CONF_WITH_IBM_WATSON` as 1 in the
example's `project-conf.h`.

Publishing
----------
By default the example will attempt to publish readings to an MQTT broker
running on the IPv6 address specified as `MQTT_CLIENT_CONF_BROKER_IP_ADDR` in
`project-conf.h`. This functionality was tested successfully with
[mosquitto](http://mosquitto.org/). This define will be ignored in IBM Watson
mode.

The publish messages include sensor readings but also some other information,
such as device uptime in seconds and a message sequence number. The demo will
publish to topic `iot-2/evt/status/fmt/json`. The device will connect using
client-id `d:<org-id>:mqtt-client:<device-id>`, where `<device-id>` gets
constructed from the device's IEEE address. `<org-id>` can be controlled
through the `MQTT_CLIENT_CONF_ORG_ID` define.

Subscribing
-----------
You can also subscribe to topics and receive commands, but this will only
work if you use "Org ID" != 'quickstart'. To achieve this, you will need to
change `MQTT_CLIENT_CONF_ORG_ID` in `project-conf.h`. In this scenario, the
device will subscribe to:

`iot-2/cmd/+/fmt/json`

You can then use this to toggle LEDs. To do this, you can for example
use mosquitto client to publish to `iot-2/cmd/leds/fmt/json`. So, to change
the state of an LED, you would do this:

`mosquitto_pub -h <broker IP> -m "1" -t iot-2/cmd/leds/fmt/json`

Where `broker IP` should be replaced with the IP address of your mosquitto
broker (the one where you device has subscribed). Replace `-m "1'` with `-m "0"`
to turn the LED back off.

Bear in mind that, even though the topic suggests that messages are of json
format, they are in fact not. This was done in order to avoid linking a json
parser into the firmware. This comment only applies to parsing incoming
messages, outgoing publish messages use proper json payload.

IBM Quickstart Service
----------------------
It is also possible to publish to IBM's quickstart service. To do so, you need
to enable this mode by setting `MQTT_CLIENT_CONF_WITH_IBM_WATSON` to 1 in
`project-conf.h`.

The device will then try to connect to IBM's quickstart over NAT64, so you will
need a NAT64 gateway in your network to make this work. A guide on how to
setup NAT64 is out of scope here, but you can find one in the
[Contiki-NG wiki](https://github.com/contiki-ng/contiki-ng/wiki/NAT64-for-Contiki%E2%80%90NG).

If you want to use IBM's cloud service with a registered device, you will need
to set `MQTT_CLIENT_CONF_ORG_ID` and then also to provide the 'Auth Token'
(`MQTT_CLIENT_CONF_AUTH_TOKEN`), which acts as a 'password'. You will also
need to configure your Organisation / Registered device on Watson such that
TLS is optional.

Note: The token will be transported in cleartext.
