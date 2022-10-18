FROM ubuntu:22.04

ENV DEBIAN_FRONTEND noninteractive

# Set user for what comes next
USER root

# Tools
# build-essential: development tools.
# gdb: development tools.
# git: development tools.
# iputils-ping: used by regression tests.
# less: convenience tool.
# lib32z1: 32-bit libz, probably used by some old 32-bit binary.
# libcanberra-gtk-module: remove warning message from renode.
# libcoap2-bin: used by regression tests.
# libxtst6: required for GraalVM/Java17 to run Cooja in GUI mode.
# mosquitto: used by the regression tests.
# mtr-tiny: used by the regression tests.
# net-tools: used by the regression tests.
# python3: used by scripts.
# snmp: used by the regression tests.
# sudo: used by the regression tests.
# udev: needed for jlink deb postinst script.
# unzip: required during docker image build for software installation.
# valgrind: used by the regression tests.
# wget: used by the regression tests.
RUN apt-get -qq update && \
    apt-get -qq -y --no-install-recommends install \
      ca-certificates > /dev/null && \
  apt-get -qq -y --no-install-recommends install \
    build-essential \
    gdb \
    git \
    iputils-ping \
    less \
    lib32z1 \
    libcanberra-gtk-module \
    libcoap2-bin \
    libpng-dev \
    libxtst6 \
    mosquitto \
    mosquitto-clients \
    mtr-tiny \
    net-tools \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-serial \
    rlwrap \
    sudo \
    screen \
    srecord \
    udev \
    unzip \
    libusb-1.0-0 \
    valgrind \
    wget \
    smitools \
    snmp \
    snmp-mibs-downloader \
    > /dev/null && \
  wget -nv https://github.com/dongjinleekr/graalvm-ce-deb/releases/download/22.2.0-0/graalvm-ce-java17_amd64_22.2.0-0.deb && \
  dpkg --unpack graalvm-ce-java17_amd64_22.2.0-0.deb && \
  apt-get -qq -y --no-install-recommends install \
    ca-certificates-java \
    java-common \
    libnss3 \
    libnspr4 \
    libsqlite3-0 \
    > /dev/null && \
  apt-get -qq -y --no-install-recommends install \
    ant \
    > /dev/null && \
  rm graalvm-ce-java17_amd64_22.2.0-0.deb && \
  apt-get -qq clean

# Install ARM toolchain
RUN wget -nv https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 && \
  tar xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /usr/local --strip-components=1 --no-same-owner && \
  rm -f gcc-arm-none-eabi-*-linux.tar.bz2

# Install msp430 toolchain
RUN wget -nv http://simonduq.github.io/resources/mspgcc-4.7.2-compiled.tar.bz2 && \
  tar xjf mspgcc*.tar.bz2 -C /tmp/ && \
  cp -f -r /tmp/msp430/* /usr/local/ && \
  rm -rf /tmp/msp430 mspgcc*.tar.bz2

## Install nRF52 SDK
RUN wget -nv https://developer.nordicsemi.com/nRF5_IoT_SDK/nRF5_IoT_SDK_v0.9.x/nrf5_iot_sdk_3288530.zip && \
    mkdir /usr/nrf52-sdk && \
    unzip -q nrf5_iot_sdk_3288530.zip -d /usr/nrf52-sdk && \
    rm nrf5_iot_sdk_3288530.zip

ENV NRF52_SDK_ROOT /usr/nrf52-sdk

# Install nRF Command Line tools
RUN wget -nv https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/10-17-3/nrf-command-line-tools_10.17.3_amd64.deb && \
  wget -nv https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/10-17-3/nrf-command-line-tools-10.17.3_Linux-amd64.tar.gz && \
  apt-get -qq -y --no-install-recommends install ./nrf-command-line-tools_10.17.3_amd64.deb > /dev/null && \
  tar zxf nrf-command-line-tools-10.17.3_Linux-amd64.tar.gz ./JLink_Linux_V766a_x86_64.deb && \
  apt-get -qq -y --no-install-recommends install ./JLink_Linux_V766a_x86_64.deb > /dev/null && \
  rm -f *.deb *.tar.gz && \
  apt-get -qq clean

# Install Renode from github releases
ARG RENODE_VERSION=1.13.0
RUN wget -nv https://github.com/renode/renode/releases/download/v${RENODE_VERSION}/renode_${RENODE_VERSION}_amd64.deb \
  && apt-get -qq -y --no-install-recommends install ./renode_${RENODE_VERSION}_amd64.deb > /dev/null \
  && rm renode_${RENODE_VERSION}_amd64.deb \
  && apt-get -qq clean

# Sphinx is required for building the readthedocs API documentation.
# RTD requirements are shared with .readthedocs.yaml for build consistency - check RTD build if modifying.
# Matplotlib is required for result visualization.
# Keep the image size down by removing the pip cache when done.
COPY files/rtd_requirements.txt /tmp
RUN python3 -m pip -q install \
      matplotlib \
      nrfutil && \
    python3 -m pip -q install -r /tmp/rtd_requirements.txt && \
    rm -rf /root/.cache /tmp/rtd_requirements.txt

# Create user, add to groups dialout and sudo, and configure sudoers.
RUN adduser --disabled-password --gecos '' user && \
    usermod -aG dialout,plugdev,sudo user && \
    echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Set user for what comes next
USER user

# Environment variables
ENV HOME                /home/user
ENV CONTIKI_NG          ${HOME}/contiki-ng
ENV COOJA               ${CONTIKI_NG}/tools/cooja
ENV                     PATH="${HOME}/.local/bin:${PATH}"
ENV                     LC_ALL=C.UTF-8
ENV                     LANG=C.UTF-8
WORKDIR                 ${HOME}

# Create Cooja shortcut
COPY --chown=user:user files/cooja ${HOME}/.local/bin/cooja
# Add aliases for CI. Use a file to avoid quoting issues.
COPY --chown=user:user files/bash_aliases ${HOME}/.bash_aliases

# Doxygen 1.8.17 in Ubuntu 20.04 gives (false) warnings on mqtt.h.
# Use a binary from the Doxygen homepage, static linking started with 1.9.3.
# Remove the PDF manual and html directory to reduce image size.
# Use the most recent version of ccache to ensure it supports the compiler
# versions in the docker image.
RUN wget -nv https://www.doxygen.nl/files/doxygen-1.9.4.linux.bin.tar.gz && \
    tar zxf doxygen-1.9.4.linux.bin.tar.gz -C ${HOME}/.local && \
    rm -rf ${HOME}/.local/doxygen-1.9.4/html ${HOME}/.local/doxygen-1.9.4/*.pdf doxygen-1.9.4.linux.bin.tar.gz && \
    (cd ${HOME}/.local/bin && ln -s ../doxygen-1.9.4/bin/doxygen .) && \
    wget -nv https://github.com/ccache/ccache/releases/download/v4.6.2/ccache-4.6.2-linux-x86_64.tar.xz && \
    tar xf ccache-4.6.2-linux-x86_64.tar.xz -C ${HOME}/.local/bin --strip-components=1 ccache-4.6.2-linux-x86_64/ccache && \
    rm ccache-*-linux-x86_64.tar.xz

# By default, we use a Docker bind mount to share the repo with the host,
# with Docker run option:
# -v <HOST_CONTIKI_NG_ABS_PATH>:/home/user/contiki-ng
# Alternatively, uncomment the next two lines to download Contiki-NG and pre-compile Cooja.
#RUN git clone --recursive https://github.com/contiki-ng/contiki-ng.git ${CONTIKI_NG}
#RUN ant -q -f ${CONTIKI_NG}/tools/cooja/build.xml jar

# Working directory
WORKDIR ${CONTIKI_NG}

# Start a bash
CMD bash --login
