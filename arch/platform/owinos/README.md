Oppila Microsystems - http://www..in
---------------------------------------------------------------

The Owinos platform boards are based on CC2538 SoC (System on Chip), featuring an ARM Cortex-M3 running at 32 MHz and with 32 kbytes of RAM and 512 kbytes of FLASH. It has the following key features:

  * On-board analog and digital sensors
  * ISM 2.4-GHz IEEE 802.15.4 & Zigbee compliant.
  * AES-128/256, SHA2 Hardware Encryption Engine.
  * ECC-128/256, RSA Hardware Acceleration Engine for Secure Key Exchange.
  * Power input with wide range 3.7-30VDC.
  * small form factor (43mm x 34mm)

Port Features

The owinos platform boards has the following key features:

  * Deep Sleep support with RAM retention for ultra-low energy consumption.
  * Native USB support (CDC-ACM). SLIP over UART for border routers is no longer a bottleneck.
  * DMA transfers for increased performance (RAM to/from RF, RAM to/from USB).

In terms of hardware support, the following drivers have been implemented for the owinos platform:

    CC2538 System-on-Chip:
        Standard Cortex M3 peripherals (NVIC, SCB, SysTick)
        Sleep Timer (underpins rtimers)
        SysTick (underpins the platform clock and Contiki's timers infrastructure)
        RF (2.4GHz)
        UART
        Watchdog (in watchdog mode)
        USB (in CDC-ACM)
        uDMA Controller (RAM to/from USB and RAM to/from RF)
        Low power modes
        Random number generator
        General-Purpose Timers. NB: GPT0 is in use by the platform code, the remaining GPTs are available for application development.
        ADC
        Cryptoprocessor (AES-ECB/CBC/CTR/CBC-MAC/GCM/CCM-128/192/256, SHA-256)
        Public Key Accelerator (ECDH, ECDSA)
        Flash-based port of Coffee
        LEDs
        Buttons
 
Check the board's specific README files for more information.

To start using Contiki with the Owinos platform boards, the following is required:

    An Owinos platform board.
    A toolchain to compile Contiki for the CC2538.
    Drivers so that your OS can communicate with your hardware.
    Software to upload images to the CC2538

Install a Toolchain

	The toolchain used to build contiki is arm-none-eabi-gcc, also used by other arm-based Contiki ports. If you are using Instant Contiki, you may have a	      version pre-installed in your system.

The platform is currently being used/tested with "GNU Tools for ARM Embedded Processors" (https://launchpad.net/gcc-arm-embedded). The current recommended version and the one being used by Contiki's regression tests on Travis is shown below.

	$ arm-none-eabi-gcc --version
	arm-none-eabi-gcc (GNU Tools for ARM Embedded Processors) 5.2.1 20151202 (release) [ARM/embedded-5-branch revision 231848]
	Copyright (C) 2015 Free Software Foundation, Inc.
	This is free software; see the source for copying conditions.  There is NO
	warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

Drivers

	The owinos platform boards can be programmed and debugged over JTAG and USB. The board has a CP2104 serial-to-USB module,the driver is commonly found 	      in most OS, but if required it can be downloaded from 
	
	<https://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx>

	The CC2538 USB Vendor and Product IDs are the following:

    	    VID 0x0451
	    PID 0x16C8

The implementation in Contiki is pure CDC-ACM: The Linux and OS X kernels know exactly what to do and drivers are not required.

On windows, you will need to provide a driver. You have two options:

    Use the signed or unsigned driver provided by TI in CC2538 Foundation Firmware. You will find them both under the drivers directory.
    Download a generic Virtual Serial Port driver and modify it so it works for the CC2538.

For the latter option:

    Download this LUFA CDC-ACM driver.
    Adjust the VID and PID near the end with the values at the start of this section.
    Next time you get prompted for the driver, include the directory containing the .inf file in the search path and the driver will be installed.

Software to Program the Nodes

	The owinos platform boards can be programmed via the jtag interface or via the serial boot loader on the chip.

	Both the OMote and ORouter has a mini JTAG 10-pin male header, which can be used to flash and debug the platforms. 

	The serial boot loader on the chip is exposed to the user via the USB interface.The board has built-in support to flash over USB using the BSL.It 
	requires to enter into the bootloader mode  by manually pressing the `BSL button` and `reset button` in hand-shaking method.


Instructions to flash for different OS are given below.

    On Windows:
       * Nodes can be programmed with TI's ArmProgConsole Flash Programmer 2. The README should be self-explanatory. With ArmProgConsole, 
	 upload the file with a .bin extension. (jtag + serial)
       * Nodes can also be programmed via the serial boot loader in the cc2538. In tools/cc2538-bsl/ you can find cc2538-bsl.py this is a python script that 
	 can download firmware to your node via a serial connection. If you use this option you just need to make sure you have a working version of python 	       installed.

    On Linux:
        Nodes can be programmed with JFlashLite. With JFlashLite, use the file with .hex extension. (jtag)
        Nodes can also be programmed via the serial boot loader in the cc2538. cc2538-bsl is used. No extra software needs to be installed.  

    On OSX:
        The cc2538-bsl.py script in tools/cc2538-bsl/ is the only option. No extra software needs to be installed. (serial)

The file with a .owinos extension is a copy of the .elf file.

Use the Port

The following examples are intended to work off-the-shelf:

    Examples under examples/platform-specific/owinos/
    MQTT example examples/cc2538dk/mqtt-demo
    Border router: examples/ipv6/rpl-border-router
    Webserver: examples/webserver-ipv6
    CoAP example: examples/er-rest-example/

Build your First Examples

	It is recommended to start with the owinos-demo, it is a simple example that explores the owinos platform features (can be compiled for both the 
	OMote and the ORouter), such as the LEDs, user button operation modes (press, release, hold-press), radio (Rime broadcast).

	The Makefile.target includes the TARGET= argument, predefining which is the target platform to compile for, it is automatically included at 
	compilation. The BOARD= argument is used to pull in specific platform files, for example the specific OMote core drivers. If no BOARD argument is 
	given, it will default to omote and compile for the OMote platform.

	To generate or override an existing one, you can run:

	make TARGET=owinos savetarget

	Then you can just run make to compile an application, otherwise you will need to do make TARGET=owinos.

	Alternatively you can export the following to your work environment:

	export BOARD=omote or export BOARD=orouter

	This will avoid having to type this argument at each compilation.

	If you want to upload the compiled firmware to a node via the serial boot loader you need to first manually enable the boot loader. To enter into the 	      bootlaoder mode, press the `BSL button` and `reset button` using hand-shaking method. 

	Hand-shaking method - Press and hold reset button and then press and release BSL button and then release the reset button now

	Then use make owinos-demo.upload.

	The PORT argument could be used to specify in which port the device is connected, in case we have multiple devices connected at the same time.

	To generate an assembly listing of the compiled firmware, run make owinos-demo.lst. This may be useful for debugging or optimizing your application 
	code. To intersperse the C source code within the assembly listing, you must instruct the compiler to include debugging information by adding 
	CFLAGS += -g to the project Makefile and rebuild by running make clean owinos-demo.lst.

	To enable printing debug output to your console, use the make login to get the information over the USB programming/debugging port, or alternatively 
	use make serialview to also add a timestamp in each print.
	
Node IEEE/RIME/IPv6 Addresses

	Nodes will generally autoconfigure their IPv6 address based on their IEEE address. The IEEE address can be read directly from the CC2538 Info Page, or 	      it can be hard-coded. Additionally, the user may specify a 2-byte value at build time, which will be used as the IEEE address' 2 LSBs.

	To configure the IEEE address source location (Info Page or hard-coded), use the IEEE_ADDR_CONF_HARDCODED define in contiki-conf.h:

	    0: Info Page
	    1: Hard-coded

	If IEEE_ADDR_CONF_HARDCODED is defined as 1, the IEEE address will take its value from the IEEE_ADDR_CONF_ADDRESS define. If IEEE_ADDR_ CONF_HARDCODED 	      is defined as 0, the IEEE address can come from either the primary or secondary location in the Info Page. To use the secondary address, define 
	IEEE_ADDR_CONF_USE_SECONDARY_LOCATION as 1.

	Additionally, you can override the IEEE's 2 LSBs, by using the NODEID make variable. The value of NODEID will become the value of the 
	IEEE_ADDR_NODE_ID pre-processor define. If NODEID is not defined, IEEE_ADDR_NODE_ID will not get defined either. For example:

	make NODEID=0x79ab

	This will result in the 2 last bytes of the IEEE address getting set to 0x79 0xAB

	Note: Some early production devices do not have an IEEE address written on the Info Page. For those devices, using value 0 above will result in a Rime 	      address of all 0xFFs. If your device is in this category, define IEEE_ADDR_CONF_HARDCODED to 1 and specify NODEID to differentiate between devices.

Low-Power Modes

	The CC2538 port supports power modes for low energy consumption. The SoC will enter a low power mode as part of the main loop when there are no more 
	events to service.

	LPM support can be disabled in its entirety by setting LPM_CONF_ENABLE to 0 in contiki-conf.h or project-conf.h.

	The Low-Power module uses a simple heuristic to determine the best power mode, depending on anticipated Deep Sleep duration and the state of various 
	peripherals.

In a nutshell, the algorithm first answers the following questions:

    		Is the RF off?
    		Are all registered peripherals permitting PM1+?
    		Is the Sleep Timer scheduled to fire an interrupt?

	If the answer to any of the above question is "No", the SoC will enter PM0. If the answer to all questions is "Yes", the SoC will enter one of PMs 
	0/1/2 depending on the expected Deep Sleep duration and subject to user configuration and application requirements.

	At runtime, the application may enable/disable some Power Modes by making calls to lpm_set_max_pm(). For example, to avoid PM2 an application could 
	call lpm_set_max_pm(1). Subsequently, to re-enable PM2 the application would call lpm_set_max_pm(2).

	The LPM module can be configured with a hard maximum permitted power mode.

		#define LPM_CONF_MAX_PM        N

	Where N corresponds to the PM number. Supported values are 0, 1, 2. PM3 is not supported. Thus, if the value of the define is 1, the SoC will only 
	ever enter PMs 0 or 1 but never 2 and so on.

	The configuration directive LPM_CONF_MAX_PM sets a hard upper boundary. For instance, if LPM_CONF_MAX_PM is defined as 1, calls to lpm_set_max_pm() 
	can only enable/disable PM1. In this scenario, PM2 can not be enabled at runtime.

	When setting LPM_CONF_MAX_PM to 0 or 1, the entire SRAM will be available. Crucially, when value 2 is used the linker will automatically stop using 	      the SoC's SRAM non-retention area, resulting in a total available RAM of 16kbytes instead of 32kbytes.

LPM and Duty Cycling Driver

	LPM is highly related to the operations of the Radio Duty Cycling (RDC) driver of the Contiki network stack and will work correctly with ContikiMAC 	      and NullRDC.

        With ContikiMAC, PMs 0/1/2 are supported subject to user configuration.
    	When NullRDC is in use, the radio will be always on. As a result, the algorithm discussed above will always choose PM0 and will never attempt to 	      drop to PM1/2.

Build headless nodes

	It is possible to turn off all character I/O for nodes not connected to a PC. Doing this will entirely disable the UART as well as the USB controller,        preserving energy in the long term. The define used to achieve this is (1: Quiet, 0: Normal output):

		#define CC2538_CONF_QUIET      0

	Setting this define to 1 will automatically set the following to 0:

    	USB_SERIAL_CONF_ENABLE
    	UART_CONF_ENABLE
    	STARTUP_CONF_VERBOSE

Code Size Optimisations

	The build system currently uses optimization level -Os, which is controlled indirectly through the value of the SMALL make variable. This value can be 	      overridden by example makefiles, or it can be changed directly in platform/owinos/Makefile.owinos

	Historically, the -Os flag has caused problems with some toolchains. If you are using one of the toolchains documented in this README, you should be 
	able to use it without issues. If for whatever reason you do come across problems, try setting SMALL=0 or replacing -Os with -O2 in cpu/cc2538/Makefile.cc2538.

Doxygen Documentation

	This port's code has been documented with doxygen. To build the documentation, navigate to $(CONTIKI)/doc and run make. This will build the entire 
	contiki documentation and may take a while.

	If you want to build this platform's documentation only and skip the remaining platforms, run this:

	make basedirs="platform/owinos core cpu/cc2538 examples/owinos examples/cc2538dk"

	Once you've built the docs, open $(CONTIKI)/doc/html/index.html and enjoy.

Other Versions of this Guide

If you prefer this guide in other formats, use the excellent [pandoc] to convert it.

        pdf: pandoc -s --toc README.md -o README.pdf
        html: pandoc -s --toc README.md -o README.html

More Reading

    Oppila website
    CC2538 System-on-Chip Solution

Maintainers

The Owinos platform is maintained by Oppila Microsystems pvt ltd
Boards are available at sales@oppila.in – Oppila Microsystems pvt ltd Banglore

