# The minimal image should be enough to build contiki for native
# targets
FROM i386/ubuntu:18.04 as minimal

ENV DEBIAN_FRONTEND noninteractive
ARG USER=user
ARG UID=1000
ARG GID=100

# Update path with toolchains
ENV PATH=${PATH}:/opt/bin

# Base Tools
RUN apt-get -qq update && \
    apt-get -qq -y --no-install-recommends install \
        gcc \
        libc6-dev \
        gdb \
        make \
        sudo \
        git \
        curl wget \
        python-pip \
        python-serial \
        mosquitto \
        mosquitto-clients \
        netcat-openbsd \
        rlwrap \
        srecord \
        valgrind \
        snmp \
        snmp-mibs-downloader \
        net-tools iproute2 iputils-ping iputils-tracepath \
        nodejs npm \
        gnupg \
        > /dev/null \
        && \
    # Install coap-cli
    npm -q install coap-cli@0.5.1 -g \
    npm cache clean && \
    apt-get -qq -y autoremove --purge npm && \
    apt-get -qq clean && \
    rm -rf /var/lib/apt/lists/*

# Create user, enable X forwarding, add to group dialout
# -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix
RUN useradd -m -s /bin/bash -N -u $UID $USER && \
    echo "${USER} ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers && \
    chmod 0440 /etc/sudoers && \
    usermod -aG dialout $USER

# Set user for what comes next
USER ${UID}

# Environment variables
ENV HOME                /home/user
ENV USER                ${USER}
ENV CONTIKI_NG          ${HOME}/contiki-ng

WORKDIR                 ${HOME}

# By default, we use a Docker bind mount to share the repo with the host,
# with Docker run option:
# -v <HOST_CONTIKI_NG_ABS_PATH>:/home/user/contiki-ng
# Alternatively, uncomment the next line to download Contiki-NG
#RUN git clone --recursive https://github.com/contiki-ng/contiki-ng.git ${CONTIKI_NG}

# Working directory
WORKDIR ${CONTIKI_NG}

# Start a bash
CMD bash --login


# COOJA build target
# Includes tools to build msp430 targets and run
# simulations with cooja
FROM minimal as cooja
USER root

# Base Tools
RUN apt-get -qq update && \
    apt-get -qq -y --no-install-recommends install \
        openjdk-8-jdk \
        ant \
        bzip2 \
        > /dev/null \
        && \
    apt-get -qq clean && \
    rm -rf /var/lib/apt/lists/*

# Install msp430 toolchain
RUN wget -nv http://simonduq.github.io/resources/mspgcc-4.7.2-compiled.tar.bz2 && \
  tar xjf mspgcc*.tar.bz2 -C /tmp/ && \
  cp -f -r /tmp/msp430/* /usr/local/ && \
  rm -rf /tmp/msp430 mspgcc*.tar.bz2

# Make sure we're using Java 8 for Cooja
RUN update-java-alternatives --set /usr/lib/jvm/java-1.8.0-openjdk-i386

# Set user for what comes next
USER ${USER}

# Environment variables
ENV JAVA_HOME           /usr/lib/jvm/java-8-openjdk-i386
ENV COOJA               ${CONTIKI_NG}/tools/cooja
ENV                     PATH="${HOME}:${PATH}"

WORKDIR ${HOME}

# Create Cooja shortcut
RUN echo "#!/bin/bash\nant -Dbasedir=${COOJA} -f ${COOJA}/build.xml run" > ${HOME}/cooja && \
  chmod +x ${HOME}/cooja

# By default, we use a Docker bind mount to share the repo with the host,
# with Docker run option:
# -v <HOST_CONTIKI_NG_ABS_PATH>:/home/user/contiki-ng
# Alternatively, uncomment the next two lines to download Contiki-NG and pre-compile Cooja.
#RUN git clone --recursive https://github.com/contiki-ng/contiki-ng.git ${CONTIKI_NG}
#RUN ant -q -f ${CONTIKI_NG}/tools/cooja/build.xml jar

# Working directory
WORKDIR ${CONTIKI_NG}

# ARM build target
# includes dependencies to build arm targets
FROM cooja as arm
USER root

# Install ARM toolchain
RUN wget -nv https://launchpad.net/gcc-arm-embedded/5.0/5-2015-q4-major/+download/gcc-arm-none-eabi-5_2-2015q4-20151219-linux.tar.bz2 && \
  tar xjf gcc-arm-none-eabi-5_2-2015q4-20151219-linux.tar.bz2 -C /tmp/ && \
  cp -f -r /tmp/gcc-arm-none-eabi-5_2-2015q4/* /usr/local/ && \
  rm -rf /tmp/gcc-arm-none-eabi-* gcc-arm-none-eabi-*-linux.tar.bz2

# Set user and workdir
USER ${USER}
WORKDIR ${CONTIKI_NG}


# nRF52 build target
# includes dependencies to build arm targets
# as well as the nRF52 toolkit
FROM arm as nrf52
USER root

# Base Tools
RUN apt-get -qq update && \
    apt-get -qq -y --no-install-recommends install \
        unzip \
        > /dev/null \
        && \
    apt-get -qq clean && \
    rm -rf /var/lib/apt/lists/*

## Install nRF52 SDK
RUN wget -nv https://developer.nordicsemi.com/nRF5_IoT_SDK/nRF5_IoT_SDK_v0.9.x/nrf5_iot_sdk_3288530.zip && \
    mkdir /usr/nrf52-sdk && \
    unzip -q nrf5_iot_sdk_3288530.zip -d /usr/nrf52-sdk && \
    rm nrf5_iot_sdk_3288530.zip

ENV NRF52_SDK_ROOT /usr/nrf52-sdk

USER ${USER}
WORKDIR ${CONTIKI_NG}

# NXP build target
# includes dependencies to build arm targets
FROM nrf52 as nxp
USER root

RUN apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF && \
  echo "deb http://download.mono-project.com/repo/ubuntu bionic main" | tee /etc/apt/sources.list.d/mono-xamarin.list && \
    apt-get -qq update && \
    apt-get -qq -y --no-install-recommends install \
        gtk-sharp2 \
        libcanberra-gtk-module:i386 \
        libgtk2.0-0 \
        mono-complete \
        screen \
        uml-utilities \
        smitools \
        > /dev/null \
        && \
    apt-get -qq clean && \
    rm -rf /var/lib/apt/lists/*

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

# Set user for what comes next
USER ${USER}
WORKDIR ${HOME}

# Download, build and install Renode
RUN git clone --quiet https://github.com/renode/renode.git \
  && cd ${HOME}/renode \
  && git checkout v1.3 \
  && ./build.sh
ENV PATH="${HOME}/renode:${PATH}"

WORKDIR ${CONTIKI_NG}

# Documentation build target
# includes doxygen and sphinx packages to build
# documentation
FROM nxp as docs
USER root

# Base Tools
RUN apt-get -qq update && \
    apt-get -qq -y --no-install-recommends install \
        doxygen \
        > /dev/null \
        && \
    apt-get -qq clean && \
    rm -rf /var/lib/apt/lists/*

# Install sphinx and sphinx_rtd_theme, required for building and testing the
# readthedocs API documentation
RUN pip -q install --upgrade pip
RUN pip -q install setuptools && pip -q install sphinx_rtd_theme sphinx

USER ${USER}
