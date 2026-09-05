# nrf: Nordic Semiconductor nRF5340 and nRF52840 (nRF MDK)

This guide's aim is to help you with using Contiki-NG for Nordic Semiconductor's nRF5340 and nRF52840 SoCs (using nRF MDK). 

This port supports the PCA10095 (nRF5340-DK), PCA10059 (nRF52840-DONGLE) and PCA10056 (nRF52840-DK) boards. It also provides the M33 application-core side of the nRF54L15 (PCA10156 DK and Seeed XIAO nRF54L15); see the [nRF54L15](#nrf54l15) section below.

## Port Features

The following features have been implemented:
* Support for the 802.15.4 mode of the radio, including IPv6 using 6LoWPAN
* Support for both TSCH and CSMA (TSCH availability varies by SoC; see the
  per-SoC sections below)
* No dependency on the nRF5 SDK
* Contiki-NG system clock and rtimers
* UART driver
* Watchdog driver
* RNG driver seeded from the hardware RNG
* Temperature sensor driver
* DK/Dongle LED and Button driver

Note that this port does not support BLE.

The port is organized as follows:
* nRF CPU drivers are located in the `arch/cpu/nrf` folder.
* The nrfx is located in the `arch/cpu/nrf/lib/nrfx` folder. This will be installed automatically as a git submodule.
* nRF boards configuration are located in the `arch/platform/nrf/<SoC>/<Board>/<Core (nRF5340 Exclusive)>/` folder.
  * SoC's: nrf5340, nrf52840
  * Boards:
    * nrf5340: dk 
    * nrf52840: dk, dongle
  * Cores:
    * nrf5340: application, network

## Prerequisites and Setup

In order to compile for the nRF5340 and nRF52840 platforms you'll need:

* An ARM compatible toolchain

The toolchain used to build Contiki-NG is arm-gcc, also used by other arm-based Contiki-NG ports.

If you use the docker image or the vagrant image, this will be pre-installed for you. Otherwise, depending on your system, please follow the respective installation instructions ([native Linux](/doc/getting-started/Toolchain-installation-on-Linux.md) / [native mac OS](/doc/getting-started/Toolchain-installation-on-macOS.md))

* GNU make

* nrfutil for programming the nRF5340 DK, nRF52840 DK and nRF52840 Dongle

nrfutil (the Rust-based binary, which replaces the end-of-life nrfjprog and
the deprecated Python nrfutil from PyPI) can be downloaded from:

https://www.nordicsemi.com/Products/Development-tools/nRF-Util

After downloading the binary, install the required subcommands:

    nrfutil install device

For flashing the nRF52840 Dongle, the nRF5 SDK tools are also needed:

    nrfutil install nrf5sdk-tools

* jq

The `.upload` targets use [jq](https://jqlang.org/) to select the attached
device to program: filtering devkits by device family and looking up a
device by `PORT=<serial port>` both require it. Without jq, uploads that
need such a lookup fail with an error asking for an explicit
`NRF_UPLOAD_SN=<serial number>`.

## Getting Started

Once all tools are installed it is recommended to start by compiling 
and flashing `examples/hello-world` application. This allows to verify 
that toolchain setup is correct.

To compile the example, go to `examples/hello-world` and execute:

    make TARGET=nrf

If the compilation is completed without errors flash the board:

    make TARGET=nrf hello-world.upload

## Examples

This target supports all the common IPv6 examples available under the `examples/` folder.

## Compilation Options

The Contiki-NG TARGET name for this port is `nrf`, so in order to compile 
an application you need to invoke GNU make as follows:

    make TARGET=nrf

In addition to this port supports the following variables which can be
set on the compilation command line:

* `NRF_UPLOAD_SN=<serial number>`  
  Allows to choose a particular DK by its serial number (printed on the label).  
  This is optional when exactly one DK of the targeted device family is
  attached (devices of other families are ignored). With more than one
  matching device connected, the `.upload` target refuses to program and
  lists the attached serial numbers; select one with
  `NRF_UPLOAD_SN=<serial number>` or use the `.upload-all` target to flash
  all attached devices of the targeted family. The same applies to the
  nRF52840 Dongle: with several dongles attached, select one with
  `NRF_UPLOAD_SN` (the dongle keeps its USB serial number in both firmware
  and bootloader mode).

* `NRF_RECOVER=1`  
  Runs `nrfutil device recover` on the selected device immediately before
  programming. Use this to unlock a device that has enabled access port
  protection (e.g. locked itself). **Warning: recovery performs a full chip
  erase of the selected core.** It is therefore never done by default and
  only when `NRF_RECOVER=1` is explicitly given on the make command line.
  Works with both the `.upload` and `.upload-all` targets.

* `PORT=<serial port>`  
  Selects the device to program by one of its serial ports (as listed by
  `tools/motelist` or `make motelist-all`), when `NRF_UPLOAD_SN` is not
  given. Works for both DKs and dongles. Since `PORT` is also used by the
  `login` target, the same variable selects the node for both programming
  and serial output. Only a `PORT` given on the make command line selects
  the device to program; a `PORT` from the environment is ignored for
  device selection (but still used by `login`).

* `BOARD={nrf5340/dk/application|nrf5340/dk/network|nrf52840/dk|nrf52840/dongle}`  
  Allows to specify if the which board and core (nrf5340 exclusive) is used.
  The default board is `nrf5340/dk/application`
  Dongle images are built with a bootloader-specific linker file and are flashed via the Nordic open bootloader (DFU) by the regular `.upload` target.

* `NRF_NATIVE_USB=<0,1>`  
  Enables or disables the native USB support on boards that have USB support. 
  This will automatically change the debug and the slip from UART to USB.
 
## Compilation Targets

Invoking make solely with the `TARGET` variable set will build all
applications in a given folder. A particular application can be built
by invoking make with its name as a compilation target:

    make TARGET=nrf hello-world 

In order to flash the application binary to a single nRF5340 DK board in the application core use `<application>.upload`
as make target, e.g.: 

    make TARGET=nrf hello-world.upload

In order to flash the application binary to all attached nRF5340 DK board in the application core use `<application>.upload-all`
as make target, e.g.: 

    make TARGET=nrf hello-world.upload-all

The `.upload-all` target also works for nRF52840 Dongles: it triggers DFU
mode on and flashes all attached dongles, e.g.:

    make TARGET=nrf BOARD=nrf52840/dongle hello-world.upload-all

Flashing a single nRF52840 Dongle uses the regular `.upload` target, e.g.:

    make TARGET=nrf BOARD=nrf52840/dongle hello-world.upload

The upload automatically triggers the DFU bootloader on a dongle running
Contiki-NG firmware. For a dongle running other firmware, the bootloader
can be activated manually by pressing the RESET button once, until the red
LED begins to pulse. With more than one dongle attached, select one with
`NRF_UPLOAD_SN=<serial number>` or `PORT=<serial port>`.

Notes when using the nRF dongle: 
* The serial output from the dongle can be accessed by attaching a USB to Serial converter to the pins described on the back of the board.
* If `nrfutil` returns an error such as `LIBUSB_ERROR_ACCESS` when attempting to perform a DFU trigger the following udev rules might be required:
```
## Set /dev/bus/usb/*/* as read-write for all users (0666) for Nordic Semiconductor devices
SUBSYSTEM=="usb", ATTRS{idVendor}=="1915", MODE="0666"
``` 

To remove all build results invoke:

    make TARGET=nrf clean

### nRF5340

The nRF5340 is a dual-core SoC with two ARM Cortex-M33 processors known as the application core and the network core. Only the network core has access to the 802.15.4 radio peripheral.

#### IPC Radio Driver (Recommended)

The recommended approach for running Contiki-NG on the nRF5340 is to use the IPC radio driver. This lets the application core run the full Contiki-NG networking stack (6LoWPAN, RPL, CoAP, etc.) while the network core acts as a dedicated radio service. The two cores communicate over a shared memory region using an IPC protocol.

**Architecture:**
* The **application core** runs the user application and full network stack. Radio operations are forwarded to the network core via IPC shared memory.
* The **network core** runs a minimal Contiki-NG image (`ipc-radio-service`) with an interrupt-driven IPC MAC that forwards received frames and handles software ACKs. Between events, the CPU sleeps to minimize energy consumption.
* Shared memory is located at `0x20070000` (in the application core's RAM1 region, accessible from both cores).
* Debug output from the network core is forwarded to the application core via a log ring buffer and printed with a `[NET]` prefix.

**Building and flashing (both cores must be programmed):**

1. Build and flash the network core radio service:

        make -C examples/platform-specific/nrf/ipc-radio-service TARGET=nrf BOARD=nrf5340/dk/network ipc-radio-service.upload

2. Build and flash any standard application on the application core (e.g., `rpl-udp`). Set the radio driver to `ipc_radio_driver`:

        make -C examples/rpl-udp TARGET=nrf BOARD=nrf5340/dk/application \
             DEFINES=NETSTACK_CONF_RADIO=ipc_radio_driver udp-server.upload

   Alternatively, add `#define NETSTACK_CONF_RADIO ipc_radio_driver` to the application's `project-conf.h`.

**Serial output** appears on VCOM2 (`/dev/ttyACM2` on Linux) at 115200 baud.

**Limitations:** TSCH is not supported over the IPC radio driver. The
synchronous command/response design and the latency of crossing the IPC
boundary make it unsuitable for TSCH's tight timing requirements. Use
CSMA as the MAC layer (the default).

See `arch/cpu/nrf/net/README.md` for the full IPC protocol specification.

#### TrustZone Secure Radio

The IPC radio driver can optionally run in the ARM TrustZone secure
world, providing a hardware-enforced security boundary for radio
communication. In this mode, the normal world accesses the radio
through Non-Secure Callable (NSC) entry points that validate all
pointers via CMSE. This enables communication policies to be enforced
in the secure world.

**Building for TrustZone:**

1. Build the secure world and the network core radio service:

        make -C examples/platform-specific/nrf/trustzone/secure-world
        make -C examples/platform-specific/nrf/ipc-radio-service TARGET=nrf BOARD=nrf5340/dk/network

2. Build the normal-world application:

        make -C examples/rpl-udp TARGET=nrf BOARD=nrf5340/dk/application \
             TRUSTZONE_SECURE_BUILD=0 \
             TRUSTZONE_SECURE_WORLD_PATH=../../examples/platform-specific/nrf/trustzone/secure-world \
             udp-server

   The `tz_radio_driver` is selected automatically in the normal-world build.

3. Merge the secure and normal world hex files:

        srec_cat \
          examples/platform-specific/nrf/trustzone/secure-world/build/nrf/nrf5340/dk/application/secure-world-example.hex -Intel \
          examples/rpl-udp/build/nrf/nrf5340/dk/application/udp-server.hex -Intel \
          -o merged.hex -Intel

4. Flash the network core and merged application core.

See `examples/platform-specific/nrf/ipc-radio-service/README.md` for
the complete step-by-step instructions.

#### GPIO Forwarding (Legacy)

An alternative approach is to run the full OS on the network core and use the application core only to start the network core and forward GPIO pins. This is provided by the `start-network-core` example:

    make TARGET=nrf BOARD=nrf5340/dk/application start-network-core.upload

Once the application core contains this example, a Contiki-NG application can be uploaded to the network core:

    make TARGET=nrf BOARD=nrf5340/dk/network hello-world.upload

The `start-network-core` example forwards UART, buttons, and LEDs. If extra GPIOs are needed on the network core, they must be forwarded in `start-network-core`.

### nRF54L15

The nRF54L15 is an ARM Cortex-M33 SoC (128 MHz, 1536 KB RRAM, 256 KB RAM) with
a 2.4 GHz radio supporting 802.15.4 and Bluetooth LE 5.4. As with the other
boards in this port, its application core runs the **full Contiki-NG networking
stack** — 802.15.4, 6LoWPAN/IPv6, RPL, and CSMA — so the standard IPv6 examples
build and run on it. (As elsewhere in this port, BLE is not supported.) The SoC
also carries a RISC-V coprocessor, the FLPR, with its own Contiki-NG port; see
*FLPR coprocessor* below.

Two boards are supported, selected with `BOARD`:

* `nrf54l15/dk` — nRF54L15-DK (PCA10156)
* `nrf54l15/xiao` — Seeed XIAO nRF54L15

Build and flash any standard example as usual:

    make TARGET=nrf BOARD=nrf54l15/xiao hello-world.flash

**Radio submodule.** The 802.15.4 driver wraps Nordic's `nrf_802154` library,
which ships in the `sdk-nrfxlib` git submodule. Initialise submodules before
building or the link fails with missing networking symbols:

    git submodule update --init --recursive

**Flashing.** Unlike the nRF5340/nRF52840 DKs (which use nrfjprog/J-Link), the
two nRF54L15 boards are flashed with the `.flash` target:

* `nrf54l15/xiao` — over its onboard CMSIS-DAP using **stock OpenOCD 0.12.0+**.
  No nRF54L15-specific OpenOCD flash driver is required: the board config writes
  RRAM with `load_image` and never invokes an OpenOCD flash driver. The
  board-specific `openocd.cfg` is selected automatically by the Makefile.
* `nrf54l15/dk` — over its onboard SEGGER J-Link.

**Current limitations.** TSCH is not supported on the nRF54L15. The
`nrf_802154`-based radio driver does not implement
`RADIO_PARAM_LAST_PACKET_TIMESTAMP`, which TSCH requires for time
synchronisation, so TSCH aborts during initialisation with:

    [ERR : TSCH] ! radio does not support getting last packet timestamp. Abort init.

Use CSMA as the MAC layer (the default). Low-power modes, the watchdog driver,
and the temperature sensor are also not yet supported on the nRF54L15.

#### FLPR coprocessor

The **FLPR** (Fast Lightweight Peripheral Processor) is a RISC-V VPR coprocessor
(RV32EMC) on the nRF54L15. It has its own Contiki-NG port, built with a separate
`nrf-vpr` target: the M33 image embeds and launches a small FLPR kernel (process
scheduler, etimer/ctimer, GRTC-driven `clock_time()`, GPIO output) while the
802.15.4 radio stays on the M33.

For the full guide — toolchain setup (the FLPR needs an RV32EMC RISC-V GCC),
build/deploy steps, the boot sequence, and the `hello-vpr` / `flpr-host`
examples — see the [nrf-vpr platform documentation](nrf-vpr.md).

### SPI

The nRF port implements the Contiki-NG SPI HAL (`os/dev/spi.h`) on top of
`nrfx_spim`. It is opt-in: set `NRF_WITH_SPI=1` and list the SPIM instance ids
in `NRF_SPI_INSTANCES`, in logical controller order. Builds that do not ask for
SPI are unaffected.

Instance choice is constrained on SoCs that group peripherals into SERIAL
slots. SPIMn and UARTEn in the same slot share an interrupt vector, so
selecting the one that collides with the console UART fails at link time with a
duplicate `SERIALn_IRQHandler`. Known-good choices are documented in
`arch/cpu/nrf/dev/spi-arch.h`: SPIM00 or SPIM22 on the nRF54L15, SPIM1 and up
on the nRF5340 application core, and any instance on the nRF52840.

`examples/platform-specific/nrf/spi-flash` brings the bus up against the
nRF54L15 DK's on-board flash, which is useful when wiring a new peripheral:
it fails on the driver rather than on your jumper wires.

### Ethernet and IPv4 (IP64)

With an ENC28J60 module on SPI, an nRF board can act as a self-contained
border router — RPL root on the 802.15.4 side, NAT64 and DNS64 on the IPv4
side, with no host in the data path. See the
[ENC28J60 / IP64 documentation](nrf-ip64-ethernet.md) for wiring, the GPIO
voltage change the nRF54L15 DK needs first, and a bring-up sequence.

## Support

For bug reports or/and suggestions please open a github issue.

## License

All files in the port are under BSD license. The nrfx is licensed on a separate terms.

## Resources

* nRF documentation (http://infocenter.nordicsemi.com)
* JLink Tools (https://www.segger.com/)
