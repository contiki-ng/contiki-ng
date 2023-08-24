# Contiki-NG TrustZone example

This example contains two different projects for the secure-world and
the normal-world. Each project are compiled and linked into separate
firmware images, which are then merged into a single hex filed that is
used to program the IoT device.

At the moment, the only supported platform is the Nordic Semiconductor
nRF5340. This platform has an application processor that runs the
merged TrustZone firmware, and a network processor which handles
communication. There is currently no support for the network processor
in Contiki-NG when running with TrustZone enabled, however.

## Getting started

Run `make` to build secure and normal world firmwares, and
merge the hex files.

Run `make clean` to remove the secure and normal world builds.

To flash to the nRF5340, run `make upload`. A specific serial port can
be chosen by adding `PORT=/dev/<port>` as an argument on the command line.

Optionally, one can change directory into secure-world and run:
```sh
make TARGET=nrf BOARD=nrf5340/dk/application tz-merged.upload PORT=/dev/<PORT>
```

To login and see serial output from an IoT device on a particular serial port:
```sh
make login PORT=/dev/<PORT>
```

## GDB setup for nRF (Linux)

Install the prerequisites for GDB if not already installed. For example,
you need nRF Command Line (nrfjprog), SEGGER J-Link,
GNU Arm Embedded toolchain, etc. These can be installed by following the
instructions in [contiki-nrf](https://docs.contiki-ng.org/en/develop/doc/platforms/nrf.html#prerequisites-and-setup).

1. Install gdb-multiarch (should already be installed with the GNU Arm embedded toolchain)
    ```sh
    sudo apt-get update -y
    sudo apt-get install gdb-multiarch
    ```
2. Compile the firmwares with debug option flags (e.g., `-O0 -ggdb2 -g2`)
to create debug symbols.

3. Open a JLinkGDBServer to allow connections from the GDB client (In
this case we target the nRF5340).
    ```sh
    JLinkGDBServer -device nrf5340_xxaa -if swd -port 2331
    ```
    * `-device` nrfxx_xxaa (What type of nrf device)
    * `-if` specifies the debug interface
    * `-port` which port to use

4. In another terminal, start gdb-multiarch:
    ```sh
    gdb-multiarch example.FILE
    ```
    * `file` could for example be .ELF or .out etc.

5. In GDB, connect to the GDB server:
    ```sh
    target remote localhost:2331
    ```

It can be good to turn off the uarte_write loop, so it is possible to
read other things.
