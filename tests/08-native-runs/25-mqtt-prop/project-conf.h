#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* The input property decoders under test exist only in MQTT version 5. */
#define MQTT_CONF_VERSION MQTT_PROTOCOL_VERSION_5

/* The MQTT implementation is built on TCP sockets, which are not compiled
 * in by default. No connection is made by this test.
 */
#define UIP_CONF_TCP 1

#endif /* PROJECT_CONF_H_ */
