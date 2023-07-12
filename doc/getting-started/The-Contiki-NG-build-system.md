# The Contiki‐NG build system

The Contiki-NG build system is designed to compile a full system image to a variety of hardware platforms. The resulting binary includes both the application code and the Contiki-NG Operating System code.

A simple example project Makefile can be found in `examples/hello-world/`:
```
CONTIKI_PROJECT = hello-world
all: $(CONTIKI_PROJECT)
CONTIKI = ../..
include $(CONTIKI)/Makefile.include
```
After defining the project name and Contiki-NG path, the common makefile is simply included. By running `make` from this directory, the `hello-world` system will be build for the default target: `native`. The `native` target is a special platform that builds Contiki-NG as a program able to run on the development system (e.g. Linux). Just run the file `hello-world.native` after compiling it.

To compile the hello-world application for a different platform, use the flag `TARGET`, e.g. with `TARGET=zoul`. On some platforms, an additional `BOARD` is necessary to further specify the target.

## Build system output structure
The build system will generate a number of files that will be placed in a number of locations. This guide uses the `hello-world` example as a use-case, but the output will be the same for all other examples.

The build system will always put the final build output file (firmware) in the example's `.` directory. This is the firmware to use to programme your device (embedded targets) or to execute (native paltform). For the hello-world example, this will be called `hello-world.$(TARGET)` (e.g. `hello-world.native` or `hello-world.zoul`.

The build system will also generate a `build/` directory, and within it various sub-directories, where it will place intermediate files. The logic is as follows:

* A directory named `build/$(TARGET)` will always be created (e.g. `build/native` or `build/zoul`).
* If the platform supports the `BOARD` make variable, then a board-specific sub-directory will also be created under `build/$(TARGET)`. For example `build/zoul/remote-reva` or `build/cc26x0-cc13x0/launchpad/cc2650`.
* You may wish to switch between different configurations while building an example. For instance, you may wish to have a build to be used for debugging and testing and a separate build to be used for deployment. The Contiki-NG supports this through the `BUILD_DIR_CONFIG` make variable, to which you can assign different values for your different builds. If you do choose to use `BUILD_DIR_CONFIG`, then the build system will create a further subdirectory called `$(BUILD_DIR_CONFIG)/` under `build/$(TARGET)/[ $(BOARD) ]/`. For example `build/zoul/remote-reva/deploy` if you run `make BUILD_DIR_CONFIG=deploy`.
* Lastly, a further sub-directory called `obj` will be created under `build/$(TARGET)/[ $(BOARD) ]/[ $(BUILD_DIR_CONFIG) ]`. For example `build/cc26x0-cc13x0/launchpad/cc2650/debug/obj`.

With the above sub-directory structure in mind, the build system will output files as follows:
* All compilation output files (`.o`) and dependency files (`.d`) will be placed under the `obj/` dir.
* All link and post-processing out files (e.g. `hello-world.elf`, `hello-world.hex`, `hello-world.bin`) will be placed in `build/$(TARGET)/[ $(BOARD) ]/[ $(BUILD_DIR_CONFIG) ]`. In the same directory, you will often also file a `.map` file. Lastly, this directory will also always host a copy of the `hello-world.$(TARGET)` file.

## Cleaning
To clean built artifacts, use:
* `clean`: Removes all build output files for `TARGET`, including the entire `build/$(TARGET)` dir. There is currently no way to only clean files built for a specific `BOARD`.
* `distclean`: Cleans all build output files for all Contiki-NG targets and also deletes the entire `build/` directory. It basically invokes `make TARGET=xyz clean` for all Contiki-NG supported platforms and once this has finished it then force-removes the `build/` directory.

## More options

The following `make` target are provided:
* `viewconf`: shows the value of important Make and C constants, for inspection of the Contiki-NG configuration
* `targets`: shows the list of supported `TARGET`
* `boards`: shows the list of supported `BOARD` for the current `TARGET`
* `savetarget`: saves the current `TARGET` and `BOARD` under `Makefile.target`, for use in subsequent runs of make
* `savedefines`: saves the current `DEFINES` flags for subsequent runs of make
* `%.o`: produces an object file from a given source file
* `%.e`: produces the pre-processed version of a given source file
* `%.s`: produces an assembly file from a given source file
* `%.ramprof`: shows a RAM profile of a given firmware
* `%.flashprof`: shows a Flash/ROM profile of a given firmware
* `login`: view the serial output of the device connected to PORT
* `serialview`: same as login, but prepend serial output with a unix timestamp
* `serialdump`: same as serialview, but also save the output to a file
* `motelist-all`: prints a list of connected devices
* `usage` and `help`: show a brief help

## Makefiles used in the Contiki-NG build system

The Contiki-NG build system is composed of a number of Makefiles. These are:

* `Makefile`: the project's makefile, located in the project directory.  This Makefile can define the `MODULES` variable, a list of modules that should be included. It can also configure the Contiki-NG networking stack (see [The Contiki-NG configuration system](The-Contiki-NG-configuration-system.md)), as well as any other standard Makefile rules and flags.
* Top-level Makefiles, located in Contiki-NG's source tree's root:
  * `Makefile.include`: the system-wide makefile. When make is run, `Makefile.include` includes the `Makefile.$(TARGET)` as well as all makefiles for the modules in the `MODULES` list (which is specified by the project's Makefile).
  * `Makefile.help`: Contains definitions of the `help` and `usage` targets.
  * `Makefile.tools`: Some build have dependencies on utilities under the `tools` dir. This Makefile makes sure those dependencies get built when required.
  * `Makefile.embedded`: Contains additional Makefile logic that applies to all of Contiki-NG's embedded platforms; that is all platforms except `native` and `cooja`.
  * `Makefile.identify-target`: This Makefile can be used to identify the selected `TARGET` used for a specific build. It can be included by example Makefiles that need to take decisions based on the value of the `TARGET` make variable.
* `Makefile.$(TARGET)` (where `$(TARGET)` is the name of the platform): rules for the specific platform, located in the platform's subdirectory in `arch/platform`. It contains the list of C files (`CONTIKI_TARGET_SOURCEFILES`) that the platform adds to the Contiki-NG system. The `Makefile.$(TARGET)` also includes the `Makefile.$(CPU)` from the `arch/cpu/$(CPU)/` directory.
* `Makefile.$(CPU)` (where `$(CPU)` is the name of the CPU/MCU): rules for the CPU architecture, located in the CPU architecture's subdirectory in `arch/cpu/`.
* `Makefile.$(MODULE)` (where `$(MODULE)` is the name of a module in the `os` directory): optional, module-specific makefile rules.
