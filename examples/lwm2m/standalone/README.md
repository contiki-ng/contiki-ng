OMA LWM2M Standalone Example
====================================

This is an example of how to make use of the OMA LWM2M and CoAP
implementation from Contiki in a native application.

The Makefile will copy necessary files from Contiki to the subfolder
```oma-lwm2m-src``` and then compile the example ```lwm2m-example```
as a native application. By copying only the needed source files,
the example can be used outside the Contiki source tree.

### Running the OMA LWM2M example

```bash
cd contiki/examples/oma-lwm2m/standalone
make
./lwm2m-example
```

The example application will start a CoAP server listening on
localhost port 5683 with an example OMA LWM2M device object.


### Testing the OMA LWM2M example

The Copper (Cu) Firefox addon can be used to access the LWM2M example.

1. Start the OMA LWM2M example as described above.

2. Get the Copper (Cu) CoAP user-agent from
[https://addons.mozilla.org/en-US/firefox/addon/copper-270430](https://addons.mozilla.org/en-US/firefox/addon/copper-270430).

3. Start Copper and discover resources at coap://127.0.0.1:5683/
   It should find three objects named 0, 1, and 3.

4. Browse to the device type resource at coap://127.0.0.1:5683/3/0/17
   (object 3, instance 0, resource 17) and then click ```GET```.
   As device type the LWM2M example should return the text "lwm2m-example".

5. Browse to the time resource at coap://127.0.0.1:5683/3/0/13
   Every time you click ```GET```, it should return the number of seconds
   since the example application was started.

### Moving the example outside Contiki

```bash
cd contiki/examples/oma-lwm2m/standalone
make copy
```

Copy the example directory contents to a directory outside Contiki.
Remove the Makefile ```Makefile.contiki``` and the remaining Makefile
will compile the example independent of the Contiki source tree.


The Hex Transport can be tested together with DTLS using:

```bash
make clean
make TRANSPORT=hex MAKE_WITH_DTLS=1
javac Hex2UDP.java
java Hex2UDP leshan.eclipse.org 5684 ./lwm2m-example
```

Note that you need to configure the Leshan server with the correct
key and ID.

(without DTLS it should be 5683 for CoAP)
