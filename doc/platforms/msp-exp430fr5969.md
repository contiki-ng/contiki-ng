# msp-exp430fr5969: TI MSP-EXP430FR5969 LaunchPad

This guide describes the Contiki-NG port for the Texas Instruments
MSP-EXP430FR5969 LaunchPad development kit featuring the MSP430FR5969
microcontroller with FRAM (Ferroelectric RAM).

## Overview

The MSP430FR5969 is a 16-bit ultra-low-power microcontroller with 64KB of
non-volatile FRAM. Unlike traditional Flash memory, FRAM provides fast,
low-power writes with virtually unlimited write endurance, making it ideal
for data logging and intermittent computing applications.

Key hardware features:
* MSP430X CPU: 16-bit data path with 20-bit addressing (1 MB address
  space), running at 8 MHz
* 64KB FRAM (unified code and data memory, non-volatile)
* 2KB SRAM
* eUSCI_A (UART) and eUSCI_B (SPI/I2C) serial modules
* 12-bit ADC with internal reference
* 2 LEDs (Red on P1.0, Green on P4.6)
* 2 push buttons (S1 on P4.5, S2 on P1.1)
* On-board eZ-FET debugger with backchannel UART
* EnergyTrace++ for power profiling

This platform has **no radio**, so networking features are disabled by default.
It is designed for standalone sensing and data logging applications.

## Port Features

The following features have been implemented:

* Contiki-NG system clock and rtimers
* UART driver (115200 baud via backchannel)
* Watchdog driver
* LED driver (Red and Green LEDs)
* GPIO HAL (pin configuration and port interrupts)
* Button HAL (both buttons, with press, release, periodic and long-press events)
* Low-power modes (LPM3 when idle)

The port is organized as follows:
* Platform-specific files are in `arch/platform/msp-exp430fr5969/`
* MSP430 CPU files are in `arch/cpu/msp430/`
* Platform uses the FR5xxx-specific Clock System (CS) module

## Prerequisites and Setup

To compile for the MSP430FR5969 you'll need:

### MSP430 Toolchain

This platform builds with the mspgcc toolchain (the `msp430-gcc` command,
GCC 4.7.x) — the same toolchain used for the other MSP430 platforms (Z1, Sky)
and for the MSPSim emulator in Cooja. Install it by following
[Toolchain installation on Linux](../getting-started/Toolchain-installation-on-Linux.md)
(or the macOS guide).

Two platform-specific notes: the `gcc-msp430` package in the Ubuntu/Debian
repositories is too old to support the MSP430X memory model the FR5969 uses,
and TI's MSP430-GCC-OPENSOURCE toolchain provides a different command
(`msp430-elf-gcc`) that this build system does not use.

### mspdebug

For programming and debugging, install mspdebug with TI library support:

```bash
# Ubuntu/Debian
sudo apt-get install mspdebug

# May also need the TI MSP Debug Stack library for tilib driver
```

### USB Permissions (Linux)

To access the LaunchPad without root, add a udev rule that grants
access to the local console user via `uaccess`. This is safer than a
world-writable `MODE="0666"` rule on multi-user systems:

```bash
# Create /etc/udev/rules.d/71-ti-permissions.rules with:
SUBSYSTEM=="usb", ATTRS{idVendor}=="2047", TAG+="uaccess"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0451", TAG+="uaccess"

# Reload rules
sudo udevadm control --reload-rules
```

On systems without `uaccess` support, grant access via a group instead,
for example `MODE="0660", GROUP="plugdev"`, and add the user to that
group.

## Getting Started

### Building

To compile the hello-world example:

```bash
cd examples/hello-world
make TARGET=msp-exp430fr5969
```

Besides `hello-world`, the `dev/leds`, `dev/gpio-hal`, `dev/button-hal`,
`libs/shell` and `libs/timers` examples also build and run on this platform.

### Programming

Upload the firmware using mspdebug:

```bash
make TARGET=msp-exp430fr5969 hello-world.upload
```

Or program directly:

```bash
mspdebug tilib "prog build/msp-exp430fr5969/hello-world.msp-exp430fr5969"
```

### Reset

Reset the device without reprogramming:

```bash
make TARGET=msp-exp430fr5969 reset
```

### Serial Output

Connect to the backchannel UART at 115200 baud. The LaunchPad appears as
two USB serial ports: `/dev/ttyACM0` (debug) and `/dev/ttyACM1` (application UART).

```bash
make TARGET=msp-exp430fr5969 PORT=/dev/ttyACM1 login
```

Or use any serial terminal:

```bash
picocom -b 115200 /dev/ttyACM1
```

**Note:** After a debugger reset, there is a ~250ms delay before serial output
begins. This allows the eZ-FET debug probe to switch from debug mode to UART
passthrough mode.

## Compilation Options

The TARGET name for this port is `msp-exp430fr5969`:

```bash
make TARGET=msp-exp430fr5969
```

### Compilation Targets

| Target | Description |
|--------|-------------|
| `<app>` | Build the application |
| `<app>.upload` | Build and program via mspdebug |
| `reset` | Reset the device |
| `login` | Connect to serial port |
| `clean` | Remove build files |

## Hardware Connections

### LEDs

| LED | Port.Pin | Contiki Macro |
|-----|----------|---------------|
| Red | P1.0 | `LEDS_RED` |
| Green | P4.6 | `LEDS_GREEN` |

### Buttons

| Button | Port.Pin | Button HAL ID |
|--------|----------|---------------|
| S1 | P4.5 | `BUTTON_HAL_ID_BUTTON_ZERO` |
| S2 | P1.1 | `BUTTON_HAL_ID_BUTTON_ONE` |

Both buttons short to ground when pressed and use the MSP430 internal
pull-ups (active low). They are exposed through the Button HAL; see the
`examples/dev/button-hal` example.

### UART (Backchannel)

| Signal | Port.Pin | eZ-FET Connection |
|--------|----------|-------------------|
| TXD | P2.0 | Application UART TX |
| RXD | P2.1 | Application UART RX |

The backchannel UART is directly connected to the eZ-FET debug probe and
appears as a USB CDC device on the host.

## Limitations

* **No radio / no IP networking**: This LaunchPad has no wireless
  transceiver, so the platform uses NullNet/NullRouting and keeps IPv6 and
  IPv4 disabled. The IP stacks also do not fit the 2 KB SRAM. Building an
  example that enables `NETSTACK_CONF_WITH_IPV6` or `NETSTACK_CONF_WITH_IPV4`
  fails at compile time with an explicit `#error` from `contiki-conf.h`
  rather than an obscure error; such examples (e.g. `rpl-udp`, `coap`) are
  not supported on this board.
* **Limited SRAM**: Only 2KB of SRAM limits the complexity of applications.
* **No external sensors**: The base LaunchPad has no external sensors.
  BoosterPacks can add sensor capabilities.

## Troubleshooting

### mspdebug hangs

The `tilib` driver can occasionally hang. The reset target includes a
10-second timeout. If programming hangs, disconnect and reconnect the USB
cable.

### Garbled serial output after reset

Ensure you're using `/dev/ttyACM1` (not ACM0) for the application UART.
The platform includes a startup delay to avoid garbled output, but the
first few characters may still be lost if the serial terminal isn't ready.

### Build errors about missing headers

Ensure the MSP430 GCC toolchain is properly installed and the `msp430.h`
header for the FR5969 is available.

## Resources

* [MSP-EXP430FR5969 LaunchPad User's Guide](https://www.ti.com/lit/ug/slau535d/slau535d.pdf)
* [MSP430FR5969 Datasheet](https://www.ti.com/lit/ds/symlink/msp430fr5969.pdf)
* [MSP430FR5xx/6xx Family User's Guide](https://www.ti.com/lit/ug/slau367p/slau367p.pdf)
* [MSP430-GCC-OPENSOURCE](https://www.ti.com/tool/MSP430-GCC-OPENSOURCE) — TI's `msp430-elf-gcc` toolchain (for reference; this port uses mspgcc `msp430-gcc`, see the toolchain note above)

## License

All files in this port are under BSD license.
