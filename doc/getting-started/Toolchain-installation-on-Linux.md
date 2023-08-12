# Toolchain installation on Linux

We recommend using the Contiki-NG Docker image for easy setup of a consistent development environment: [doc:docker].

Alternatively, you can install the toolchains natively on your system.
This page describes how to do so, for a Linux development environment.

## Install development tools for Contiki-NG

Start by installing some necessary packages:
```bash
$ sudo apt update
$ sudo apt install build-essential doxygen git git-lfs curl wireshark python3-serial srecord rlwrap
```

On recent Ubuntu releases, you may get an error `ifconfig: not found` and `netstat: not found` when trying to run `tunslip6`. If this is the case, use apt to install `net-tools`.

While installing Wireshark, select enable the feature that lets non-superuser capture packets (select "yes").
Now add yourself to the `wireshark` group:
```bash
$ sudo usermod -a -G wireshark <user>
```

Then you will - if you intend to develop or re-configure examples - also need to install
an code/text editor. There are many alternatives so we leave that up to you!


### Install ARM compiler

An ARM compiler is needed to compile for ARM-based platforms such as CC2538DK and Zoul.

You should download it from https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads or you can grab an older version from launchpad.net.

```bash
$ wget https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2
$ tar -xjf gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2
```

This will create a directory named `gcc-arm-none-eabi-9-2020-q2-update` in your current working dir. Add `<working-directory>/gcc-arm-none-eabi-9-2020-q2-update/bin` to your path.


### Install MSP430 compiler

A compiler for MSP430 is needed to use the MSPSim emulator in Cooja. It is possible to install a GCC compiler for MSP430 from the Ubuntu package repository (`sudo apt install gcc-msp430`) but that version is too old to support the full memory of newer versions of MSP430. 

The preferred version of MSP430 gcc is 4.7.2. To install it on `/usr/local` if you're using 64-bit Linux, run:

```
$ wget -nv http://simonduq.github.io/resources/mspgcc-4.7.2-compiled.tar.bz2 && \
  tar xjf mspgcc*.tar.bz2 -C /tmp/ && \
  cp -f -r /tmp/msp430/* /usr/local/ && \
  rm -rf /tmp/msp430 mspgcc*.tar.bz2
```

If desired, a script to compile MSP430 GCC 4.7.2 from source can be found [here](https://github.com/contiki-ng/contiki-ng/blob/develop/tools/toolchain/msp430/buildmsp.sh).


### Install Java for the Cooja network simulator

```bash
$ sudo apt install default-jdk ant
$ update-alternatives --config java
There is only one alternative in link group java (providing /usr/bin/java): /usr/lib/jvm/java-8-openjdk-amd64/jre/bin/java
Nothing to configure.
```

### Install JTAG software for CC2538 (optional)

Many platforms such as Zolertia Zoul can be programmed via builtin USB but for programming a device using a JTAG dongle, Segger J-Link has been verified to work well with CC2538. The software for Segger J-Link can be downloaded from [Segger](https://www.segger.com/jlink-software.html). Download the installation file for Debian Linux.

```bash
$ sudo dpkg --install jlink_4.98.5_i386.deb
```

### Install a CoAP client (optional)

Optionally and if you want to use CoAP examples, you can install the CoAP client `coap-cli`:
```bash
$ sudo apt-get install -y npm \
  && sudo apt-get clean \
  && sudo npm install coap-cli -g \
  && sudo ln -s /usr/bin/nodejs /usr/bin/node
```
### Install the Mosquitto MQTT broker and accompanying clients (optional)

Install Mosquitto, if you need MQTT:
```bash
$ sudo apt-get install -y mosquitto mosquitto-clients
```

## User access to USB
To be able to access the USB without using sudo, the user should be part of the groups `plugdev` and `dialout`.

```bash
$ sudo usermod -a -G plugdev <user>
$ sudo usermod -a -G dialout <user>
```

### Improve stability for CC2538

The modem manager might interfere with the USB for CC2538.
Update the configuration with:
```bash
echo 'ATTRS{idVendor}=="0451", ATTRS{idProduct}=="16c8", ENV{ID_MM_DEVICE_IGNORE}="1"' >> /lib/udev/rules.d/77-mm-usb-device-blacklist.rules
```

### After configuration
Reboot the system after all configurations have been made.

## Clone Contiki-NG

```bash
$ git clone https://github.com/contiki-ng/contiki-ng.git
$ cd contiki-ng
$ git submodule update --init --recursive
```

*Note*: we recommend cloning and then initializing the submodules rather than using `git clone --recursive`.
The latter results in submodules that use absolute paths to the top-level git, rather than relative paths, which are more flexible for a number of reasons.

[doc:docker]: /doc/getting-started/Docker
