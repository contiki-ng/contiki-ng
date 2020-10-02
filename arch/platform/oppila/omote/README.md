Oppila Microsystems omote platform -  http://www.oppila.in
-------------------------------------------------------------------------

The omote platform is a IoT Hardware development platform based
on TI's CC2538 system on chip (SoC), featuring an ARM Cortex-M3 with 512KB
flash, 32Kb RAM, 2.4GHz RF interface , and the
following goodies:

* ISM 2.4-GHz IEEE 802.15.4 & Zigbee compliant.
* AES-128/256, SHA2 Hardware Encryption Engine.
* ECC-128/256, RSA Hardware Acceleration Engine for Secure Key Exchange.
* On board sensors LDR, ADXL345 and BMP180
* Power input with wide range 3.7-30VDC.
* Programming over BSL by enetring into bootloader mode
* On-board micro USB connector for USB 2.0 applications.
* Two LEDs
* User, BSL and Reset buttons.
* On-board printed PCB 2.4Ghz antenna. 

Port Features

The omote has the following key features:

  * Deep Sleep support with RAM retention for ultra-low energy consumption.
  * Native USB support (CDC-ACM). SLIP over UART for border routers is no longer a bottleneck.
  * DMA transfers for increased performance (RAM to/from RF, RAM to/from USB).

In terms of hardware support, the following drivers have been implemented for the oppila-based platforms:

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

The omote has 13 pinouts to connect boolean,digital and analog sensors based on I2C,UART,and SPI Protocols as well as other sensors or actuators you may need to connect.

More Reading
============
1. [Oppila Microsystems omote website](http://www.oppila.in)
2. [CC2538 System-on-Chip Solution for 2.4-GHz IEEE 802.15.4 and ZigBee applications (SWRU319B)][cc2538]

[cc2538]: http://www.ti.com/product/cc2538     "CC2538"
*/
