FROM ubuntu:20.04

ENV DEBIAN_FRONTEND noninteractive

# Set user for what comes next
USER root

# Tools
RUN apt-get -qq update && \
    apt-get -qq -y --no-install-recommends install \
      ca-certificates \
      gnupg \
      software-properties-common > /dev/null && \
    apt-get -qq clean

RUN apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF && \
  echo "deb http://download.mono-project.com/repo/ubuntu stable-focal main" | tee /etc/apt/sources.list.d/mono-official-stable.list && \
  add-apt-repository ppa:mosquitto-dev/mosquitto-ppa && \
  apt-get -qq update && \
  apt-get -qq -y --no-install-recommends install \
    ant \
    build-essential \
    gdb \
    git \
    gtk-sharp2 \
    iputils-tracepath \
    iputils-ping \
    less \
    lib32z1 \
    libcanberra-gtk-module \
    libcoap2-bin \
    libfreetype6-dev \
    libgtk2.0-0 \
    libncurses5 \
    libpng-dev \
    mono-complete \
    mosquitto \
    mosquitto-clients \
    net-tools \
    openjdk-11-jdk \
    python3-dev \
    python3-pip \
    python3-setuptools \
    python3-serial \
    rlwrap \
    sudo \
    screen \
    srecord \
    uml-utilities \
    unzip \
    libusb-1.0-0 \
    valgrind \
    wget \
    smitools \
    snmp \
    snmp-mibs-downloader \
    > /dev/null \
  && apt-get -qq clean

# Install ARM toolchain
RUN wget -nv https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 && \
  tar xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /usr/local --strip-components=1 --no-same-owner && \
  rm -f gcc-arm-none-eabi-*-linux.tar.bz2

# Install msp430 toolchain
RUN wget -nv http://simonduq.github.io/resources/mspgcc-4.7.2-compiled.tar.bz2 && \
  tar xjf mspgcc*.tar.bz2 -C /tmp/ && \
  cp -f -r /tmp/msp430/* /usr/local/ && \
  rm -rf /tmp/msp430 mspgcc*.tar.bz2

# Install NXP toolchain (partial, with binaries excluded. Download from nxp.com)
RUN wget -nv http://simonduq.github.io/resources/ba-elf-gcc-4.7.4-part1.tar.bz2 && \
  wget -nv http://simonduq.github.io/resources/ba-elf-gcc-4.7.4-part2.tar.bz2 && \
  wget -nv http://simonduq.github.io/resources/jn516x-sdk-4163-1416.tar.bz2 && \
  mkdir /tmp/jn516x-sdk /tmp/ba-elf-gcc && \
  tar xjf jn516x-sdk-*.tar.bz2 -C /tmp/jn516x-sdk && \
  tar xjf ba-elf-gcc-*part1.tar.bz2 -C /tmp/ba-elf-gcc && \
  tar xjf ba-elf-gcc-*part2.tar.bz2 -C /tmp/ba-elf-gcc && \
  cp -f -r /tmp/jn516x-sdk /usr/ && \
  cp -f -r /tmp/ba-elf-gcc /usr/ && \
  rm -rf jn516x*.bz2 ba-elf-gcc*.bz2 /tmp/ba-elf-gcc* /tmp/jn516x-sdk*

ENV PATH="/usr/ba-elf-gcc/bin:${PATH}"

## Install nRF52 SDK
RUN wget -nv https://developer.nordicsemi.com/nRF5_IoT_SDK/nRF5_IoT_SDK_v0.9.x/nrf5_iot_sdk_3288530.zip && \
    mkdir /usr/nrf52-sdk && \
    unzip -q nrf5_iot_sdk_3288530.zip -d /usr/nrf52-sdk && \
    rm nrf5_iot_sdk_3288530.zip

ENV NRF52_SDK_ROOT /usr/nrf52-sdk

# Install nRF Command Line tools
RUN wget -nv https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/10-12-1/nRFCommandLineTools10121Linuxamd64.tar.gz && \
  mkdir /tmp/nrf-cli-tools && \
  tar xzf nRFCommandLineTools10121Linuxamd64.tar.gz -C /tmp/nrf-cli-tools && \
  dpkg -i /tmp/nrf-cli-tools/JLink_Linux_V688a_x86_64.deb && \
  dpkg -i /tmp/nrf-cli-tools/nRF-Command-Line-Tools_10_12_1_Linux-amd64.deb && \
  rm -rf nRFCommandLineTools10121Linuxamd64.tar.gz /tmp/nrf-cli-tools

# Sphinx is required for building the readthedocs API documentation.
# RTD requirements are shared with .readthedocs.yaml for build consistency - check RTD build if modifying.
# Matplotlib is required for result visualization.
# Nrfutil 6.1.3 does not work with protobuf 4, so install latest 3.x
# Keep the image size down by removing the pip cache when done.
COPY files/rtd_requirements.txt /tmp
RUN python3 -m pip -q install --upgrade pip && \
    python3 -m pip -q install \
      setuptools \
      matplotlib \
      'protobuf<=4' && \
    python3 -m pip -q install \
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

# Doxygen 1.8.17 in Ubuntu 20.04 gives (false) warnings on mqtt.h.
# Use a binary from the Doxygen homepage, static linking started with 1.9.3.
# Remove the PDF manual and html directory to reduce image size.
# Use the most recent version of ccache to ensure it supports the compiler
# versions in the docker image.
RUN wget -nv https://www.doxygen.nl/files/doxygen-1.9.4.linux.bin.tar.gz && \
    tar zxf doxygen-1.9.4.linux.bin.tar.gz -C ${HOME}/.local && \
    rm -rf ${HOME}/.local/doxygen-1.9.4/html ${HOME}/.local/doxygen-1.9.4/*.pdf doxygen-1.9.4.linux.bin.tar.gz && \
    (cd ${HOME}/.local/bin && ln -s ../doxygen-1.9.4/bin/doxygen .) && \
    wget -nv https://github.com/ccache/ccache/releases/download/v4.6.1/ccache-4.6.1-linux-x86_64.tar.xz && \
    tar xf ccache-4.6.1-linux-x86_64.tar.xz -C ${HOME}/.local/bin --strip-components=1 ccache-4.6.1-linux-x86_64/ccache && \
    rm ccache-4.6.1-linux-x86_64.tar.xz

# Download, build and install Renode
RUN git clone --quiet https://github.com/renode/renode.git \
  && cd ${HOME}/renode \
  && git checkout v1.13.0 \
  && ./build.sh
ENV PATH="${HOME}/renode:${PATH}"

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
