FROM 32bit/ubuntu:16.04

# Tools
RUN apt-get update && \
  apt-get install -y --no-install-recommends \
  build-essential doxygen git wget unzip python-serial python-pip \
  default-jdk ant srecord iputils-tracepath rlwrap \
  mosquitto mosquitto-clients gdb \
  && apt-get clean

# Install ARM toolchain
RUN wget https://launchpad.net/gcc-arm-embedded/5.0/5-2015-q4-major/+download/gcc-arm-none-eabi-5_2-2015q4-20151219-linux.tar.bz2 && \
  tar xjf gcc-arm-none-eabi-5_2-2015q4-20151219-linux.tar.bz2 -C /tmp/ && \
  cp -f -r /tmp/gcc-arm-none-eabi-5_2-2015q4/* /usr/local/ && \
  rm -rf /tmp/gcc-arm-none-eabi-* gcc-arm-none-eabi-*-linux.tar.bz2

# Install msp430 toolchain
RUN wget http://simonduq.github.io/resources/mspgcc-4.7.2-compiled.tar.bz2 && \
  tar xjf mspgcc*.tar.bz2 -C /tmp/ && \
  cp -f -r /tmp/msp430/* /usr/local/ && \
  rm -rf /tmp/msp430 mspgcc*.tar.bz2

# Install NXP toolchain (partial, with binaries excluded. Download from nxp.com)
RUN wget http://simonduq.github.io/resources/ba-elf-gcc-4.7.4-part1.tar.bz2 && \
  wget http://simonduq.github.io/resources/ba-elf-gcc-4.7.4-part2.tar.bz2 && \
  wget http://simonduq.github.io/resources/jn516x-sdk-4163-1416.tar.bz2 && \
  mkdir /tmp/jn516x-sdk /tmp/ba-elf-gcc && \
  tar xjf jn516x-sdk-*.tar.bz2 -C /tmp/jn516x-sdk && \
  tar xjf ba-elf-gcc-*part1.tar.bz2 -C /tmp/ba-elf-gcc && \
  tar xjf ba-elf-gcc-*part2.tar.bz2 -C /tmp/ba-elf-gcc && \
  cp -f -r /tmp/jn516x-sdk /usr/ && \
  cp -f -r /tmp/ba-elf-gcc /usr/ && \
  rm -rf jn516x*.bz2 ba-elf-gcc*.bz2 /tmp/ba-elf-gcc* /tmp/jn516x-sdk*

ENV PATH="/usr/ba-elf-gcc/bin:${PATH}"

## Install nRF52 SDK
RUN wget https://developer.nordicsemi.com/nRF5_IoT_SDK/nRF5_IoT_SDK_v0.9.x/nrf5_iot_sdk_3288530.zip && \
    mkdir /usr/nrf52-sdk && \
    unzip nrf5_iot_sdk_3288530.zip -d /usr/nrf52-sdk && \
    rm nrf5_iot_sdk_3288530.zip

ENV NRF52_SDK_ROOT /usr/nrf52-sdk

# Install sphinx and sphinx_rtd_theme, required for building and testing the
# readthedocs API documentation
RUN pip install --upgrade pip
RUN pip install setuptools
RUN pip install sphinx_rtd_theme sphinx

# Create user, enable X forwarding, add to group dialout
# -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix
RUN export uid=1000 gid=1000 && \
    mkdir -p /home/user && \
    echo "user:x:${uid}:${gid}:user,,,:/home/user:/bin/bash" >> /etc/passwd && \
    echo "user:x:${uid}:" >> /etc/group && \
    echo "user ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers && \
    chmod 0440 /etc/sudoers && \
    chown ${uid}:${gid} -R /home/user && \
    usermod -aG dialout user

# Set user for what comes next
USER user

# Environment variables
ENV JAVA_HOME           /usr/lib/jvm/default-java
ENV HOME                /home/user
ENV CONTIKI_NG          ${HOME}/contiki-ng
ENV COOJA               ${CONTIKI_NG}/tools/cooja
ENV                     PATH="${HOME}:${PATH}"
WORKDIR                 ${HOME}

# Create Cooja shortcut
RUN echo "#!/bin/bash\nant -Dbasedir=${COOJA} -f ${COOJA}/build.xml run" > ${HOME}/cooja && \
  chmod +x ${HOME}/cooja

# Install coap-cli
RUN sudo apt-get install -y npm \
  && sudo apt-get clean \
  && sudo npm install coap-cli -g \
  && sudo ln -s /usr/bin/nodejs /usr/bin/node

# Install Mono and libcanberra-gtk:i386 (for Renode)
RUN sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF \
  && echo "deb http://download.mono-project.com/repo/ubuntu xenial main" | sudo tee /etc/apt/sources.list.d/mono-xamarin.list \
  && sudo apt-get update \
  && sudo apt-get install -y mono-complete gksu libgtk2.0-0 screen uml-utilities gtk-sharp2 libcanberra-gtk-module:i386 \
  && sudo apt-get clean

# Download, build and install Renode
RUN git clone https://github.com/renode/renode.git \
  && cd ${HOME}/renode \
  && git checkout v1.3 \
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

# Enable IPv6 -- must be done at runtime, hence added to .profile
RUN echo "sudo sysctl -w net.ipv6.conf.all.disable_ipv6=0 > /dev/null" >> ${HOME}/.profile

# Start a bash
CMD bash --login
