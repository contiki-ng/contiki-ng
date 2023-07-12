# gecko: Contiki-NG for Silicon Labs (using GSDK)
This guide's aim is to help you with using Contiki-NG for Silicon Labs 
(using Gecko SDK). 

This port supports the efr32mg12p332f1024gl125 opn and the brd4162a and brd4166a boards.

## Port Features
The following features have been implemented:
* Support for the 802.15.4 mode of the radio, including IPv6 using 6LoWPAN
* Support CSMA and TSCH
* Contiki-NG system clock and rtimers
* UART driver
* Watchdog driver
* RNG driver seeded from radio
* LED and Button driver
* Coffee File System

The port is organized as follows:
* Silicon Labs CPU drivers are located in the `arch/cpu/gecko` folder.
* The Gecko SDK is located in the `arch/cpu/gecko/lib/gecko_sdk` folder. 
  * This will be installed automatically as a git submodule.
* Silicon Labs boards configuration are located in the `arch/platform/gecko/<Board>` folder.
  * OPNs's: 
    * efr32mg12p332f1024gl125
  * Boards:
    * brd4162a
    * brd4166a

## Prerequisites and Setup
In order to compile need:

* An ARM compatible toolchain
  * The toolchain used to build Contiki-NG is arm-gcc, also used by other arm-based Contiki-NG ports.
  * If you use the docker image or the vagrant image, this will be pre-installed for you. Otherwise, depending on your system, please follow the respective installation instructions ([native Linux](/doc/getting-started/Toolchain-installation-on-Linux.md) / [native macOS](/doc/getting-started/Toolchain-installation-on-macOS.md)).

* GNU make

* Simplicity Commander for programming
  * An article can be found here: [Simplicity Commander](https://community.silabs.com/s/article/simplicity-commander?language=en_US)

* Git LFS
  * The Gecko SDK includes some pre-compiled libraries, [Rail Libary](https://docs.silabs.com/rail/latest/api-index), needed for the radio driver. These libraries are checked in using Git LFS to improve the checkout performance.
  
## Getting Started
Once all tools are installed it is recommended to start by compiling 
and flashing `examples/hello-world` application. This allows to verify 
that toolchain setup is correct.

To compile the example, go to `examples/hello-world` and execute:

    make TARGET=gecko

If the compilation is completed without errors flash the board:

    make TARGET=gecko hello-world.upload

## Examples
This target supports all the common IPv6 examples available under the `examples/` folder.

## Compilation Options
The Contiki-NG TARGET name for this port is `gecko`, so in order to compile 
an application you need to invoke GNU make as follows:

    make TARGET=gecko

In addition to this port supports the following variables which can be
set on the compilation command line:

* `DEVICE_SERIAL_NO=<serial number>`  
  Allows to choose a particular WSTK by its serial number 
  (available on the display of the WSTK).
  This is useful if you have more than one board connected to your
  PC and wish to flash a particular device using the `.upload` target. 

* `DEVICE_IP_ADDRESS=<ip address>`  
  Allows to choose a particular DK by its ip address 
  (available on the display of the WSTK).
  This is useful if you have more than one board connected to your
  network and wish to flash a particular device using the `.upload` target.

* `BOARD={brd4162a/brd4166a}`
  Allows to specify if the which board is used.
  The default board is `brd4162a`

## Support
For bug reports or/and suggestions please open a github issue.

## License
All files in the port are under BSD license. 
The Gecko SDK and auto generated files are licensed on a separate terms.

## Resources
* Gecko SDK (https://github.com/SiliconLabs/gecko_sdk)
* Rail Library (https://docs.silabs.com/rail/latest/api-index)
* Git LFS (https://github.com/git-lfs/git-lfs)
* JLink Tools (https://www.segger.com/)
