# Contiki-NG TrustZone example

This example contains two projects for the secure world and the normal
world. Each project is compiled and linked into a separate firmware
image, which are then merged into a single hex file for programming the
IoT device.

The only supported platform is the Nordic Semiconductor nRF5340. The
application processor runs the merged TrustZone firmware; the network
processor runs the IPC radio service which provides 802.15.4 radio
access. Radio operations from the normal world pass through the secure
world's NSC entry points, enabling communication policy enforcement at
the TrustZone boundary.

## Getting started

Run `make` to build the secure and normal world firmwares and merge
the hex files. The merged hex is placed in
`secure-world/build/nrf/nrf5340/dk/application/tz-merged.hex`.

The network core must also be flashed with the IPC radio service:

```sh
make -C ../ipc-radio-service TARGET=nrf BOARD=nrf5340/dk/network ipc-radio-service.upload
```

Then flash the merged TrustZone firmware:

```sh
make upload
```

A specific serial port can be chosen with `PORT=/dev/<port>`.

To see serial output:

```sh
make login PORT=/dev/<PORT>
```

## Running any Contiki-NG application in the normal world

Any Contiki-NG application built for `nrf5340/dk/application` can be
turned into a TrustZone normal world by passing `TRUSTZONE=1` on the
command line. This automatically:

- selects the normal-world linker script and `tz_radio_driver`,
- recursively (re)builds the secure world,
- links the normal world against the secure world's CMSE import library,
- merges both images into a `*.tz.hex` file,
- and redirects standard upload targets such as `%.upload` to flash that
  merged image.

The application core depends on the network core's IPC radio service,
so flash that first (one-time per board). For example, to run RPL UDP
`udp-server` as the normal world:

```sh
# Network core (one-time per board)
make -C examples/platform-specific/nrf/ipc-radio-service \
     TARGET=nrf BOARD=nrf5340/dk/network NRF_UPLOAD_SN=$SN \
     ipc-radio-service.upload

# Application core, with TrustZone
make -C examples/rpl-udp TARGET=nrf BOARD=nrf5340/dk/application \
     TRUSTZONE=1 NRF_UPLOAD_SN=$SN udp-server.upload
```

Available knobs (all optional except `TRUSTZONE` itself):

| Variable                       | Default                                                            | Purpose                                  |
|--------------------------------|--------------------------------------------------------------------|------------------------------------------|
| `TRUSTZONE`                    | unset                                                              | `1` enables the TrustZone build and merge |
| `TRUSTZONE_SECURE_WORLD_PATH`  | `$(CONTIKI)/examples/platform-specific/nrf/trustzone/secure-world` | Custom secure-world directory            |
| `TRUSTZONE_SECURE_FIRMWARE`    | `secure-world-example`                                             | Custom secure-world firmware name        |

Setting `TRUSTZONE=1` together with `TRUSTZONE_SECURE_BUILD=1` is an
error, since `TRUSTZONE=1` selects the normal world while
`TRUSTZONE_SECURE_BUILD=1` selects the secure world.

Note that the same `build/nrf/nrf5340/dk/application/` directory is
shared between TrustZone and non-TrustZone builds, so switching modes
in the same checkout currently requires removing the build directory
first (`rm -rf build`).

### Manual build (advanced)

If you need fine-grained control, you can still drive the build by
hand: build the secure world, build the normal world with
`TRUSTZONE_SECURE_BUILD=0`, and merge the two hex files with
`srec_cat`. The `TRUSTZONE=1` flag is simply a convenience that
performs these steps for you.

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
