# MQTT

MQTT is a publish/subscribe messaging transport protocol for M2M communications. It runs over TCP or TLS and is supported by many IoT platforms.

## MQTT in Contiki-NG
Contiki-NG features a client implementation of [MQTT version 3.1][mqtt-3-1-spec]. The implementation supports MQTT QoS levels 0 and 1 and allows a Contiki-NG MQTT client to subscribe with and publish to an MQTT broker.

The MQTT engine is implemented in `os/net/app-layer/mqtt/mqtt.[ch]`. The implementation does not currently offer any configuration options.

The MQTT client engine has been tested against the [Mosquitto MQTT broker][mosquitto], as well as against IBM's Quickstart / Watson IoT Platform.

Visit [tutorial:mqtt] for an example on how to use the MQTT client on your device.

## Limitations
* The implementation does not support MQTT QoS 2.
* The implementation does not support reception of publish messages with QoS 1.
* The implementation does not support MQTT over TLS.

[mqtt-3-1-spec]: http://public.dhe.ibm.com/software/dw/webservices/ws-mqtt/mqtt-v3r1.html
[tutorial:mqtt]: /doc/tutorials/MQTT
[mosquitto]: https://mosquitto.org/
