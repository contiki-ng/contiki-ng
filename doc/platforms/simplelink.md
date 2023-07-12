# simplelink: TI SimpleLink MCU Platform

The objective of this wiki is to provide extensive self-help resources
including technical collateral and information for developing and running
Contiki-NG on TI SimpleLink&trade; devices.

The [TI SimpleLink MCU Platform][ti-simplelink-overview] is TI's
microcontroller platform for wired and wireless devices, which provides a
single software development environment for all SimpleLink devices. Currently,
only the CC13xx and CC26xx device family is supported.

**Please note:**

  * **Release v4.3 only supports Revision C for all CC13x2 and CC26x2 devices. For Revision E support you need to use branch `develop` as of 31th May 2019. Please refer to [the following E2E forum post](https://e2e.ti.com/support/wireless-connectivity/bluetooth/f/538/t/767769) to find out which revision your device is.**
  * **TSCH support is present on this platform since Contiki-NG release 4.6 (November 2020).** It has been successfully tested on most boards supported by this platform, including CC2650, CC2652R1, CC1310, and CC1312R1 LaunchPads. However, note that not all board and configurations have been tested and some problems have been reported, for example, TSCH on CC1352P1 currently has problems in the 2.4 GHz mode, and in the sub-GHz mode it fails to interoperate with other devices, e.g. CC1312R1.

For the **CC13xx and CC26xx device family**, the following development boards
are supported:

* **LaunchPads**, relevant files and drivers are under `launchpad/`
    * [CC1310 LaunchPad][cc1310-lp]
    * [CC1312R1 LaunchPad][cc1312r1-lp]
    * [CC1350 LaunchPad][cc1350-lp]
    * [CC1350 LaunchPad for 433 MHz][cc1350-4-lp]
    * [CC1352R1 LaunchPad][cc1352r1-lp]
    * [CC1352P LaunchPads][cc1352p-lp]
    * [CC2650 LaunchPad][cc2650-lp]
    * [CC26x2R1 LaunchPad][cc26x2r1-lp]
* **SensorTags**, relevant files and drivers are under `sensortag/`
    * [CC1350 SensorTag][cc1350-stk]
    * [CC2650 SensorTag][cc2650-stk]
* **[SmartRF06 Evaluation Board][srf06-eb]**, relevant files and drivers are under `srf06/`
    * CC13x0 EVM
    * CC26x0 EVM

The platform code can be found under
`$(CONTIKI)/arch/platform/simplelink/cc13xx-cc26xx`, and the CPU code can be
found under `$(CONTIKI)/arch/cpu/simplelink-cc13xx-cc26xx`. This port is only
meant to work with 7x7 mm chips. Contributions to add support for other chip
types is always welcome.

This guide assumes that you have basic understanding of how to use the command
line and perform basic admin tasks on UNIX family OSs.


## Port Features

The platform has the following key features:

* TI SimpleLink MCU Platform integration with Contiki-NG.
    * NoRTOS integration
    * TI drivers
    * SimpleLink middleware plugins
* Support for both CC13x0/CC26x0 and CC13x2/CC26x2 devices.
* Deep Sleep support with RAM retention for ultra-low energy consumption.
* Support for CC13xx prop mode: IEEE 802.15.4g-compliant Sub-1 GHz operation.
* Support for CC26xx ieee mode: IEEE 802.15.4-compliant 2.4 GHz operation.
* Concurent BLE beacons.

In terms of hardware support, the following drivers have been implemented:

* LaunchPad
    * LEDs
    * Buttons
    * External SPI flash
* SensorTag
    * LEDs
    * Buttons
    * Board sensors
        * Buzzer
        * Reed relay
        * MPU9250 - Motion processing unit
        * BMP280 - Digital pressure sensor
        * TMP007 - IR thermophile sensor
        * HDC1000 - Humidity and temperature sensor
        * OPT3001 - Light sensor
    * External SPI flash
* SmartRF06 EB peripherals
    * LEDs
    * Buttons
    * UART connectivity over the XDS100v3 backchannel


## Requirements

To use the port you need:

* TI's CC13xx/CC26xx Core SDK. The correct version will be installed
  automatically as a submodule when you clone Contiki-NG.

* Contiki can automatically upload firmware to the nodes over serial with the
  included [cc2538-bsl script][cc2538-bsl]. Note that uploading over serial
  does not work for the Sensortag, you can use TI's [Uniflash][uniflash] in
  this case.

* A toolchain to build firmware:

    * If you use the docker image or the vagrant image, this will be pre-installed for you. Otherwise, depending on your system, please follow the respective installation instructions ([native Linux](/doc/getting-started/Toolchain-installation-on-Linux) / [native mac OS](/doc/getting-started/Toolchain-installation-on-macOS)).

    * TI's [Code Composer Studio][ccs] can be used to build and debug
      Contiki-NG applications for the SimpleLink platform, which is especially
      useful when developing on Windows. Refer to [Set up Contiki-NG in Code
      Composer Studio](#set-up-contiki-ng-in-code-composer-studio) for more
      details.

* The *SRecord* package for manipulating EPROM load files.
    * **For Windows**, see the [srecord homepage][srecord].
    * **For Linux**, install via apt-get,

            $ sudo apt-get install srecord

    * **For OS X**, install via Homebrew,

            $ brew install srecord

For additional help on how to set your system up, you may also find the guides
below useful:

* [Native toolchain installation (Linux)](/doc/getting-started/Toolchain-installation-on-Linux)
* [Native toolchain installation (macOS)](/doc/getting-started/Toolchain-installation-on-macOS)


## Examples

To build an example with the SimpleLink platform, you need to set
`TARGET=simplelink`. In addition, you need to set the `BOARD` variable to the
board name corresponding to the device you are using. For example, using the
CC1310 LaunchPad would yield the following command `make TARGET=simplelink
BOARD=launchpad/cc1310`.

You can view all available boards by running `make TARGET=simplelink boards`.
The `BOARD` variable defaults to `srf06/cc26x0` (which is the SmartRF06 EB +
CC26X0EM) if not specified. Currently, the following boards are supported:

| Board                                       | `BOARD`             |
|:------------------------------------------- |:-------------------- |
| [CC1310 LaunchPad][cc1310-lp]               | `launchpad/cc1310`   |
| [CC1312R1 LaunchPad][cc1312r1-lp]           | `launchpad/cc1312r1` |
| [CC1350 LaunchPad][cc1350-lp]               | `launchpad/cc1350`   |
| [CC1350 LaunchPad for 433 MHz][cc1350-4-lp] | `launchpad/cc1350-4` |
| [CC1352R1 LaunchPad][cc1352r1-lp]           | `launchpad/cc1352r1` |
| [CC1352P LaunchPad][cc1352p-lp]             | `launchpad/cc1352p`  |
| [CC2650 LaunchPad][cc2650-lp]               | `launchpad/cc2650`   |
| [CC26x2R1 LaunchPad][cc26x2r1-lp]           | `launchpad/cc26x2r1` |
| [CC1350 SensorTag][cc1350-stk]              | `sensortag/cc1350`   |
| [CC2650 SensorTag][cc2650-stk]              | `sensortag/cc2650`   |
| [Srf06EB+CC13x0EM][srf06-eb]                | `srf06/cc13x0`       |
| [Srf06EB+CC26x0EM][srf06-eb]                | `srf06/cc26x0`       |

If you want to switch between building for one platform to the other, make
certain to run `make clean` before building for the new one, or you will get
linker errors.

To generate an assembly listing of the compiled firmware, supposing `BUILD_DIR_BOARD = <build-dir-board>` and `CONTIKI_PROJECT = <contiki-project>` in the respective Makefiles, run `make TARGET=simplelink BOARD=<board> <build-dir-board>/<contiki-project>.lst`. For example, considering the CC2650 SensorTag and the `hello-world` project, you can run `make TARGET=simplelink BOARD=sensortag/cc2650 build/simplelink/sensortag/cc2650/hello-world.lst`. This may be useful for debugging or optimizing your
application code. To intersperse the C source code within the assembly
listing, you must instruct the compiler to include debugging information by
adding `DEBUG = 1` to the project Makefile and rebuild by running `make
clean && make`.

There are currently no platform-specific examples for the SimpleLink platform.
However, any of the platform-agnostic examples can be used. More details about
examples can be found in their respective READMEs.


## How to Program your Device

In general terms, there are two possible ways to program your device:

* Over JTAG. This is always possible.
* Using the serial ROM bootloader. Some conditions need to be met before this
  is possible.


### Over JTAG

The build process will output firmware in multiple formats: a `*.bin` file, a
`*.elf` file and an Intel HEX file (`*.hex`). The correct file to upload to
your device depends on the tool you use to do the programming. More
information in the corresponding subsection.

This is always possible and you have two options in terms of what software to
use:

* TI's [SmartRF Flash Programmer 2][srf-fp]. Windows only. Supports `*.bin`,
  `*.elf` and `*.hex`.
* TI's [Uniflash][uniflash]. Linux, OS X and Windows. Supports `*.bin`,
  `*.elf` and `*.hex`.


### Using the ROM bootloader

Under some circumstances, the device can also be programmed through its ROM
bootloader, using the [`cc2538-bsl`][cc2538-bsl] script under tools. This is
commonly done using `make <contiki-project>.upload` (e.g. `make
hello-world.upload`), which automatically invokes `cc2538-bsl` with the
correct arguments.

This is currently only supported for the `x0` devices of the family (cc26x0, cc13x0), but not for the newer `x2` devices (e.g. cc1312r1 or cc1352p1).

#### Device Enumeration

LaunchPads and Sensortags use an XDS110 debugger, while the SmartRF06 EB uses
an XDS100v3 debugger. If you are using a SmartRF06 EB, make sure the "Enable
UART" jumper is set.

**On Windows**, you can view connected devices with Device Manager. Open up Device
Manager and expand **Ports (COM & LPT)**. You should
observe various XDS debugger connections, depending on which boards are
connected.

For an XDS110 debugger, two COM ports are available: an Application/User UART
port and an Auxiliary Data port. For any serial connections, use the
Application/User UART port.

For an XDS100v3 debugger, only a single USB Serial port is available. Use this
port for any serial connections.

**On Linux**, XDS110 debuggers will show up under `/dev` as `/dev/ttyACM*`,
while XDS100v3 will show up under `/dev` as `/dev/ttyUSB*`.

However, if the XDS100v3 debugger does not show up, you can manullay configure
the device. First, find the BUS ID of the XDS100v3 debugger with `lsusb`.

    $ lsusb
    ...
    Bus 001 Device 002: ID 0403:a6d1 Future Technology Devices International, Ltd

The ID in this case is `0403:a6d1`. Register this as a new FTDI ID with the
`ftdi_sio` driver.

From kernel version 3.12 and newer:

    # modprobe ftdi_sio
    # echo 0403 a6d1 > /sys/bus/usb-serial/drivers/ftdi_sio/new_id

Before kernel version 3.12:

    # modprobe ftdi_sio vendor=0x0403 product=0xa6d1

**On OS X**, the device will show up as `/dev/tty.usbmodem<sequence of letters
and numbers>` (e.g. `tty.usbmodemL1000191`)


#### Conditions to use the ROM bootloader

On Linux and OS X, you can program your device via the chip's serial ROM
bootloader. In order for this to work, the following conditions need to be
met:

* The board supports the bootloader. This is the case for SmartRF06EB with
  CC13x0/CC26x0 EMs and it is also the case for LaunchPads. Note that
  uploading over serial does not (and will not) work for the Sensortag.
* The ROM bootloader is unlocked.

> You will not be able to use the ROM bootloader with a new out-of-the-box
> device programmed with factory firmware.

For newly-bought hardware, you need to use the JTAG to first erase the device
using either [SmartRF Flash Programmer 2][srf-fp] or [UniFlash][uniFlash] (see
the relevant subsection).

After reset, the device will either execute the firmware on its flash or enter bootloader mode so it can be programmed. This is dictated by the
following logic:

* If the flash is empty, the device will enter bootloader mode and can be
  programmed using the ROM bootloader.
* If the flash contains a firmware image:
    * If the firmware is configured to lock the bootloader (which is the case
      e.g. for factory images), the device will execute the firmware and will
      not enter ROM bootloader mode.
    * If the firmware is configured to unlock the bootloader, and if a
      specific (configurable) DIO pin is high/low (also configurable), the
      device will enter bootloader mode and can be programmed using the ROM
      bootloader.

To enable/unlock the bootloader backdoor in your image, define
`CCFG_CONF_ROM_BOOTLOADER_ENABLE` as 1 in your application's `project-conf.h`.
The correct DIO and high/low state required to enter bootloader mode will be
automatically configured for you, depending on your device.

With the above in mind, force your device into bootloader mode by keeping the
correct user button pressed, and then pressing and releasing the reset button.
On the SmartRF06EB, you enter the bootloader by resetting the EM (EM RESET
button) while holding the `select` button. For the LaunchPad, you enter the
bootloader by resetting the chip while holding `BTN_1`. It is possible to
change the pin that will trigger bootloader mode by changing
`CCFG_CONF_BL_PIN_NUMBER` in your application's `project-conf.h`.

Remember that the device will always enter bootloader mode if you erase its
flash contents.

If your device has correctly entered bootloader mode, you can now program it.

The serial uploader script will automatically pick the first available serial
port. If this is not the port where your node is connected to, you can force the
script to use a specific port by defining the `PORT` argument, e.g.:

    $ make PORT=/dev/ttyACM0 hello-world.upload

If you get the error below, the most probable cause is that you have specified
the wrong `PORT`, or the device has not entered bootloader mode:

    Connecting to target...
    ERROR: Timeout waiting for ACK/NACK after 'Synch (0x55 0x55)'
    make: *** [hello-world.upload] Error 1

Some common causes why the device has not entered bootloader mode:

* The device's flash contains an image that was built with
  `CCFG_CONF_ROM_BOOTLOADER_ENABLE` defined as 0. In this case, you will need
  to use SmartRF Flash Programmer 2 or UniFlash to erase flash.
* You programmed the device with firmware meant for a different device (e.g.
  you programmed a LaunchPad with an image built for a Sensortag). In this
  case, you will also need to use SmartRF Flash Programmer 2 or UniFlash to
  erase flash.
* You reset the device without keeping the correct button pressed. Simply try
  again.

For more information on the serial bootloader, see its README under the
`tools/cc2538-bsl` directory.


## Building Deployment / Production Images

For deployment/production images, it is *strongly* recommended to:

* Disable the ROM bootloader by defining `CCFG_CONF_ROM_BOOTLOADER_ENABLE` as
  0. In doing so, it is impossible to enter bootloader mode unless one first
  erases the device's flash.
* Disable the JTAG interface, by defining `CCFG_CONF_JTAG_INTERFACE_DISABLE`
  as 0. In doing so, the only JTAG operation available will be a device forced
  mass erase (using SmartRF Flash Programmer 2 or UniFlash).

Both macros have default values set in `cc13xx-cc26xx-conf.h`, found under
`arch/cpu/simplelink-cc13xx-cc26xx/`. You can override the defaults in your
application's `project-conf.h`.

If you do not follow these guidelines, an individual with physical access to
your device may be able to read out its flash contents. This could give them
access to your IP and it could also lead to a compromise of e.g. keys used for
encryption.


## Border Router over UART

The platform code can be used as a border router (SLIP over UART) via the `rpl-border-router` example. This example is expected to work
off-the-shelf, without any modifications required.


## slip-radio with 6lbr

The platform can also operate as a slip-radio over UART, to be used with
[6lbr][6lbr].

Similar to the border router, the example is expected to work off-the-shelf,
without any modifications required.


## 2.4 GHz vs Sub-1 GHz operation

The platform supports both modes of operation, provided the chip also has the
relevant capability. 2.4 GHz mode is sometimes called IEEE mode while Sub-1
GHz mode is sometimes called Prop mode, based on the respective RF commands
used in the corresponding implementation.

If you specify nothing, the platform will default to Sub-1 GHz mode for CC13xx
devices and 2.4 GHz mode for CC26xx devices. To force either mode, you need to
set `RF_CONF_MODE` to the relevant `RF_MODE_*` in your application's
`project-conf.h`.

    // For 2.4 GHz (IEEE) Mode
    #define RF_CONF_MODE    RF_MODE_2_4_GHZ
    // For Sub-1 GHz (Prop) Mode
    #define RF_CONF_MODE    RF_MODE_SUB_1_GHZ


## Low-Power Operation

The platform takes advantage of the Power driver, which is part of TI Drivers.
In a nutshell, other TI Drivers will acquire and release certain power
constraints, and the Power driver will seamlessly turn on/off power domains
depending on what power constraints are set. When there are no events in the
Contiki-NG event queue, the Power driver will put the CPU into the lowest
possible power state.

Because this platform's low-power operation is handled inside TI-provided drivers,
the Contiki-NG energest module has no immediate way of determining the CPU's
power state with accuracy. More specifically, it is impossible to distinguish
between `ENERGEST_TYPE_LPM` and `ENERGEST_TYPE_DEEP_LPM`. When using the energest
module for this platform, the value of `ENERGEST_TYPE_DEEP_LPM` will always be
zero; this is expected behaviour. All time spent by the CPU in any low-power mode
will be captured under `ENERGEST_TYPE_LPM`.

## SimpleLink Software Environment

The SimpleLink software environment is a collection of drivers (TI Drivers)
and plugins, which are common across different SimpleLink device families. At
the core of the SimpleLink platform, there is a Real-Time Operating System (RTOS),
which provides services such as timing and scheduling tasks.

For Contiki-NG, the NoRTOS kernel is used. NoRTOS is a single-threaded RTOS
which only implements bare-minimum RTOS primitives, such as timers and
semaphores. NoRTOS is required for using TI Drivers. Interrupts will still
preempt the main thread, but typical "multithreading" is not supported.

TI Drivers is a collection of drivers for a number of peripherals. Most
drivers are common across several device families, while some are available
only for certain families.

Plugins, or middleware, are components which add functionality on top of
drivers, such as communication stacks and graphics libraries.

The SimpleLink software environment is packaged together in a Software
Development Kit (SDK). SimpleLink SDKs can be downloaded for each SimpleLink
device family, and are updated by TI in a quarterly manner. However, Contiki-NG
provides a Core SDK, which is a SimpleLink SDK common for all CC13xx and
CC26xx devices. The Core SDK is provided as a `git` submodule.


### Override Core SDK

By default, the Core SDK will be used by the build system when building.
However, you can override the Core SDK with a locally installed SimpleLink SDK
by overriding the `CORE_SDK` environment variable. The variable must point to
the full path of the installed SimpleLink SDK, as in the following example:

```bash
$ make CORE_SDK=/opt/ti/simplelink_cc26x2_sdk_2_30_00_34 TARGET=simplelink BOARD=launchpad/cc26x2r1
```

This allows you to update the Core SDK version to a newer version than what is
provided by Contiki-NG.

However, there are some limitations you need to be aware of when overriding
the Core SDK. There is no good way to determine the minimal SimpleLink SDK
version which is compatible with the Contiki-NG version you are working with.
This comes from the fact that the SimpleLink SDKs are updated at a faster rate
by TI than what Contiki-NG updates the Core SDK submodule.

In addition, the SimpleLink SDK must also correspond to the board you are
compiling for. For example, if you are overriding the Core SDK with a
SimpleLink CC26x2 SDK, then you must use a CC26x2 device, such as the CC26x2R1
LaunchPad.


### Configure TI Drivers

Some of the TI drivers are partially integrated with the Contiki-NG
environment, and therefore has to be initialized by the Contiki-NG run-time if
used. In order to make this configurable, certain drivers can be enabled or
disabled by setting a configuration define in your application's
`project-conf.h`. Enabling or disabling a driver means in this context whether
the Contiki-NG run-time should initialize the driver at run-time.

Below is a summary of which TI drivers can be configured with which
configuration defines, and what the default value of the configuration defines
are.


| TI Driver | Configuration Define  | Default Value |
| --------- | --------------------- |:-------------:|
| UART      | `TI_UART_CONF_ENABLE` | 1             |
| SPI       | `TI_SPI_CONF_ENABLE`  | 1             |
| I2C       | `TI_I2C_CONF_ENABLE`  | 1             |
| NVS       | `TI_NVS_CONF_ENABLE`  | 0             |
| SD        | `TI_SD_CONF_ENABLE`   | 0             |


Disabling unused drivers are beneficial, as it reduces code and memory
footprint. However, some features may depend on certain drivers, and disabling
a driver which some other drivers depend on will result in a compile error.
For example, the Sensortag sensor drivers use the I2C driver, and therefore
disabling the I2C driver requires Board sensors to be disabled as well.

Some of the drivers have multiple peripherals, which can be independently
enabled or disabled. This is done purely for reducing the memory footprint, as
the compiler is not able to optimize and remove unused peripherals when a
driver is enabled.

In other words, enabling the SPI driver with both peripherals but only using
the SPI0 peripheral, you still have to have the SPI1 peripheral configuration
objects in memory.


| Peripheral   | Configuration Define              | Default Value         | CC13x0/CC26x0 | CC13x2/CC26x2 |
| ------------ | --------------------------------- |:---------------------:|:-------------:|:-------------:|
| UART0        | `TI_UART_CONF_UART0_ENABLE`       | `TI_UART_CONF_ENABLE` | Yes           | Yes           |
| UART1        | `TI_UART_CONF_UART1_ENABLE`       | 0                     | Yes           | Yes           |
| SPI0         | `TI_SPI_CONF_SPI0_ENABLE`         | `TI_SPI_CONF_ENABLE`  | Yes           | Yes           |
| SPI1         | `TI_SPI_CONF_SPI1_ENABLE`         | 0                     | No            | Yes           |
| I2C0         | `TI_I2C_CONF_I2C0_ENABLE`         | `TI_I2C_CONF_ENABLE`  | Yes           | Yes           |
| NVS Internal | `TI_NVS_CONF_NVS_INTERNAL_ENABLE` | `TI_NVS_CONF_ENABLE`  | Yes           | Yes           |
| NVS External | `TI_NVS_CONF_NVS_EXTERNAL_ENABLE` | `TI_NVS_CONF_ENABLE`  | Yes           | Yes           |


Any other TI Driver, except for the RF driver, can be used as normal.


### SimpleLink Support

Any issues regarding TI software from the SimpleLink SDK, or any issues
regarding the software implementation of this platform, please post to the
[E2E forum][e2e].

For any issues regarding Contiki-NG in general that is not directly relevant
to the software implementation of this platform, please post an issue to the
Contiki-NG repository.


## Set up Contiki-NG in Code Composer Studio

Before anything else, make sure you have cloned out the Contiki-NG repository
and at least checked out the `coresdk_cc13xx_cc26xx` and `CMSIS` submodules.

```bash
$ git clone https://github.com/contiki-ng/contiki-ng.git
$ cd contiki-ng
$ git submodule update --init arch/cpu/simplelink-cc13xx-cc26xx/lib/coresdk_cc13xx_cc26xx arch/cpu/arm/CMSIS
```

Download the necessary software:

* TI's [Code Composer Studio][ccs] (CCS) with support for CC13xx/CC26xx
  devices installed.
* The ARM GCC add-on in CCS. In CCS:
    * View &rarr; CCS App Center.
    * Search for `ARM GCC` and click *Select*.
* If you are using **Windows**:
    * Install [Git for Windows][git4win].
* If you are using **Linux**:
    * `sudo apt-get install build-essential`

Note that when debugging CC13x0 and CC26x0 devices with CCS, the Watchdog
module should be disabled (unless you are debugging Watchdog usage). The
Watchdog on CC13x0 and CC26x0 devices is not properly paused when halted by
the debugger, and therefore causes an unexpected system reset which breaks the
debugging session. The Watchdog module is disabled by setting
`WATCHDOG_CONF_DISABLE` to 1 in your application's `project-conf.h` file.

In CCS, do the following steps:

1. Create an empty CCS project. In CCS:
    * File &rarr; New &rarr; CCS Project.
    * Set **Target** to *SimpleLink Wireless MCU* and **Target device** to
      your device.
    * Name the project to your liking.
    * Make sure the **Compiler version** is set to *GNU compiler*.
    * Select the *Empty Project* template.
    * Click *Finish*.

2. Add a path variable for Contiki-NG. This is only for convenience, as this
   allows us to refer to the Contiki-NG source folder later. In CCS:
    * In Project Explorer, right click the project &rarr; Properties.
    * Resource &rarr; Linked Resources.
    * Click *New*.
    * Set **Name** to `CONTIKI_ROOT`.
    * Set **Location** to the path of the Contiki-NG repository. You can click
      *Folder* and manually select the relevant folder.

3. Add Contiki-NG source folders to the project. This allows you to browse
   Contiki-NG source files in CCS without copying them into the project
   folder. In CCS:
    * In Project Explorer, right click the project &rarr; New &rarr; Folder.
    * Set **Folder name** to `contiki-ng`.
    * Click *Advanced*.
    * Select *Link to alternate location (Linked Folder)*.
    * In the textbox, write `${CONTIKI_ROOT}`.
    * Click *Finish*.
    * Click **No** if CCS asks to include *.cfg* files, as CCS is
      interpreting them as XDCTools configuration files.

4. **Windows only**: Add Git for Windows and SRecord to the PATH environment variable 
   in the CCS project. The Contiki-NG build system needs other shell tools
   such as `git` and `make` from Git for Windows and the `srec_cat` tool from the        SRecord package. In CCS:
    * In Project Explorer, right click the project &rarr; Properties.
    * CCS Build &rarr; Environment.
    * If the `PATH` variable does not exist, click *Add*, set **Name** to
      `PATH` and **Value** to `${Path}`.
    * Select the `PATH` variable and click *Edit*.
    * Prepend the absolute path of both the `bin` and `usr\bin` folders from
      the Git for Windows installation and the absolute path of the SRecord                 folder from the SRecord package extraction to the `PATH` variable.
        * Separate paths with `;`.
        * For example, if the installation path for Git for Windows is
          `C:\Git` and the extraction path for SRecord is `C:\SRecord`, then prepend           `C:\Git\bin;C:\Git\usr\bin;C:\SRecord;` to the `PATH` variable.
    * Click *OK*.

5. Adjust build settings to correctly invoke the Contiki-NG Makefile. In CCS:
    * In Project Explorer, right click the project &rarr; Properties.
    * Make sure *Show advanced settings* (in the bottom left corner) is clicked.
    * CCS Build &rarr; Builder.
    * Unselect *Generate Makefiles automatically*.
    * Under *Build location*, set **Build directory** to the project directory
      of your application.
        * For building a Contiki-NG example, use the source directory of the
          example (e.g. `${CONTIKI_ROOT}/examples/hello-world`).
        * For building your own applications, put the application sources and
          the Makefile into your project directory, not in the Contiki-NG
          directory.
    * Under *Make build targets*;
        * Unselect **Build on resource save (Auto build)**.
        * Select **Build (Incremental build)** and set it to `<contiki-project>`               (e.g. `hello-world`) assigned in the project Makefile.
        * Select **Clean** and set it to `clean`.

6. Add the `TARGET` and `BOARD` variables, and add debug symbols:
    * In the project Makefile, add the following lines:
        * Set `TARGET` to `simplelink`, i.e.:

            ```
            TARGET = simplelink
            ```
        * Set `BOARD` to the device you want to compile for, e.g.:

            ```
            BOARD = launchpad/cc1310
            ```
        * Set `DEBUG` to 1 to enable debug symbols.

            ```
            DEBUG = 1
            ```

7. Build the project. In CCS:
    * In Project Explorer, right click the project &rarr; Build Project.
    * If something goes wrong, it is usually due to tools not being found.
      Check the `PATH` environment variable in that case.

8. Create and configure a debug session. In CCS:
    * Start by creating a default debug session.
        * In Project Explorer, right click the project &rarr; Debug As &rarr; Code
          Composer Debug Session.
        * This is expected to fail, because the path of the executable file guessed             by CCS is wrong.
    * Set the correct path.
        * In Project Explorer, right click the project &rarr; Properties.
        * Select *Run/Debug Settings*.
        * Select the newly created launch session with the same name as the
          project and click *Edit*.
        * Under *Program*, set **Program** to the absolute path of the generated               `*.elf` file. It is recommended to click *File System* and select the
          `*.elf` file manually to make sure the path is correct.
    * Start debugging.
        * In Project Explorer, right click the project &rarr; Debug As &rarr; Code
          Composer Debug Session.
        * The debug session should start as intended, and you should be able to
          step through the source code.

9. In order to view the serial output of the device connected to `PORT`, via Terminal    in CCS:
    * View &rarr; Terminal.
    * Open a Terminal by clicking the designated icon.
    * Set **Choose terminal** to `Serial Terminal` and **Serial port** to the right         `PORT`.
    * Click *OK*.
    * **Note:** You will most likely see that the new line character (`\n`) in the         output is interpreted as `LF` instead of `CRLF`. This misinterpretation is a         known problem for which no solution has been provided so far. An indirect             solution for this problem is to insert a carriage return character (`\r`)             before a new line one. One way to do so is to modify `contiki-ng/arch/cpu/simplelink-cc13xx-cc26xx/dev/dbg-arch.c` as follows:

      ```
      75    75           return 0;
      76    76         }
      77    77
            78    +    /* Find the number of new line characters in the sequence. */
            79    +    size_t n = 0;
            80    +    for(size_t i = 0; i < max_len; i++)
            81    +      if(*(seq + i) == '\n')
            82    +        ++n;
            83    +
            84    +    if(n > 0) {
            85    +      /*
            86    +       * Create the second sequence by inserting a carriage return character
            87    +       * before any new line one in the first sequence.
            88    +       */
            89    +      unsigned char ch, seq2[max_len + n];
            90    +      size_t j = 0;
            91    +      for(size_t i = 0; i < max_len; i++) {
            92    +        ch = *(seq + i);
            93    +        if(ch != '\n')
            94    +          seq2[j] = ch;
            95    +        else {
            96    +          seq2[j] = '\r';
            97    +          seq2[++j] = '\n';
            98    +        }
            99    +        ++j;
           100    +      }
           101    +      num_bytes = (int)uart0_write((const unsigned char *)seq2, max_len + n);
           102    +    }
           103    +    else
      78          -    num_bytes = (int)uart0_write(seq, max_len);
           104    +      num_bytes = (int)uart0_write(seq, max_len);
           105    +    
      79   106         return (num_bytes > 0)
      80   107                ? num_bytes
      81   108                : 0;
      ```

      Note that the above modification results in increased stack memory usage which, in turn, increases the probability of stack memory overflow. There might be alternative solutions, especially direct ones that might be provided by the Eclipse/CCS community in the future.


[ti-simplelink-overview]: https://www.ti.com/wireless-connectivity/simplelink-solutions/overview/overview.html
[cc1310-lp]:              https://www.ti.com/tool/LAUNCHXL-CC1310
[cc1312r1-lp]:            https://www.ti.com/tool/LAUNCHXL-CC1312R1
[cc1350-lp]:              https://www.ti.com/tool/LAUNCHXL-CC1350
[cc1350-4-lp]:            https://www.ti.com/tool/LAUNCHXL-CC1350-4
[cc1352r1-lp]:            https://www.ti.com/tool/LAUNCHXL-CC1352R1
[cc1352p-lp]:             https://www.ti.com/tool/LAUNCHXL-CC1352P
[cc2650-lp]:              https://www.ti.com/tool/LAUNCHXL-CC2650
[cc26x2r1-lp]:            https://www.ti.com/tool/LAUNCHXL-CC26X2R1
[cc1350-stk]:             https://www.ti.com/tool/CC1350STK
[cc2650-stk]:             https://www.ti.com/tool/CC2650STK
[srf06-eb]:               https://www.ti.com/tool/SMARTRF06EBK

[ccs]:        https://www.ti.com/tool/CCSTUDIO
[uniflash]:   https://www.ti.com/tool/UNIFLASH
[srf-fp]:     https://www.ti.com/tool/FLASH-PROGRAMMER
[e2e]:        https://e2e.ti.com/support/wireless-connectivity

[6lbr]:       https://cetic.github.io/6lbr
[gcc-arm]:    https://launchpad.net/gcc-arm-embedded
[cc2538-bsl]: https://github.com/JelmerT/cc2538-bsl
[srecord]:    http://srecord.sourceforge.net/
[git4win]:    https://gitforwindows.org/
