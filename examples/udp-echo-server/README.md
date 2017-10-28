UDP echo server example
=======================
This example demonstrates how to implement a simple UDP-server by
using the original UDP API (as opposed to using the simple-udp module).

It is anticipated that this example will run on all supported platforms.

To test, compile and programme your device (or execute on the native
platform). Then you can use netcat, like so:

    $ nc -6u <your device's IPv6 address> 3000

Type something in your console and you should see it echoed back by the
UDP server running on your device.
