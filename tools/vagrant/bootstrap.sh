#!/usr/bin/env bash

# Use nearby mirrors.
sudo sed -i -e 's#http://archive.ubuntu.com/ubuntu#mirror://mirrors.ubuntu.com/mirrors.txt#' /etc/apt/sources.list

# Install i386 binary support on x64 system and required tools
sudo dpkg --add-architecture i386
sudo add-apt-repository --yes ppa:mosquitto-dev/mosquitto-ppa
sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 77B7346A59027B33C10CAFE35E64E954262C4500
sudo apt update
sudo apt install -y --no-install-recommends \
       libc6:i386 \
       libstdc++6:i386 \
       libncurses5:i386 \
       libz1:i386 \
       ant \
       build-essential \
       ccache \
       doxygen \
       gdb \
       git \
       iputils-tracepath \
       libcoap-1-0-bin \
       linux-image-extra-virtual \
       mosquitto \
       mosquitto-clients \
       npm \
       openjdk-11-jdk \
       python3-magic \
       python3-pip \
       python3-serial \
       rlwrap \
       smitools \
       snmp \
       snmp-mibs-downloader \
       srecord \
       sudo \
       uncrustify \
       unzip \
       valgrind \
       wget

sudo apt-get clean

# Sphinx is required for building the readthedocs API documentation.
# Matplotlib is required for result visualization.
# After that, install nrfutil, work around broken pc_ble_driver_py dependency,
# and remove the pip cache.
sudo -H python3 -m pip -q install --upgrade pip && \
sudo -H python3 -m pip -q install \
      setuptools \
      sphinx_rtd_theme \
      sphinx \
      matplotlib && \
sudo rm -rf /root/.cache

# Install ARM toolchain
wget -nv https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 && \
sudo tar xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /usr/local --strip-components=1 --no-same-owner && \
rm -f gcc-arm-none-eabi-*-linux.tar.bz2

# Install msp430 toolchain
wget http://simonduq.github.io/resources/mspgcc-4.7.2-compiled.tar.bz2
tar xjf mspgcc*.tar.bz2 -C /tmp/
sudo cp -f -r /tmp/msp430/* /usr/local/
rm -rf /tmp/msp430 mspgcc*.tar.bz2

# Install NXP toolchain (partial, with binaries excluded. Download from nxp.com)
wget http://simonduq.github.io/resources/ba-elf-gcc-4.7.4-part1.tar.bz2
wget http://simonduq.github.io/resources/ba-elf-gcc-4.7.4-part2.tar.bz2
wget http://simonduq.github.io/resources/jn516x-sdk-4163-1416.tar.bz2
mkdir -p /tmp/jn516x-sdk /tmp/ba-elf-gcc
tar xjf jn516x-sdk-*.tar.bz2 -C /tmp/jn516x-sdk
tar xjf ba-elf-gcc-*part1.tar.bz2 -C /tmp/ba-elf-gcc
tar xjf ba-elf-gcc-*part2.tar.bz2 -C /tmp/ba-elf-gcc
sudo cp -f -r /tmp/jn516x-sdk /usr/
sudo cp -f -r /tmp/ba-elf-gcc /usr/
rm -rf jn516x*.bz2 ba-elf-gcc*.bz2 /tmp/ba-elf-gcc* /tmp/jn516x-sdk*

echo 'export PATH="/usr/ba-elf-gcc/bin:${PATH}"' >> ${HOME}/.bashrc

## Install nRF52 SDK
wget https://developer.nordicsemi.com/nRF5_IoT_SDK/nRF5_IoT_SDK_v0.9.x/nrf5_iot_sdk_3288530.zip
sudo mkdir -p /usr/nrf52-sdk
sudo unzip nrf5_iot_sdk_3288530.zip -d /usr/nrf52-sdk
rm nrf5_iot_sdk_3288530.zip

echo "export NRF52_SDK_ROOT=/usr/nrf52-sdk" >> ${HOME}/.bashrc

sudo usermod -aG dialout,plugdev,sudo vagrant

# Environment variables
echo "export CONTIKI_NG=${HOME}/contiki-ng" >> ${HOME}/.bashrc
echo "export COOJA=${CONTIKI_NG}/tools/cooja" >> ${HOME}/.bashrc
echo "export PATH=${HOME}/.local/bin:${PATH}" >> ${HOME}/.bashrc
echo "export WORKDIR=${HOME}" >> ${HOME}/.bashrc
source ${HOME}/.bashrc

# Create Cooja shortcut
mkdir -p ${HOME}/.local/bin
cp -a ${HOME}/contiki-ng/tools/docker/files/cooja ${HOME}/.local/bin

# Docker
curl -fsSL get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker vagrant

# Docker image "Contiker" alias
echo 'alias contiker="docker run --privileged --mount type=bind,source=$CONTIKI_NG,destination=/home/user/contiki-ng -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v /dev/bus/usb:/dev/bus/usb -ti contiker/contiki-ng"' >> /home/vagrant/.bashrc
source ${HOME}/.bashrc
