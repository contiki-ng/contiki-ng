Contiki-NG is an operating system for resource-constrained devices in the Internet of Things. Contiki-NG contains an RFC-compliant, low-power IPv6 communication stack, enabling Internet connectivity. The system runs on a variety of platforms based on energy-efficient architectures such as the ARM Cortex-M3/M4 and the Texas Instruments MSP430. The code footprint is on the order of a 100 kB, and the memory usage can be configured to be as low as 10 kB. The source code is available as open source with a [3-clause BSD license](https://github.com/contiki-ng/contiki-ng/wiki/License).

More about Contiki-NG at [doc:more-about-contiki-ng].

Online presence:
* GitHub repository: https://github.com/contiki-ng/contiki-ng
* Documentation: https://github.com/contiki-ng/contiki-ng/wiki
* Gitter: https://gitter.im/contiki-ng
* Contiki-NG tag on Stack Overflow: https://stackoverflow.com/questions/tagged/contiki-ng
* Twitter: https://twitter.com/contiki_ng
* Web site: http://contiki-ng.org
* Nightly testbed runs: https://contiki-ng.github.io/testbed

Also feel free to download our [cheat sheet](https://contiki-ng.github.io/resources/contiki-ng-cheat-sheet.pdf)!

# Citing Contiki-NG in Academic Manuscripts

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


# Documentation

## Setting up Contiki-NG

* [Docker image](https://github.com/contiki-ng/contiki-ng/wiki/Docker)
* [Vagrant image](https://github.com/contiki-ng/contiki-ng/wiki/Vagrant)
* [Native toolchain installation (Linux)](https://github.com/contiki-ng/contiki-ng/wiki/Toolchain-installation-on-Linux)
* [Native toolchain installation (macOS)](https://github.com/contiki-ng/contiki-ng/wiki/Toolchain-installation-on-macOS)
* [IP64 setup with Jool](https://github.com/contiki-ng/contiki-ng/wiki/NAT64-for-Contiki%E2%80%90NG)
* [The Contiki-NG build system](https://github.com/contiki-ng/contiki-ng/wiki/The-Contiki%E2%80%90NG-build-system)
* [The Contiki-NG configuration system](https://github.com/contiki-ng/contiki-ng/wiki/The-Contiki%E2%80%90NG-configuration-system)
* [The Contiki-NG logging system](https://github.com/contiki-ng/contiki-ng/wiki/The-Contiki%E2%80%90NG-logging-system)

## Programming Contiki-NG

* [Repository structure](https://github.com/contiki-ng/contiki-ng/wiki/Repository-structure)
* [Multitasking and scheduling](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Multitasking-and-scheduling)
* [Processes and events](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Processes-and-events)
* [Synchronization primitives](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Synchronization-primitives)
* [Timers](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Timers)
* [Memory management](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Memory-management)
* [Packet buffers](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Packet-buffers)
* [Energy monitoring with Energest](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Energest)
* [UDP communication](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-UDP-communication)
* [IPv6-less networking with NullNet](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-NullNet)
* [A guide on porting Contiki-NG to new platforms](https://github.com/contiki-ng/contiki-ng/wiki/Porting-Contiki%E2%80%90NG-to-new-platforms)
* API documentation (partial): https://contiki-ng.readthedocs.io

## Key networking modules and services

* [IPv6 networking stack](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-IPv6)
* [IPv6 multicast](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-IPv6-multicast)
* [The RPL routing protocol](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-RPL)
* [CoAP and CoAPs](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-CoAP)
* [OMA LWM2M](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-LWM2M)
* [TSCH and 6TiSCH](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-TSCH-and-6TiSCH)
* [6TiSCH 6top sublayer](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-6TiSCH-6top-sub-layer)
* [6TiSCH scheduler Orchestra](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Orchestra)
* [IPv6 over BLE with BLEach](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-IPv6-over-BLE)
* [Communication security](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Communication-Security)
* [SNMP](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-SNMP)

## Storage systems
* [Coffee File System](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Coffee)
* [Antelope database management system](https://github.com/contiki-ng/contiki-ng/wiki/Documentation:-Antelope)

## The Contiki-NG platforms

* [cc2538dk: TI cc2538 development kit](https://github.com/contiki-ng/contiki-ng/wiki/Platform-cc2538dk)
* [cc26x0-cc13x0 / srf06-cc26xx: TI cc26x0 and cc13x0 platforms](https://github.com/contiki-ng/contiki-ng/wiki/Platform-srf06-cc26xx)
* [cooja: Cooja native motes platform](https://github.com/contiki-ng/contiki-ng/wiki/Platform-cooja)
* [jn516x: NXP jn516x series](https://github.com/contiki-ng/contiki-ng/wiki/Platform-jn516x)
* [native: Contiki-NG as a native process](https://github.com/contiki-ng/contiki-ng/wiki/Platform-native)
* [nrf52840: Nordic Semiconductor nRF52840](https://github.com/contiki-ng/contiki-ng/wiki/Platform-nrf52840)
* [nrf: Nordic Semiconductor nRF5340 and nRF52840 (using nRF MDK)](https://github.com/contiki-ng/contiki-ng/wiki/Platform-nrf)
* [nrf52dk: Nordic Semiconductor nRF52 development kit](https://github.com/contiki-ng/contiki-ng/wiki/Platform-nrf52dk)
* [openmote-cc2538: OpenMote cc2538](https://github.com/contiki-ng/contiki-ng/wiki/Platform-openmote-cc2538)
* [simplelink: TI SimpleLink MCU Platform](https://github.com/contiki-ng/contiki-ng/wiki/Platform-simplelink)
* [sky: Tmote Sky / TelosB](https://github.com/contiki-ng/contiki-ng/wiki/Platform-sky)
* [zoul: Zolertia Zoul platforms: Firefly, RE-mote and Orion](https://github.com/contiki-ng/contiki-ng/wiki/Platform-zoul)
# Tutorials

Basics:
* [Hello, World!](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-Hello,-World!)
* [Logging](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-Logging)
* [NG shell](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-Shell)
* [RAM and ROM usage](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-RAM-and-ROM-usage)
* [Simple energy usage estimation](https://github.com/contiki-ng/contiki-ng/wiki/Instrumenting-Contiki-NG-applications-with-energy-usage-estimation)
* [Custom Energest application](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-Energy-monitoring)
* [Timers and events](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-Timers-and-events)

Networking:
* [IPv6 ping](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-IPv6-ping)
* [RPL basics](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-RPL)
* [RPL with border router](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-RPL-border-router)
* [TSCH and 6TiSCH](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-TSCH-and-6TiSCH)
* [Switching from CSMA to TSCH](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-Switching-to-TSCH)
* [CoAP](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-CoAP)
* [LWM2M, IPSO objects, and NAT64](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-LWM2M-and-IPSO-Objects)
* [LWM2M Queue Mode](https://github.com/contiki-ng/contiki-ng/wiki/LWM2M-and-IPSO-Objects-with-Queue-Mode)
* [MQTT](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-MQTT)

Simulation:
* [Cooja: getting started](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-Running-Contiki%E2%80%90NG-in-Cooja)
* [Cooja: simulating a RPL network](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-running-a-RPL-network-in-Cooja)
* [Cooja: simulating a RPL network with a border router](https://github.com/contiki-ng/contiki-ng/wiki/Cooja:-simulating-a-border-router)
* [Running Contiki-NG in Renode](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-Running-Contiki%E2%80%90NG-in-Renode)

# Organization, etc.

* [Organization](https://github.com/contiki-ng/contiki-ng/wiki/Organization)
* [Contributing](https://github.com/contiki-ng/contiki-ng/wiki/Contributing)
* [Code style](https://github.com/contiki-ng/contiki-ng/wiki/Code-style)
* [Development cycle](https://github.com/contiki-ng/contiki-ng/wiki/Development-cycle)
* [Releases](https://github.com/contiki-ng/contiki-ng/releases)
* [Roadmap](https://github.com/contiki-ng/contiki-ng/wiki/Roadmap)
* [Issue and PR labels](https://github.com/contiki-ng/contiki-ng/wiki/Issue-and-Pull-Request-Labels)
* [Where to report issues, ask questions, etc.?](https://github.com/contiki-ng/contiki-ng/wiki/Where-to-report-issues,-ask-questions,-etc.%3F)
* [Logo](https://github.com/contiki-ng/contiki-ng/wiki/Logo)

[doc:more-about-contiki-ng]: https://github.com/contiki-ng/contiki-ng/wiki/More-about-Contiki%E2%80%90NG