LWM2M Standalone Example
====================================

This is an example of how to make use of the OMA LWM2M and CoAP
implementation from Contiki-NG in a native application.

The Makefile will copy necessary files from Contiki-NG to the subfolder
```lwm2m-src``` and then compile the example ```lwm2m-example```
as a native application. By copying only the needed source files,
the example can be used outside the Contiki-NG source tree.

### Running the LWM2M example

```bash
cd contiki/examples/lwm2m/standalone
make
./lwm2m-example
```

The example application will start a CoAP server listening on localhost port
5683 with some example LWM2M objects. By default, the example application will
also register itself with the Leshan server at leshan.eclipse.org. To specify
a different LWM2M server:
`./lwm2m-example coap://<server host address> <endpoint name>`.

For example to connect to a locally running LWM2M server:

```bash
./lwm2m-example coap://127.0.0.1/ example-endpoint-name
```

### Running the LWM2M example with DTLS

The example currently only supports PSK and the default credentials can be
changed in the file `lwm2m-example.c`.

```c
#define PSK_DEFAULT_IDENTITY "Client_identity"
#define PSK_DEFAULT_KEY      "secretPSK"
```

To compile with DTLS support and connect to a local LWM2M server with matching
credentials configured:

```bash
cd contiki/examples/lwm2m/standalone
make clean
make MAKE_WITH_DTLS=1
./lwm2m-example coaps://127.0.0.1
```

### Moving the example outside Contiki-NG

```bash
cd contiki/examples/lwm2m/standalone
make copy
```

Copy the example directory contents to a directory outside Contiki-NG.
Remove the Makefile ```Makefile.contiki``` and the remaining Makefile
will compile the example independent of the Contiki-NG source tree.


### Running the LWM2M examle with HEX transport

The Hex Transport can be tested together with DTLS using:

```bash
make clean
make TRANSPORT=hex MAKE_WITH_DTLS=1
javac Hex2UDP.java
java Hex2UDP leshan.eclipse.org 5684 ./lwm2m-example
```

Note that you need to configure the Leshan server with the correct key and ID.

(without DTLS it should be 5683 for CoAP).
