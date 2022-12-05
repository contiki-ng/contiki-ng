# nrf: Nordic Semiconductor nRF5340 and nRF52840 (nRF MDK)

This guide's aim is to help you with using Contiki-NG for Nordic Semiconductor's nRF5340 and nRF52840 SoCs (using nRF MDK). 

This port supports the PCA10095 (nRF5340-DK), PCA10059 (nRF52840-DONGLE) and PCA10056 (nRF52840-DK) boards.

## Port Features

The following features have been implemented:
* Support for the 802.15.4 mode of the radio, including IPv6 using 6LoWPAN
* Support for both TSCH and CSMA
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

* nrfjprog for programming the nRF5340 DK, nRF52840 DK

nrfjprog is supplied as part of the nRF Command Line Tools and can be downloaded from the following link:

https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Command-Line-Tools

* nrfutil for programming the nRF52840 Dongle

nrfutil is available on PyPy: https://pypi.org/project/nrfutil/

A typical way to install this would be using pip: `pip3 install nrfutil`

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
  This is useful if you have more than one DK connected to your
  PC and wish to flash a particular device using the `.upload` target. 

* `BOARD={nrf5340/dk/application|nrf5340/dk/network|nrf52840/dk|nrf52840/dongle}`  
  Allows to specify if the which board and core (nrf5340 exclusive) is used.
  The default board is `nrf5340/dk/application`
  Dongle images are built with a bootloader-specific linker file and should be flashed using the `.dfu-upload` target.

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

In order to flash the application binary to a single nRF52840 Dongle use `<application>.dfu-upload`
as make target, e.g.: 

    make TARGET=nrf BOARD=nrf52840/dongle hello-world.dfu-upload PORT=/dev/ttyACM0

Where `PORT` is the name of the USB CDC-ACM port that the dongle is on.  
The bootloader can be activated by pressing the RESET button once, until the red LED begins to pulse.

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

The nRF5340 is a dual core SoC. It has two ARM Cortex M33 which are known as application and network core.

For the time being we only support one core running the OS but the SoC has shared memory access that can be used for inter-core communication.

Among the known limitations, the most important one is that only the network core has access to the radio but the network core does not start by default, the application core must start it. The nrf5340 application core will compile with the `nullradio_driver`

In order to start the network core there is a platform specific example, examples/platform-specific/nrf/start-network-core.

    make TARGET=nrf BOARD=nrf5340/dk/application start-network-core.upload

Once the application core contains this example. The hello-world can be uploaded to the network core.

    make TARGET=nrf BOARD=nrf5340/dk/network hello-world.upload

The start-network-core will forward the UART, Buttons, LEDs. If extra GPIO's are needed in the network core, it must be forwarded in the start-network-core.

## Support

For bug reports or/and suggestions please open a github issue.

## License

All files in the port are under BSD license. The nrfx is licensed on a separate terms.

## Resources

* nRF documentation (http://infocenter.nordicsemi.com)
* JLink Tools (https://www.segger.com/)
