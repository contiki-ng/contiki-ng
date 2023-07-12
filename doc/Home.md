# Contiki-NG Documentation

Contiki-NG is an operating system for resource-constrained devices in the Internet of Things. Contiki-NG contains an RFC-compliant, low-power IPv6 communication stack, enabling Internet connectivity. The system runs on a variety of platforms based on energy-efficient architectures such as the ARM Cortex-M3/M4 and the Texas Instruments MSP430. The code footprint is on the order of a 100 kB, and the memory usage can be configured to be as low as 10 kB. The source code is available as open source with a [3-clause BSD license](/doc/project/License).

[More about Contiki-NG](/doc/project/More-about-Contiki-NG.md).

Online presence:
* GitHub repository: https://github.com/contiki-ng/contiki-ng
* Documentation: https://contiki-ng.readthedocs.io/en/develop/
* Gitter: https://gitter.im/contiki-ng
* Contiki-NG tag on Stack Overflow: https://stackoverflow.com/questions/tagged/contiki-ng
* Twitter: https://twitter.com/contiki_ng
* Web site: http://contiki-ng.org
* Nightly testbed runs: https://contiki-ng.github.io/testbed

Also feel free to download our [cheat sheet](https://contiki-ng.github.io/resources/contiki-ng-cheat-sheet.pdf)!

## Documentation

### Setting up Contiki-NG

* [Docker image](/doc/getting-started/Docker.md)
* [Vagrant image](/doc/getting-started/Vagrant.md)
* [Native toolchain installation (Linux)](/doc/getting-started/Toolchain-installation-on-Linux.md)
* [Native toolchain installation (macOS)](/doc/getting-started/Toolchain-installation-on-macOS.md)
* [IP64 setup with Jool](/doc/getting-started/NAT64-for-Contiki-NG.md)
* [The Contiki-NG build system](/doc/getting-started/The-Contiki-NG-build-system.md)
* [The Contiki-NG configuration system](/doc/getting-started/The-Contiki-NG-configuration-system.md)
* [The Contiki-NG logging system](/doc/getting-started/The-Contiki-NG-logging-system.md)

### Programming Contiki-NG

* [Repository structure](/doc/programming/Repository-structure.md)
* [Multitasking and scheduling](/doc/programming/Multitasking-and-scheduling.md)
* [Processes and events](/doc/programming/Processes-and-events.md)
* [Synchronization primitives](/doc/programming/Synchronization-primitives.md)
* [Timers](/doc/programming/Timers.md)
* [Memory management](/doc/programming/Memory-management.md)
* [Packet buffers](/doc/programming/Packet-buffers.md)
* [Energy monitoring with Energest](/doc/programming/Energest.md)
* [UDP communication](/doc/programming/UDP-communication.md)
* [IPv6-less networking with NullNet](/doc/programming/NullNet.md)
* [A guide on porting Contiki-NG to new platforms](/doc/programming/Porting-Contiki-NG-to-new-platforms.md)
* [API Documentation](/doxygen.rst)

### Key networking modules and services

* [IPv6 networking stack](/doc/programming/IPv6.md)
* [IPv6 multicast](/doc/programming/IPv6-multicast.md)
* [The RPL routing protocol](/doc/programming/RPL.md)
* [CoAP and CoAPs](/doc/programming/CoAP.md)
* [OMA LWM2M](/doc/programming/LWM2M.md)
* [TSCH and 6TiSCH](/doc/programming/TSCH-and-6TiSCH.md)
* [6TiSCH 6top sublayer](/doc/programming/6TiSCH-6top-sub-layer.md)
* [6TiSCH scheduler Orchestra](/doc/programming/Orchestra.md)
* [IPv6 over BLE with BLEach](/doc/programming/IPv6-over-BLE.md)
* [Communication security](/doc/programming/Communication-Security.md)
* [SNMP](/doc/programming/SNMP.md)

### Storage systems
* [Coffee File System](/doc/programming/Coffee.md)
* [Antelope database management system](/doc/programming/Antelope.md)

### The Contiki-NG platforms

* [cc2538dk: TI cc2538 development kit](/doc/platforms/cc2538dk.md)
* [cc26x0-cc13x0: TI cc26x0 and cc13x0 platforms](/doc/platforms/cc26x0-cc13x0.md)
* [cooja: Cooja native motes platform](/doc/platforms/cooja.md)
* [Gecko: Silicon Labs MCU Platform](/doc/platforms/gecko.md)
* [native: Contiki-NG as a native process](/doc/platforms/native.md)
* [nrf52840: Nordic Semiconductor nRF52840](/doc/platforms/nrf52840.md)
* [nrf: Nordic Semiconductor nRF5340 and nRF52840 (using nRF MDK)](/doc/platforms/nrf.md)
* [openmote-cc2538: OpenMote cc2538](/doc/platforms/openmote-cc2538.md)
* [simplelink: TI SimpleLink MCU Platform](/doc/platforms/simplelink.md)
* [sky: Tmote Sky / TelosB](/doc/platforms/sky.md)
* [zoul: Zolertia Zoul platforms: Firefly, RE-mote and Orion](/doc/platforms/zolertia/zoul.md)

## Tutorials

Basics:
* [Hello, World!](/doc/tutorials/Hello,-World!.md)
* [Logging](/doc/tutorials/Logging.md)
* [RAM and ROM usage](/doc/tutorials/RAM-and-ROM-usage.md)
* [NG shell](/doc/tutorials/Shell.md)
* [Timers and events](/doc/tutorials/Timers-and-events.md)
* [Simple energy usage estimation](/doc/tutorials/Instrumenting-Contiki-NG-applications-with-energy-usage-estimation.md)
* [Custom Energest application](/doc/tutorials/Energy-monitoring.md)

Networking:
* [CoAP](/doc/tutorials/CoAP.md)
* [IPv6 ping](/doc/tutorials/IPv6-ping.md)
* [RPL basics](/doc/tutorials/RPL.md)
* [RPL with border router](/doc/tutorials/RPL-border-router.md)
* [LWM2M, IPSO objects, and NAT64](/doc/tutorials/LWM2M-and-IPSO-Objects.md)
* [LWM2M Queue Mode](/doc/tutorials/LWM2M-and-IPSO-Objects-with-Queue-Mode.md)
* [MQTT](/doc/tutorials/MQTT.md)
* [Switching from CSMA to TSCH](/doc/tutorials/Switching-to-TSCH.md)
* [TSCH and 6TiSCH](/doc/tutorials/TSCH-and-6TiSCH.md)

Simulation:
* [Cooja: getting started](/doc/tutorials/Running-Contiki-NG-in-Cooja.md)
* [Cooja: simulating a RPL network](/doc/tutorials/Running-a-RPL-network-in-Cooja.md)
* [Cooja: simulating a RPL network with a border router](/doc/tutorials/Cooja-simulating-a-border-router.md)
* [Running Contiki-NG in Renode](/doc/tutorials/Running-Contiki-NG-in-Renode.md)

## Organization, etc.

* [Organization](/doc/project/Organization.md)
* [Contributing](/doc/project/Contributing.md)
* [Code style](/doc/project/Code-style.md)
* [Development cycle](/doc/project/Development-cycle.md)
* [Releases](https://github.com/contiki-ng/contiki-ng/releases)
* [Roadmap](/doc/project/Roadmap.md)
* [Issue and PR labels](/doc/project/Issue-and-Pull-Request-Labels.md)
* [Where to report issues, ask questions, etc.?](/doc/project/Where-to-report-issues,-ask-questions,-etc.md)
* [Logo](/doc/project/Logo.md)


## Citing Contiki-NG in Academic Manuscripts

If you are publishing academic work and you wish to cite Contiki-NG, here is how to do so:

Text:
> George Oikonomou, Simon Duquennoy, Atis Elsts, Joakim Eriksson, Yasuyuki Tanaka, Nicolas Tsiftes, "The Contiki-NG open source operating system for next generation IoT devices", SoftwareX, 18, 2022, https://doi.org/10.1016/j.softx.2022.101089.

BibTeX:

    @article{Contiki-NG,
      title = {The {Contiki-NG} open source operating system for next generation {IoT} devices},
      author = {George Oikonomou and Simon Duquennoy and Atis Elsts and Joakim Eriksson and Yasuyuki Tanaka and Nicolas Tsiftes},
      journal = {SoftwareX},
      volume = {18},
      pages = {101089},
      year = {2022},
      issn = {2352-7110},
      doi = {https://doi.org/10.1016/j.softx.2022.101089},
      keywords = {Contiki-NG, Internet of Things, Resource-Constrained Devices},
    }
