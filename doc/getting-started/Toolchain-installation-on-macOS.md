# Toolchain installation on macOS

We recommend using the Contiki-NG Docker image for easy setup of a consistent development environment: [doc:docker].

Alternatively, you can install the toolchains natively on your system.
This page describes how to do so, for OS X.

## Install development tools for Contiki-NG
Start by installing Xcode Command Line Utilities
```bash
$ xcode-select --install
```

This guide makes extensive usage of [homebrew](https://brew.sh/). If you don't already have it, install it:

```bash
$ /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

Using homebrew, install a bunch of helper tools, such as git, srecord, doxygen (to build the API documentation), mosquitto (to test Contiki-NG's MQTT functionality), tuntap (for tunslip and to run native examples with networking), rlwrap (for shell history), python (python 3 for running scripts and pip).

```bash
$ brew install git git-lfs srecord doxygen uncrustify ant mosquitto wget libmagic rlwrap python make gawk
$ brew tap caskroom/cask
$ brew cask install tuntap
```

You may also need to update your PATH environment variable to use GNU Make 4 when running the "make" command. This can be set in your shell initialization file (e.g., ~/.zshrc) as follows.

```bash
$ echo 'PATH="/opt/homebrew/opt/make/libexec/gnubin:$PATH"' >> ~/.zshrc
```

Note that if Homebrew is installed in a different path, you have to modify the command accordingly.

### Install some python packages
Those are used/needed by some Contiki-NG Python scripts.

```bash
$ pip install intelhex pyserial python-magic
```

### Install the ARM GCC toolchain

An ARM compiler is needed to compile for ARM-based platforms.

#### Using homebrew

You can install the arm-gcc toolchain using homebrew.

```bash
# to tap the repository
$ brew tap contiki-ng/contiki-ng-arm
# to install the toolchain
$ brew install contiki-ng-arm-gcc-bin
```

This will automatically install the same version as the one currently installed in the [docker image](/doc/getting-started/Docker) and used by the CI tests.

The formula itself is hosted under [contiki-ng/homebrew-contiki-ng-arm](https://github.com/contiki-ng/homebrew-contiki-ng-arm)

#### Manual Installation

You should download the toolchain from https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads or you can grab an older version from launchpad.net.

```bash
$ wget https://launchpad.net/gcc-arm-embedded/5.0/5-2015-q4-major/+download/gcc-arm-none-eabi-5_2-2015q4-20151219-mac.tar.bz2
$ tar jxf gcc-arm-none-eabi-5_2-2015q4-20151219-mac.tar.bz2
```

This will create a directory named `gcc-arm-none-eabi-5_2-2015q4` in your current working dir. Add `<working-directory>/gcc-arm-none-eabi-5_2-2015q4/bin` to your path.

You can also try using the latest version, but be prepared to get compilation warnings and/or errors.

### Install the MSP430 toolchain

The best way to achieve this on OS X is through homebrew, using a formula provided in a tap. Follow the instructions here: https://github.com/tgtakaoka/homebrew-mspgcc

### Install Java JDK for the Cooja network simulator
Nothing exciting here, just download and install Java for OSX. You will need the JDK 17, not just the runtime.

### Install a CoAP client (libcoap)
Optionally and if you want to use CoAP examples, you can install the CoAP client distributed with [libcoap].

Firstly you will need the following packages:

* automake
* autoconf
* libtool
* pkg-config

They can be installed with homebrew:
```
$ brew install automake pkg-config libtool autoconf
```

Then clone libcoap, compile and build it:
```
$ git clone --recursive https://github.com/obgm/libcoap.git
$ cd libcoap/
$ ./autogen.sh 
$ ./configure --disable-doxygen --disable-documentation --disable-shared
$ make
$ make install
```
This will install `coap-client` under `/usr/local/bin`

```
$ coap-client 
coap-client v4.2.0alpha -- a small CoAP implementation
(c) 2010-2015 Olaf Bergmann <bergmann@tzi.org>

usage: coap-client [-A type...] [-t type] [-b [num,]size] [-B seconds] [-e text]
		[-m method] [-N] [-o file] [-P addr[:port]] [-p port]
		[-s duration] [-O num,text] [-T string] [-v num] [-a addr] [-U]

		[-u user] [-k key] [-r] URI
[...]
```
## Clone Contiki-NG

```bash
$ git clone https://github.com/contiki-ng/contiki-ng.git
$ cd contiki-ng
$ git submodule update --init --recursive
```

*Note*: we recommend cloning and then initializing the submodules rather than using `git clone --recursive`.
The latter results in submodules that use absolute paths to the top-level git, rather than relative paths, which are more flexible for a number of reasons.

[doc:docker]: /doc/getting-started/Docker
[libcoap]: https://libcoap.net/
