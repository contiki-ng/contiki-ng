A Quick Introduction to the Erbium (Er) CoAP Engine
===================================================

EXAMPLE FILES
-------------

- coap-example-server.c: A CoAP server example showing how to use the CoAP
  layer to develop server-side applications.
- coap-example-client.c: A CoAP client that polls the /actuators/toggle resource
  every 10 seconds and cycles through 4 resources on button press (target
  address is hard-coded).
- plugtest-server.c: The server used for draft compliance testing at ETSI
  IoT CoAP Plugtests. Erbium (Er) participated in Paris, France, March 2012 and
  Sophia-Antipolis, France, November 2012 (configured for native).

PRELIMINARIES
-------------

- Get the Copper (Cu) CoAP user-agent from
  [https://addons.mozilla.org/en-US/firefox/addon/copper-270430](https://addons.mozilla.org/en-US/firefox/addon/copper-270430)

TMOTE SKY HOWTO
---------------

The CoAP example no longer fits in the limited ROM of the Tmote Sky.
Please use a platform with larger ROM instead.

NATIVE HOWTO
------------

With the target native you can test your CoAP applications without
constraints, i.e., with large buffers, debug output, memory protection, etc.
The plugtest-server is thought for the native platform, as it requires
an 1280-byte IP buffer and 1024-byte blocks.

        make TARGET=native plugtest-server
        sudo ./plugtest-server.native

Open new terminal

        make connect-native

- Start Copper and discover resources at coap://[fdfd::ff:fe00:10]:5683/
- You can enable the ETSI Plugtest menu in Copper's preferences

Under Windows/Cygwin, WPCAP might need a patch in
<cygwin>\usr\include\w32api\in6addr.h:

    21,23c21
    < #ifdef __INSIDE_CYGWIN__
    <     uint32_t __s6_addr32[4];
    < #endif
    ---
    >     u_int __s6_addr32[4];
    36d33
    < #ifdef __INSIDE_CYGWIN__
    39d35
    < #endif

DETAILS
-------

Erbium implements the Proposed Standard of CoAP. Central features are commented
in coap-example-server.c.  In general, coap supports:

- All draft-18 header options
- CON Retransmissions (note COAP_MAX_OPEN_TRANSACTIONS)
- Blockwise Transfers (note COAP_MAX_CHUNK_SIZE, see plugtest-server.c for
  Block1 uploads)
- Separate Responses (no rest_set_pre_handler() required anymore, note
  coap_separate_accept(), _reject(), and _resume())
- Resource Discovery
- Observing Resources (see EVENT_ and PERIODIC_RESOURCE, note
  COAP_MAX_OBSERVERS)

TODOs
-----

- Dedicated Observe buffers
- Optimize message struct variable access (directly access struct without copying)
- Observe client
- Multiple If-Match ETags
- (Message deduplication)
