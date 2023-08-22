# Porting Contiki‐NG to new platforms

This page provides a basic guide on how to port Contiki-NG to a new hardware device. The guide assumes that your device is an IoT-type device with the following characteristics:

* A microcontroller (e.g. Arm Cortex, msp430- or avr-based), plus on-chip peripherals (such as UART, SPI, I2C, DMA...).
* A radio (either integrated on the same chip as your MCU, or on a separate chip). This can be e.g. a standard IEEE 802.15.4 radio operating at the 2.4GHz band, a sub-ghz radio, or a BLE radio.
* Various off-chip peripherals, e.g. LEDs, buttons, sensors.

This guide assumes that you understand the basics of the C programming language and of GNU make. It also assumes that you have already familiarised yourself with the basics of the Contiki-NG build system ([doc:build-system]), especially the concepts of `TARGET`, `BOARD` and `MODULES`.

## Port Contiki-NG to your board and MCU
In a nutshell, creating a Contiki-NG port involves the following steps:

* [Adding support for your CPU](#cpu-code). Simply speaking, this means adding support for everything that happens _inside_ your device's main chip. If your CPU is already supported, you can skip to the next step.
  * Developing drivers and arch-specific modules for your CPU.
  * Extending the Contiki-NG build system for your CPU.
* [Adding support for your platform](#platform-code). Simply speaking, this means adding support for everything that happens _outside_ your device's main chip (e.g. LEDs, buttons, sensors).
  * Developing drivers and arch-specific modules for your platform.
  * Extending the Contiki-NG build system for your platform.
* [Writing some examples](#create-some-examples).
* [Adding some compile tests](#add-ci-tests) for your port.
* [Creating API docs and guides](#add-documentation).

### CPU code
If your CPU is already supported by Contiki-NG, skip to the "[Platform code](#platform-code)" section of this guide.

If not, let's assume that your MCU is called `my-new-mcu`.

* Create a directory under `arch/cpu/` and name it `my-new-mcu`.
* Under `arch/cpu/my-new-mcu`, create the following files:
  * `Makefile.my-new-mcu`: This is where you will explain to the build system how to build firmware images suitable for your MCU.
  * `my-new-mcu-conf.h`: This is where you will put MCU-specific macros that users are expected to be able to modify (for example whether you want your internal watchdog timer to be enabled/disabled).
  * `my-new-mcu-def.h`: This is where you will put MCU-specific macros that users should _not_ modify. One such example is the `define` for `CLOCK_CONF_SIZE` and the number of software clock ticks per second `CLOCK_CONF_SECOND`
  * `doxygen-group.txt`: This is where you will define where your CPU code's documentation will be located in the API doc structure. See for example [`arch/cpu/cc2538/doxygen-group.txt`](https://github.com/contiki-ng/contiki-ng/tree/develop/arch/cpu/cc2538/doxygen-group.txt).

#### Configure the build system
In all cases, your `Makefile.my-new-mcu` will need to specify your CPU-dependent source files that you will implement later. These are specified by appending to the `CONTIKI_SOURCEFILES` make variable. For example, after you have developed your CPU code, you are likely to end up with something like this:

```Makefile
#### CPU-dependent source files
CONTIKI_CPU_SOURCEFILES += soc.c clock.c rtimer-arch.c uart.c watchdog.c

DEBUG_IO_SOURCEFILES += dbg-printf.c dbg-snprintf.c dbg-sprintf.c strformat.c

USB_SOURCEFILES += usb-core.c cdc-acm.c usb-arch.c usb-serial.c cdc-acm-descriptors.c

#### This is what will actually instruct the system to build all of the above
CONTIKI_SOURCEFILES += $(CONTIKI_CPU_SOURCEFILES) $(DEBUG_IO_SOURCEFILES)
CONTIKI_SOURCEFILES += $(USB_SOURCEFILES)

```

If your MCU is cortex-based, you are very likely going to be able to use the existing CM3/CM4 build infrastructure. This could be as simple as including our CM3- or CM4-specific `Makefile` at the end of your `Makefile.my-new-mcu`. For example, if your MCU is a CM3:
```Makefile
include $(CONTIKI)/arch/cpu/arm/cortex-m/cm3/Makefile.cm3
```
This will automatically do most of the things you need to do to build Contiki-NG for CM3- or CM4-based systems. It will configure build targets, the toolchain and some common `CFLAGS` and `LDFLAGS`. You can always append to those in your Makefile. 

If your MCU is _not_ cortex-based, you have additional work to do. You will at least need to:
* Specify your toolchain name and location by providing values for the `CC`, `LD` etc make variables.
* Specify `CFLAGS`, `LDFLAGS` etc in order to build for your MCU.
* Provide a compile rule. Here you have two options, either to use the rule in the top-level `Makefile.include` or create your own rule. To do the latter, you will have to set `CUSTOM_RULE_C_TO_OBJECTDIR_O = 1` in your Makefile.
* Provide a link rule. Here you have two options, either to use the link rule in the top-level `Makefile.include` or create your own link rule. To do the latter, you will have to set `CUSTOM_RULE_LINK = 1` in your Makefile.

Generally-speaking, a `Makefile.my-new-mcu` will be very different for different MCUs and its impossible to provide an exhaustive guide of what needs to go in it, this largely depends on your requirements. As a rule of thumb, try to re-use existing build infrastructure where possible and consult existing CPU Makefiles for ideas and common practices. Do get in touch with questions if in doubt!

#### Develop MCU drivers
Contiki-NG provides its own platform-independent `main()` function (implemented in [`os/contiki-main.c`](https://github.com/contiki-ng/contiki-ng/tree/develop/os/contiki-main.c)). Depending on your CPU architecture/toolchain etc, you will need to specify that this function needs to be called at the end of your firmware's entry point / CPU initialisation code.

You will then need to implement drivers to underpin the Contiki-NG timer infrastructure. As a bare minimum, you will need to provide:

* A source for the software clock. Commonly, you will create a `clock.c` file and implement the functions required by the clock API. You will also most likely need to implement an interrupt handler that will update the software clock counter each time your selected hardware clock source ticks. See [`os/sys/clock.h`](https://github.com/contiki-ng/contiki-ng/tree/develop/os/sys/clock.h) for the function prototypes.
* The arch-specific implementation of rtimers: This is likely to be in files named `rtimer-arch.[ch]`. See [`os/sys/rtimer.h`](https://github.com/contiki-ng/contiki-ng/tree/develop/os/sys/rtimer.h).

Once timing drivers are in place, you will most likely want to implement a driver such that your device can print debugging output for you. Again, this is very CPU-dependent, but the main objective here is to reach a stage where `printf` in the code results in some console output that you are able to see. In some cases this means you will need to implement a UART driver and provide an implementation of `putchar` that outputs bytes over the UART. If your toolchain provides implementations of `printf` and related functions you can choose to use them. Or you can have a look at our lightweight implementations under [`os/lib/dbg-io`](https://github.com/contiki-ng/contiki-ng/tree/develop/os/lib/dbg-io).

It is recommended to proceed with providing implementations of functions to manipulate the global interrupt flag. Have a look at the API in [`os/sys/int-master.h`](https://github.com/contiki-ng/contiki-ng/tree/develop/os/sys/int-master.h).

When you reach this stage you should have all the CPU code you need to run a simple `hello-world` [example][tutorial:hello-world] and a simple example that uses Contiki-NG's timers and events. You can continue with CPU drivers, or you can shift your attention to platform (off-chip) code ([next section](#platform-code)).

With interrupt manipulation in place, your MCU will support generic mutexes and critical sections. Generic mutexes rely on disabling/re-enabling interrupts. If your MCU features mutex-related instructions, you can choose to provide a mutex implementation that exploits those instructions and therefore does not rely on disabling the global interrupt. Such functions are already provided for Cortex-M MCUs. You may also choose to provide MCU-specific memory barriers (again, already supported for Cortex-M processors, as well as for msp430).

The following steps really depend on your priorities. On most cases and depending on your requirements, you will want to consider developing the following drivers:

* A driver for your MCU's internal watchdog timer, if it has one (API in [`os/dev/watchdog.h`](https://github.com/contiki-ng/contiki-ng/tree/develop/os/dev/watchdog.h)).
* An implementation of the MCU-specific parts of the GPIO HAL (see [`os/dev/gpio-hal`](https://github.com/contiki-ng/contiki-ng/tree/develop/os/dev/gpio-hal)) for the API.
* Serial line input. Commonly, by extending your UART driver so it can receive characters from a console. This will enable usage of the [Contiki-NG shell][tutorial:shell].
* A driver for your radio, if your MCU has on-chip radio capability. If it does, you will need to implement the arch-specific components of the API defined in [`os/dev/radio.h`](https://github.com/contiki-ng/contiki-ng/tree/develop/os/dev/radio.h). Special attention must be paid if you wish for your radio to support TSCH. Detailed information on adding TSCH support to your radio can be found in the [respective wiki page][doc:tsch].
* A driver for your MCU's internal RNG, if it has one (API in [`os/lib/random.h`](https://github.com/contiki-ng/contiki-ng/tree/develop/os/lib/random.h)).
* Drivers that will allow you to use external peripherals, for example SPI, I2C, ADC. A HAL is defined for SPI ([`os/dev/spi.h`](https://github.com/contiki-ng/contiki-ng/blob/develop/os/dev/spi.h)), but not for the rest (yet!).

#### Interrupt handlers
If you are developing CPU code, you will almost certainly be required to implement hardware interrupt handlers. While doing so, please pay special attention to the fact that some Contiki-NG system and library functions are _not_ safe to run within an interrupt context. More details in [doc:multitasking-and-scheduling]. 

### Platform code
If you are reading this, your CPU is either already supported by Contiki-NG, or you have already started adding support as per the instructions in the "[CPU code](#cpu-code)" section of this guide.

Let's assume that your platform is called `my-platform` and that it is powered by a new CPU called `my-new-mcu`, for which you are adding support at the same time.

* Under `arch/platform`, create a directory called `my-platform`.
* Under `arch/platform/my-platform`, create the following files:
  * `Makefile.my-platform`: This is where you will explain to the build system how to build firmware images suitable for your platform and its variants.
  * `platform.c`: This is where you will provide platform-specific functions needed by Contiki-NG's `main()` routine. Using `platform.c` as the filename is a convention, not a strictly technical requirement.
  * `contiki-conf.h`: This is where you will put platform-specific macros that users are expected to be able to modify.
  * `my-platform-def.h` (optionally): This is where you will put platform-specific macros that users should _not_ modify (for example, LED pin mappings).
  * `doxygen-group.txt`: This is where you will define where your platform code's documentation will be located in the API doc structure. See for example [`arch/platform/nrf/doxygen-group.txt`](https://github.com/contiki-ng/contiki-ng/tree/develop/arch/platform/nrf/doxygen-group.txt).

#### Prepare the configuration system
Open `contiki-conf.h` and:

* Paste the boilerplate that includes example-specific configuration files at the top:
```C
/* Include Project Specific conf */
#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif /* PROJECT_CONF_PATH */
```

* Include `my-platform-def.h`, `my-new-mcu-def.h` at the top, _before_ any user configuration
* Include `my-new-mcu-conf.h` at the end, _after_ all user configuration.

In the end, your `contiki-conf.h` should look something like this:

```C
#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

/* Possibly standard includes here, e.g. stdint.h */
/*---------------------------------------------------------------------------*/
/* Include Project Specific conf */
#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif /* PROJECT_CONF_PATH */
/*---------------------------------------------------------------------------*/
#include "my-platform-def.h"
#include "my-new-mcu-def.h"
/*---------------------------------------------------------------------------*/
/* 
 * All user configuration to go here
 */
/*---------------------------------------------------------------------------*/
/* Include CPU-related configuration */
#include "my-new-mcu-conf.h"
/*---------------------------------------------------------------------------*/
#endif /* CONTIKI_CONF_H */
```

#### Configure the build system
Edit your `Makefile.my-platform`. As a minimum, you will need to:

* Add to the `CLEAN` variable, so that your port's firmware images will get cleaned when you run `make clean`.
```Makefile
CLEAN += *.my-platform
```
* Append platform sourcefiles to the `CONTIKI_SOURCEFILES` make variable.
* Define the path to the CPU dir and include `Makefile.my-new-mcu`.
```Makefile
CONTIKI_CPU=$(CONTIKI)/arch/cpu/my-new-mcu
include $(CONTIKI_CPU)/Makefile.my-new-mcu
```
* Specify `MODULES` needed by your platform. For example, if you need the CC1200 radio driver, you will need to add:
```Makefile
MODULES += arch/dev/cc1200
```
* Specify additional, platform-specific make rules. For example, you may wish to specify rules for the `upload` and `login` targets. See existing platform Makefiles for more info on those.

In the end, your `Makefile.my-platform` will look something like that
```Makefile
CONTIKI_SOURCEFILES += platform.c <platform sourcefiles here>

CLEAN += *.my-platform

CONTIKI_CPU=$(CONTIKI)/arch/cpu/my-new-mcu
include $(CONTIKI_CPU)/Makefile.my-new-mcu

MODULES += arch/dev/cc1200
```

As before, these are merely the basics to get you started. You will almost certainly want to add more content to your `Makefile.my-platform`, depending on your requirements. Before adding things, always think if there is something already in place that you can re-use. Also think whether adding something to the CPU Makefile is a more sane choice.

#### Provide startup, main loop and low-power functions
Contiki-NG has a platform-independent `main()` routine. The first thing you need to do is provide implementations for platform-specific functions that will be called by `main()` as part Contiki-NG's startup sequence. You will need to provide three functions, commonly in `platform.c`:

* `platform_init_stage_one()`
* `platform_init_stage_two()`
* `platform_init_stage_three()`

See the documentation in [`os/sys/platform.h`](https://github.com/contiki-ng/contiki-ng/tree/develop/os/sys/platform.h) for details of what should go in which of those functions.

You then need to decide whether you want to specify your own main loop (do so only if you have a good reason), or simply use the one provided by Contiki-NG. In the former case, you will need to implement the main loop inside a function called `platform_main_loop()` (again, commonly in `platform.c`) and to add to `my-platform-def.h`:
```C
#define PLATFORM_CONF_PROVIDES_MAIN_LOOP 1
```
One platform that defines its own main loop is the [native](https://github.com/contiki-ng/contiki-ng/tree/develop/arch/platform/native/platform.c) platform.

Lastly, the main loop will periodically try to put your device to a low-power state. This is achieved by calling `platform_idle()`, which is one more function that you will need to provide an implementation for (once again in `platform.c`).

#### Develop platform drivers
At this stage, you can choose to start implementing drivers for your other peripherals, such as:
* LEDs, in a file commonly called `leds-arch.c` (see the HAL/API under [`os/dev/leds.h`](https://github.com/contiki-ng/contiki-ng/tree/develop/os/dev/leds.h))
* Buttons, in a file commonly called `board-buttons.c` (see the HAL/API under [`os/dev/button-hal.h`](https://github.com/contiki-ng/contiki-ng/blob/develop/os/dev/button-hal.h))
* Sensors/Actuators
* External storage (e.g. SPI flash). Have a look at a generic driver available under [`arch/dev/ext-flash`](https://github.com/contiki-ng/contiki-ng/tree/develop/arch/dev/ext-flash), it may prove suitable for your needs.
* Displays (e.g. LCD displays)

#### Add support for similar board variants
Imagine that you are adding support for two very similar, but not quite identical devices. For example, imagine that you are porting Contiki-NG for two devices. Both devices have the same CPU and the same LED and button configuration, but:

* One device has a USB connector and is primarily meant to be used as a border router or slip radio. Let's assume this is called `usb-board`.
* The second device does not have a USB connector, but has additional sensing elements and a battery holder. Let's call this `sensing-board`.

In this scenario, it makes sense to re-use as much as possible platform code for both device variants. In terms of the Contiki-NG build system, the `TARGET` make variable for both devices will be `my-platform`, but the `BOARD` make variable will be different for the two (`usb-board` and `sensing-board` respectively).

The best way to support this is by creating two subdirectories under `arch/platform/my-platform`, each with the name of the respective board. Within each of those subdirectories, you can optionally create a Makefile using the board's name as its extension. So in this example, you will end up with:

* `platform/my-platform/usb-board/Makefile.usb-board`
* `platform/my-platform/sensing-board/Makefile.sensing-board`

Then edit your platform's top-level `Makefile.my-platform` and add this line:
```Makefile
#### Include the board dir if one exists
-include $(PLATFORM_ROOT_DIR)/$(BOARD)/Makefile.$(BOARD)
```

Within these newly-created Makefiles, you can specify board-specific Make variables and, importantly, board-specific source files to be included in the build. You will want to place those source-files in the board-specific directory. An example of this approach is adopted in [`arch/platform/zoul`](https://github.com/contiki-ng/contiki-ng/tree/develop/arch/platform/zoul).

Following our example above, here is what you should put where:
* In `platform/my-platform`: Everything common for both boards, e.g. the LED driver and button driver.
* In `platform/my-platform/sensing-board`: Drivers for the sensing elements on this board. You will append them to `CONTIKI_SOURCEFILES` inside `Makefile.sensing-board`.
* In `platform/my-platform/usb-board`: Drivers for anything specific to the `usb-board`. If there is nothing specific to this board, then note that you do not _have_ to create the sub-directory in the first place.

As a final comment, imagine that you have a number of different boards, featuring partially overlapping sets of peripherals. In this scenario, one approach is to create a directory called `common` under your `arch/platform/my-platform`. You can place all peripheral drivers in this directory and then pick and choose what to compile for each board using the `CONTIKI_SOURCEFILES` make variable within each individual board's `$(BOARD)/Makefile.$(board)`. For an example of this approach, see [`arch/platform/cc26x0-cc13x0`](https://github.com/contiki-ng/contiki-ng/tree/develop/arch/platform/cc26x0-cc13x0).

### Create some examples
You will likely want your users to be able to use some of the existing Contiki-NG platform-independent examples on your device. For those examples, either make sure they run off-the-shelf, or extend them accordingly. If you want an existing example to do something slightly different on your hardware, _do not_ create a copy of the entire example in a separate directory. It is always better to change the examples so it can provide for platform-specific extensions. For some ideas of how this can be achieved, see the [`mqtt-client`](https://github.com/contiki-ng/contiki-ng/tree/develop/examples/mqtt-client), [`sensniff`](https://github.com/contiki-ng/contiki-ng/tree/develop/examples/sensniff) and [`slip-radio`](https://github.com/contiki-ng/contiki-ng/tree/develop/examples/slip-radio) examples.

You will also likely want your users to use examples the fully expose your platform's features (e.g. sensors). Under `examples/platform-specific`, create a sub-directory with the same name as your target's name and create such examples therein. Examples in this location are expected to contain any amount of platform-specific code.

### Add CI tests
As part of this step, you will want to achieve two things:

* Compile-test existing examples for your platform
* Compile-test your new platform-specific examples

All of those can be achieved by adding to the existing `Makefiles` under `tests/NN-compile-XYZ` (e.g. [`tools/01-compile-base/Makefile`](https://github.com/contiki-ng/contiki-ng/tree/develop/tests/01-compile-base/Makefile)). In all cases, you will simply need to add a single line for each example that you wish to compile-test for your platform.

For example, assume that you have developed multiple board variants (eg. `board-a` and `board-b`). If `BOARD` is unspecified, your platform code builds for `board-a` by default. Assume also that:

* You have developed two platform-specific examples under `examples/platform-specific/my-platform` and those examples are `base-example` and `advanced-example`. You want to test those for both boards.
* You want to test [`hello-world`](https://github.com/contiki-ng/contiki-ng/tree/develop/examples/hello-world), and [`rpl-udp`](https://github.com/contiki-ng/contiki-ng/tree/develop/examples/rpl-udp) for `board-a` only.
* You want to test [`rpl-border-router`](https://github.com/contiki-ng/contiki-ng/tree/develop/examples/rpl-border-router) and [`sensniff`](https://github.com/contiki-ng/contiki-ng/tree/develop/examples/sensniff) for `board-b` only.

You then need to add the following lines (not all need to be added to the same `Makefile`/CI job):

```
hello-world/my-platform \
rpl-udp/my-platform \
rpl-border-router/my-platform:BOARD=board-b \
sensniff/my-platform:BOARD=board-b \
platform-specific/my-platform/basic-example/my-platform:BOARD=board-a \
platform-specific/my-platform/basic-example/my-platform:BOARD=board-b \
platform-specific/my-platform/advanced-example/my-platform:BOARD=board-a \
platform-specific/my-platform/advanced-example/my-platform:BOARD=board-b \
```
You do not need to add tests for every single example/board combination, apply common sense in terms of what tests will be sufficient to cover your entire platform/CPU code base. Test what _you_ consider to be the most key examples for your platform.

If you are uncertain in terms of which `Makefile` to add your tests to, don't hesitate to get in touch.
### Add documentation
If you are not planning to release the port then this section is of little relevance. However, if you _are_ planning to release your port and especially if you wish for it to be considered for upstream merge, you will be expected to provide the following:

* A basic how-to page on the wiki, similar to those listed under [The Contiki-NG platforms][wiki-platforms]. Make sure to name long-term maintainers.
* README files in platform-specific examples.
* API docs (doxygen) for new code modules that you may have developed as part of your porting work. This is especially true if you are providing code that is likely to be used by other platform developers. As a bare minimum, you will be expected to document all non-static functions and their arguments, enums and configuration defines. Make sure to also document semantics: For example, don't just say that the argument `foo` can take values `true` or `false`, but also describe the function's expected behaviour for each of those two cases.

If you are planning to contribute your port for inclusion, please also make sure to read "New platforms" in the [Contributing page][wiki-contributing].

## Some common good practice

### Observe the code style convention
If you are planning to contribute your port for inclusion, make sure you observe the code style and naming convention described in the [respective wiki page][wiki-code-style]. 

### Avoid code duplication
Try to avoid code duplication as much as possible. If a source file already exists that does what you need, try to compile it in-place instead of creating a copy of it. If the original source file does almost, but not quite, what you need to do, it is better to recommend modifications to the original file, instead of creating a copy. This is true for platform drivers, but also for examples. In the latter case, Contiki-NG provides mechanisms to write cross-platform examples that only require minor extensions to use on new devices. Before copying an entire example directory, make sure your goal cannot be achieved by extending an existing one.

### Is it a CPU thing, or is it a platform thing?
Try to put the correct code module / configuration macro / Make code at the correct location. For example, if you introduce a user configuration macro that is likely to be applicable to all boards using the same CPU, put this macro in the respective file in your CPU directory, instead of the platform directory. In the long term, this will prevent duplication and make code easier to maintain.

### Do not add platform code in platform-independent files
Do _not_ wrap platform-specific code inside `#if` blocks in platform-independent code files (files under `os` and `examples`). Imagine for example the following snippet:

```C
static void
platform_independent_function(void)
{
  /* Do platform independent stuff */

#if CONTIKI_TARGET_MY_NEW_PLATFORM
  platform_foo_function();
#endif

  /* Do remaining platform independent stuff */
}
```

Even though we have some examples of this coding practice in the current code base, the practice is _strongly_ discouraged. The preferred way to achieve the same goal is situation-dependent; if you absolutely need to make a change of this nature and you are stuck as to how to achieve this in a portable fashion, don't hesitate to get in touch for advice.

## Support
Feel free to ask your porting questions in the "Developers" room on [gitter](https://gitter.im/contiki-ng).

[tutorial:hello-world]: /doc/tutorials/Hello,-World!
[tutorial:shell]: /doc/tutorials/Shell
[wiki-platforms]: /doc/platforms/index.rst
[wiki-code-style]: /doc/project/Code-style
[wiki-contributing]: /doc/project/Contributing
[doc:tsch]: TSCH-and-6TiSCH.md#porting-tsch-to-a-new-platform
[doc:build-system]: /doc/getting-started/The-Contiki-NG-build-system
[doc:multitasking-and-scheduling]: Multitasking-and-scheduling.md#writing-interrupt-handlers
