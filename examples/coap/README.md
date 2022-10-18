# coap

## CoAP examples: client, server, and plugtest server

* coap-example-server: A CoAP server example showing how to use the CoAP layer to develop server-side applications.
* coap-example-client: A CoAP client that polls the /actuators/toggle resource every 10 seconds and cycles through 4 resources on button press (target address is hard-coded).
* coap-plugtest-server: The server used for draft compliance testing at ETSI IoT CoAP Plugtests. Erbium (Er) participated in Paris, France, March 2012 and Sophia-Antipolis, France, November 2012 (configured for native).

The examples can run either on a real device or as native.
In the latter case, just start the executable with enough permissions (e.g. sudo), and you will then be able to reach the node via tun.
A tutorial for setting up the CoAP server example and querying it is provided on the wiki.
