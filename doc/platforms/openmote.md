# openmote: OpenMote-CC2538 and OpenMote-B

The OpenMote platform is based on TI's CC2538 SoC (System on Chip), featuring an ARM Cortex-M3 running at 16/32 MHz and with 32 kbytes of RAM and 256/512 kbytes of FLASH. It has the following key features:

  * Standard Cortex M3 peripherals (NVIC, SCB, SysTick)
  * Sleep Timer (underpins rtimers)
  * SysTick (underpins the platform clock and Contiki-NG's timers infrastructure)
  * 2.4 GHz radio interface
  * UART
  * Watchdog (in watchdog mode)
  * USB (in CDC-ACM)
  * uDMA Controller (RAM to/from USB and RAM to/from RF)
  * Random number generator
  * Low Power Modes
  * General-Purpose Timers
  * ADC
  * Cryptoprocessor (AES-ECB/CBC/CTR/CBC-MAC/GCM/CCM-128/192/256, SHA-256)
  * Public Key Accelerator (ECDH, ECDSA)
  * Flash-based port of Coffee
  * PWM
  * Built-in core temperature and battery sensor

The OpenMote platform supports two boards:
* The original OpenMote (`openmote-cc2538`),
* The new OpenMote revision B (`openmote-b`). This board features two radios: TI's CC2538 and Atmel's AT86RF215.

## Port Features

The port is organized as follows:
* the drivers for CC2538 CPU are located in the `arch/cpu/cc2538` folder,
* the OpenMote port is located in the `arch/platform/openmote` folder where:
  * `dev` contains drivers for various sensors,
  * `openmote-cc2538` includes configuration for the OpenMote-CC2538 board,
  * `openmote-b` includes configuration for the OpenMote-B board.
* the driver for AT86RF215 radio (available only on OpenMote-B) is located in `arch/dev/radio/at86rf215`.

## Prerequisites and Setup

To start using Contiki-NG with the OpenMote, the following is required:

 * An OpenMote board: OpenMote-B or OpenMote-CC2538 with a OpenUSB, OpenBase or OpenBattery carrier boards.
 * A toolchain to compile Contiki-NG for the CC2538.
 * Drivers so that your OS can communicate with your hardware.
 * Software to upload images to the CC2538.

### Toolchain Installation

The toolchain used to build Contiki-NG is arm-gcc, also used by other arm-based Contiki-NG ports.

If you use the docker image or the vagrant image, this will be pre-installed for you. Otherwise, depending on your system, please follow the respective installation instructions ([native Linux](/doc/getting-started/Toolchain-installation-on-Linux) / [native mac OS](/doc/getting-started/Toolchain-installation-on-macOS)).

### Software to Program the Nodes

The OpenMote nodes can be programmed via the jtag interface or via the serial boot loader on the chip.

The OpenMote boards have a mini JTAG 10-pin male header, compatible with the `SmartRF06` development board, which can be used to flash and debug the platforms. Alternatively one could use the `JLink` programmer with a 20-to-10 pin converter like the following: <https://www.olimex.com/Products/ARM/JTAG/ARM-JTAG-20-10/>. An experimental example for OpenOCD with `Olimex` programmer can be found [here](https://github.com/9morano/contiki-ng/tree/openmote/arch/platform/openmote/openmote-b)

For the OpenMote-CC2538 boards, the serial boot loader on the chip is exposed to the user via the USB interface of both the OpenUSB and the OpenBase carrier boards. The OpenUSB carrier board is capable to automatically detect the boot loader sequence and flash the CC2538 without user intervention. The OpenBase carrier board does not have such feature, so to activate the boot loader the user needs to short the ON/SLEEP pin to GND and then press the reset button. To activate the serial boot loader on OpenMote-B board, the user needs to short the PA7 pin to GND and then press the reset button.

Instructions to flash for different OS are given below.

* On Windows:
    * Nodes can be programmed with TI's ArmProgConsole or the SmartRF Flash Programmer 2. The README should be self-explanatory. With ArmProgConsole, upload the file with a `.bin` extension. (jtag + serial)
    * Nodes can also be programmed via the serial boot loader in the cc2538. In `tools/cc2538-bsl/` you can find `cc2538-bsl.py` python script, which can download firmware to your node via a serial connection. If you use this option you just need to make sure you have a working version of python installed. You can read the README in the same directory for more info. (serial)

* On Linux:
    * Nodes can be programmed with TI's UniFlash tool. With UniFlash, use the file with `.openmote` extension. (jtag + serial)
    * Nodes can also be programmed via the serial boot loader in the cc2538 through the `cc2538-bsl.py` python script. No extra software needs to be installed. (serial)

* On OSX:
    * The `cc2538-bsl.py` script in `tools/cc2538-bsl/` is the only option. No extra software needs to be installed. (serial)

## Getting Started

Once the tools are installed, you can start with the platform-independent `examples/hello-world` example. For a review of `make` targets see [doc:build-system].

To build the example, go to the `examples/hello-world` and execute:

    make TARGET=openmote

This will build for the default board `openmote-cc2538`. To switch between boards, use the `BOARD` variable. For example:

    make TARGET=openmote BOARD=openmote-cc2538

or:
    
    make TARGET=openmote BOARD=openmote-b

If you want to upload the compiled firmware to a node via the serial boot loader you need first to manually enable the boot loader. Then use:

    make TARGET=openmote BOARD=openmote-cc2538 hello-world.upload
    
To disable the boot loader and start the Contiki-NG, remove the shorting connection and restart the node.

If you are compiling for the OpenMote-CC2538 Rev.A1 board (CC2538SF53, 256 KB Flash) you have to pass `BOARD_REVISION=REV_A1` in all your `make` commands to ensure that the linking stage configures the linker script with the appropriate parameters. If you are compiling for older OpenMote-CC2538 revisions (CC2538SF53, 512 KB Flash) you can skip this parameter since the default values are already correct.

The `PORT` argument could be used for `make` command to specify on which port the device is connected, in case you have multiple devices connected at the same time.

To enable printing debug output to your console, use the `make login` to get the information over the USB programming/debugging port, or alternatively use `make serialview` to also add a timestamp in each print.

## Node IEEE and IPv6 Addresses

Nodes will generally autoconfigure their IPv6 address based on their IEEE address. The IEEE address can be read directly from the CC2538 Info Page, or it can be hard-coded. Additionally, the user may specify a 2-byte value at build time, which will be used as the IEEE address' 2 LSBs.

To configure the IEEE address source location (Info Page or hard-coded), use the `IEEE_ADDR_CONF_HARDCODED` define in contiki-conf.h:

* 0: Info Page
* 1: Hard-coded

If `IEEE_ADDR_CONF_HARDCODED` is defined as 1, the IEEE address will take its value from the `IEEE_ADDR_CONF_ADDRESS` define. If `IEEE_ADDR_CONF_HARDCODED` is defined as 0, the IEEE address can come from either the primary or secondary location in the Info Page. To use the secondary address, define `IEEE_ADDR_CONF_USE_SECONDARY_LOCATION` as 1.

Additionally, you can override the IEEE's 2 LSBs, by using the `NODEID` make variable. If `NODEID` is not defined, `IEEE_ADDR_NODE_ID` will not get defined either. For example:

    make NODEID=0x79ab

This will result in the 2 last bytes of the IEEE address getting set to 0x79 0xAB

Note: Some early production devices do not have am IEEE address written on the Info Page. For those devices, using value 0 above will result in a Rime address of all 0xFFs. If your device is in this category, define `IEEE_ADDR_CONF_HARDCODED` to 1 and specify `NODEID` to differentiate between devices.

## Low-Power Modes

The CC2538 port supports power modes for low energy consumption. The SoC will enter a low power mode as part of the main loop when there are no more events to service.

LPM support can be disabled in its entirety by setting `LPM_CONF_ENABLE` to 0 in `contiki-conf.h` or `project-conf.h`.

The Low-Power module uses a simple heuristic to determine the best power mode, depending on anticipated Deep Sleep duration and the state of various peripherals.

In a nutshell, the algorithm first answers the following questions:

* Is the RF off?
* Are all registered peripherals permitting PM1+?
* Is the Sleep Timer scheduled to fire an interrupt?

If the answer to any of the above question is "No", the SoC will enter PM0. If the answer to all questions is "Yes", the SoC will enter one of PMs 0/1/2 depending on the expected Deep Sleep duration and subject to user configuration and application requirements.

At runtime, the application may enable/disable some Power Modes by making calls to `lpm_set_max_pm()`. For example, to avoid PM2 an application could call `lpm_set_max_pm(1)`. Subsequently, to re-enable PM2 the application would call `lpm_set_max_pm(2)`.

The LPM module can be configured with a hard maximum permitted power mode.

    #define LPM_CONF_MAX_PM        N

Where N corresponds to the PM number. Supported values are 0, 1, 2. PM3 is not supported. Thus, if the value of the define is 1, the SoC will only ever enter PMs 0 or 1 but never 2 and so on.

The configuration directive `LPM_CONF_MAX_PM` sets a hard upper boundary. For instance, if `LPM_CONF_MAX_PM` is defined as 1, calls to `lpm_set_max_pm()` can only enable/disable PM1. In this scenario, PM2 can not be enabled at runtime.

When setting `LPM_CONF_MAX_PM` to 0 or 1, the entire SRAM will be available. Crucially, when value 2 is used the linker will automatically stop using the SoC's SRAM non-retention area, resulting in a total available RAM of 16 kbytes instead of 32 kbytes.

## Build headless nodes

It is possible to turn off all character I/O for nodes not connected to a PC. Doing this will entirely disable the UART as well as the USB controller, preserving energy in the long term. The define used to achieve this is (1: Quiet, 0: Normal output):

    #define CC2538_CONF_QUIET      0

Setting this define to 1 will automatically set the following to 0:

* `USB_SERIAL_CONF_ENABLE`
* `UART_CONF_ENABLE`
* `STARTUP_CONF_VERBOSE`

## Code Size Optimisations

The build system currently uses optimization level `-Os`, which is controlled indirectly through the value of the `SMALL` make variable. This value can be overridden by example makefiles, or it can be changed directly in `platform/openmote-cc2538/Makefile.openmote-cc2538`.

Historically, the `-Os` flag has caused problems with some toolchains. If you are using one of the toolchains documented in this README, you should be able to use it without issues. If for whatever reason you do come across problems, try setting `SMALL=0` or replacing `-Os` with `-O2` in `cpu/cc2538/Makefile.cc2538`.

## OpenMote-B transceivers

The OpenMote-B board includes two transceivers:
* TI's CC2538, which supports IEEE 802.15.4-2006 and operates in 2.4 GHz ISM band.
* Atmel's AT86RF215, which supports the IEEE 802.15.4-2012 and operates in both the 868/915 MHz and 2.4 GHz ISM bands.

By default, the OpenMote-B uses the CC2538 radio. To use the experimental driver for the AT86RF215 radio, you can define:

    #define OPENMOTEB_CONF_USE_ATMEL_RADIO   1

### Antenna RF switch

The OpenMote-B board has two SMA connectors for external antennas. One connector, labeled "sub-GHz," is always connected to the 868/915 MHz interface of the AT86RF215 radio. The other SMA connector, labeled "2.4-GHz," is connected to an RF switch that can link it to either the CC2538 radio or the AT86RF215 radio. The proper configuration is automatically managed by the functions located in `arch/platform/openmote-b/antenna.c`.

### AT86RF215 driver

The AT86RF215 radio features two RF cores (2.4 GHz and sub-GHz) and supports the following PHY modes: MR-FSK, MR-O-QPSK, and MR-OFDM. The current driver implementation (Oct. 2023) is designed to support only one core at a time (only the 2.4 GHz core has been tested). However, a different driver implementation could enable simultaneous operation of both RF cores. Additionally, the driver was only tested with the legacy O-QPSK modulation. The driver primarily supports the TSCH MAC mode, but also works with CSMA.

For more information, refer to the driver files located in `arc/dev/radio/at86rf215`.

## Maintainers

The original OpenMote-CC2538 was developed by OpenMote Technologies. Main contributor: Pere Tuset <peretuset@openmote.com>

Support for OpenMote-B was developed by Anders Wallberg (@wallb) and updated by @kkrentz and @9morano.

[doc:build-system]: /doc/getting-started/The-Contiki-NG-build-system
