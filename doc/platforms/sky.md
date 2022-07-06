# sky: Tmote Sky / TelosB

This platform is Contiki's historical default platform. It is still in Contiki-NG, mostly for emulation purposes with MSPSim in Cooja.

Key features:
* 250kbps 2.4GHz IEEE 802.15.4 Chipcon Wireless Transceiver
* 8MHz Texas Instruments MSP430 microcontroller (10k RAM, 48k Flash)
* Integrated ADC, DAC, Supply Voltage Supervisor, and DMA Controller
* Integrated onboard antenna with 50m range indoors / 125m range outdoors
* Integrated Humidity, Temperature, and Light sensors
* Hardware link-layer encryption and authentication
* USB interface

The platform datasheet is available at: http://www.eecs.harvard.edu/~konrad/projects/shimmer/references/tmote-sky-datasheet.pdf

Due to RAM and ROM limitations, the Sky can only run selected subsets of the stack, e.g. RPL+TSCH does not fit with downward routing, and very few logs can be enabled in general.
