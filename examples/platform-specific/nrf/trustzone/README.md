# Contiki-NG TrustZone example

This example contains two different projects for the secure-world and
the normal-world. Each project are compiled and linked into separate
firmware images, which are then merged into a single hex filed that is
used to program the IoT device.

At the moment, the only supported platform is the Nordic Semiconductor
nRF54L15. This phase-1 port only validates split secure/non-secure application images on the application core and does not yet claim full SPU/MPC attribution coverage for all peripherals.

Current hardware status:
- secure world boots and hands off to non-secure world
- the normal-world example is intentionally limited to a minimal hello-world path via `tz_api_println()`
- the secure image owns the real nRF54L15 radio
- the non-secure image uses a TrustZone radio proxy and currently prints `non-secure radio ready on channel 26`
- over-the-air packet TX/RX is not validated yet

Notes:
- the generic `trustzone` example is kept minimal on purpose while the nRF54L15 TrustZone port is still being stabilized
- timer- and radio-heavy validation should use `examples/platform-specific/nrf/trustzone/rpl-udp`

For the current port state and the radio work plan, see:
- `doc/nrf54l15-trustzone-solution.md`
- `doc/nrf54l15-trustzone-port.md`
- `doc/nrf54l15-trustzone-radio-plan.md`

There is also a TrustZone wrapper for a real IPv6/RPL application at:
- `examples/platform-specific/nrf/trustzone/rpl-udp`

## Getting started

Run `make` to build secure and normal world firmwares, and
merge the hex files.

Run `make clean` to remove the secure and normal world builds.

To flash to the nRF54L15 XIAO, run `make upload`. A specific serial port can
be chosen by adding `PORT=/dev/<port>` as an argument on the command line.

Optionally, one can change directory into secure-world and run:
```sh
make TARGET=nrf BOARD=nrf54l15/xiao tz-merged.upload PORT=/dev/<PORT>
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
this case we target the nRF54L15).
    ```sh
    JLinkGDBServer -device NRF54L15_XXAA -if swd -port 2331
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
