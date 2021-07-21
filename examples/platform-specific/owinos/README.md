Owinos test examples
============================================

The following tests are valid for the OMote platform

Compile and install an example
-------------------

To flash either hardware platform use the same `TARGET=owinos` and the following:

* OMote : `BOARD=omote`

An example on how to compile is shown next:

`make TARGET=owinos BOARD=omote`

Or alternatively if you just type `make`, it will default to use the `BOARD=omote`.

To upload an example to your Owinos device, just add the `.upload` target as:

`make TARGET=owinos BOARD=omote owinos-demo.upload`

Optionally you can select a specific USB port to flash a given device, in Linux
and assuming there is a device at the `/dev/ttyUSB0`:

`make TARGET=owinos BOARD=omote owinos-demo.upload PORT=/dev/ttyUSB0`

If you omit the `PORT` argument, the system will flash all Owinos devices connected over USB.

Visualize the console output
-------------------

Just type `make login` to open a connection to the console via USB.
As above to specify a given port use the `PORT=/dev/ttyUSB0` argument.

Alternatively you can save the above `PORT`, `TARGET` or `BOARD` as follows:

`export TARGET=owinos BOARD=omote PORT=/dev/ttyUSB0`

This will save you to type these when running a command on the terminal

Documentation and guides
-------------------

More information about the platforms, guides and specific documentation can be found at [Oppila Microsystems Wiki][wiki]

[wiki]: https://www.oppila.in

