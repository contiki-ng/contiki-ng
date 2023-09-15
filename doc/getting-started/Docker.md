# Docker

We provide multiple Docker images for Contiki-NG hosted on DockerHub, as [`contiker/contiki-ng`](https://hub.docker.com/r/contiker/contiki-ng).

The `Dockerfile` can be found in the Contiki-NG repository under `tools/docker`.
As all Continuous Integration tests are run in a Docker container, it is easy to reproduce the testing environment locally via Docker.

## Setup

To get started, install Docker. On Ubuntu for instance (you'll need set up the repository; see [install-docker-ce] for details):
```shell
sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```
NOTE: other docker versions than the one suggested above might now work well with the instructions below. Default docker runtimes in Ubuntu are typically not going to work well.

Make sure your user is added to the unix group `docker`:
```shell
sudo usermod -aG docker <your-user>
```

Log out, and log in again.

Download the Contiki-NG image:
```shell
docker pull contiker/contiki-ng
```

This will automatically download `contiker/contiki-ng:latest`, which is the image used in CI and which we recommend for development. If you wish to use a different image version please follow the guidelines in the start of the article.
The image is meant for use with Contiki-NG as a bind mount, which means you make the Contiki-NG repository on the host accessible from inside the container.
This way, you can work on the codebase using host tools / editors, and build/run commands on the same codebase from the container.
If you do not have it already, you need to check out Contiki-NG:
```shell
git clone https://github.com/contiki-ng/contiki-ng.git
cd contiki-ng
git submodule update --init --recursive
```

Then, it is a good idea to create an alias that will help start docker with all required options.
On Linux, you can add the following to `~/.profile` or similar, for instance, to `~/.bashrc`:
```shell
export CNG_PATH=<absolute-path-to-your-contiki-ng>
alias contiker="docker run --privileged --sysctl net.ipv6.conf.all.disable_ipv6=0 --mount type=bind,source=$CNG_PATH,destination=/home/user/contiki-ng -e DISPLAY=$DISPLAY -e LOCAL_UID=$(id -u $USER) -e LOCAL_GID=$(id -g $USER) -v /tmp/.X11-unix:/tmp/.X11-unix -v /dev/bus/usb:/dev/bus/usb -ti contiker/contiki-ng"
```

## Launching and Exiting

### Shell for new container
To start a bash inside a new container, simply type:
```shell
contiker
```

You will be under `/home/user/contiki-ng` in the container, which is mapped to your local copy of Contiki-NG.

### Additional shell for existing container
Typing `contiker` as above will launch a new container. Sometimes it is useful to have multiple terminal sessions within a single container, e.g., to run a tunslip6 on one terminal and other commands on another one. To achieve this, start by running:

```shell
docker ps
```
This will present you with a list of container IDs. Select the ID of the container you wish to open a terminal for and then

```shell
docker exec -it <the ID> /bin/bash
```

### Exit
To exit a container, use `exit`.

## Usage

From the container, you can directly go to an example project and build it (see [tutorial:hello-world]).
It is also possible to run CI tests, e.g.:
```shell
cd tests/14-rpl-lite
make 01-rpl-up-route.testlog
# Running test 01-rpl-up-route with random Seed 1.................... OK
```

You can even start Cooja from the container:
```shell
cd tools/cooja
./gradlew run
```

Or use the shortcut:
```shell
cooja
```

Or directly from the host (outside the container)
```shell
contiker cooja
```

It is also possible to start a container to just run one command, e.g.:
```shell
contiker bash -c ls
```

To run a CI test:
```shell
contiker bash -c "make -C tests/14-rpl-lite 01-rpl-up-route.testlog"
```
The user has `sudo` rights with no password (obviously sandboxed in the container).

[tutorial:hello-world]: /doc/tutorials/Hello,-World!
[install-docker-ce]: https://docs.docker.com/install/linux/docker-ce/ubuntu/#install-docker-ce

## On Windows
### Prerequisites
* [VcXsrv][download-vcxsrv]
* [Docker for Windows][install-windows-docker-ce]; enable "Shared Drives" feature with a drive where you have contiki-ng local repository.

### Limitations
* Cannot use USB from a container as of writing (See: https://github.com/docker/for-win/issues/1018)
* contiki-ng repository MUST NOT be one which is cloned within a WSL environment and accessed by a path under `%LOCALAPPDATA%`. See [this post](https://devblogs.microsoft.com/commandline/do-not-change-linux-files-using-windows-apps-and-tools/) for this limitation.

### How to Run
1. Start VcXsrv (run `XLaunch.exe`)
1. Open `cmd.exe` (you can use PowerShell if you want)
1. Hit the following command (replace `/c/Users/foobar/contiki-ng` with a location of contiki-ng local repository in your environment)
```
C:\> docker run --privileged --sysctl net.ipv6.conf.all.disable_ipv6=0 --mount type=bind,source=/c/Users/foobar/contiki-ng,destination=/home/user/contiki-ng -e DISPLAY="host.docker.internal:0.0" -ti contiker/contiki-ng
```
Tested with Windows 10, version 1809.

You can run a Docker container from a WSL environment as well:
1. Prepare `/etc/wsl.conf` to make WSL mount drives directly under `/` instead of under `/mnt` (see this [doc][wsl.conf] for further information)
1. Place `contiki-ng` local repository somewhere other than under `%LOCALAPPDATA%`, for instance, `/c/Users/foobar/contiki-ng`
1. Run the following command in a WSL shell
```shell
docker run --privileged --sysctl net.ipv6.conf.all.disable_ipv6=0 --mount type=bind,source=/c/Users/foobar/contiki-ng,destination=/home/user/contiki-ng -e DISPLAY=host.docker.internal:0.0 -ti contiker/contiki-ng
```

[install-windows-docker-ce]:https://docs.docker.com/docker-for-windows/install/
[download-vcxsrv]:https://sourceforge.net/projects/vcxsrv/
[wsl.conf]:https://docs.microsoft.com/en-us/windows/wsl/wsl-config#set-wsl-launch-settings 

## On macOS

There are two Docker solutions available: [Docker for Mac](https://docs.docker.com/docker-for-mac/) and [Docker Toolbox on macOS](https://docs.docker.com/toolbox/toolbox_install_mac/).
Refer to [Docker for Mac vs. Docker Toolbox](https://docs.docker.com/docker-for-mac/docker-toolbox/) for general differences between the solutions.

If you want to access USB devices from a Docker container, "Docker Toolbox on macOS" is the **only** choice as of writhing this.
"Docker for Mac" doesn't support USB pass-through (https://docs.docker.com/docker-for-mac/faqs/#questions-about-dockerapp).

### Without XQuartz
If you don't need to run `cooja` with its GUI, the setup procedure becomes simple:

1. install "Docker for Mac" or "Docker Toolbox on macOS"
1. prepare `contiker` alias
1. run contiker: `$ contiker bash`

`contiker` alias you need is slightly different depending on your Docker solution. Note you need `CNG_PATH` definition as mentioned above.

#### for "Docker for Mac"
```shell
export CNG_PATH=<absolute-path-to-your-contiki-ng>
alias contiker="docker run                                                           \
               --privileged                                                          \
               --sysctl net.ipv6.conf.all.disable_ipv6=0                             \
               --mount type=bind,source=$CNG_PATH,destination=/home/user/contiki-ng  \
               -ti contiker/contiki-ng"
```

#### for "Docker Toolbox on macOS"
```shell
export CNG_PATH=<absolute-path-to-your-contiki-ng>
alias contiker="docker run                                                           \
               --privileged                                                          \
               --sysctl net.ipv6.conf.all.disable_ipv6=0                             \
               --mount type=bind,source=$CNG_PATH,destination=/home/user/contiki-ng  \
               --device=/dev/ttyUSB0                                                 \
               --device=/dev/ttyUSB1                                                 \
               -ti contiker/contiki-ng"
```

### With XQuartz

In order to access the X server from a Docker container, you need to use `socat` because neither of the Docker solutions can handle Unix domain sockets properly.
Note that your host-based firewall may block connections on TCP Port 6000.

1. install "Docker for Mac" or "Docker Toolbox on macOS"
1. install XQuartz: `$ brew cask install xquartz` (may need reboot)
1. install socat: `$ brew install socat`
1. prepare `contiker` alias
1. open XQuartz: `$ open -a XQuartz`
1. map TCP Port 6000 of 127.0.0.1 to the Unix-domain socket for XQuartz:
    ```shell
    socat TCP-LISTEN:6000,reuseaddr,fork UNIX-CLIENT:\"$DISPLAY\"
    ```
1. run contiker: `$ contiker bash`

Put the following lines into `~/.profile` or similar below the `CNG_PATH` definition.

#### for "Docker for Mac"
```shell
export CNG_PATH=<absolute-path-to-your-contiki-ng>
alias contiker="docker run --privileged \
  --mount type=bind,source=$CNG_PATH,destination=/home/user/contiki-ng \
  --sysctl net.ipv6.conf.all.disable_ipv6=0 \
  -e DISPLAY=docker.for.mac.host.internal:0 \
  -ti contiker/contiki-ng"
```

#### for "Docker Toolbox on macOS"
```shell
export CNG_PATH=<absolute-path-to-your-contiki-ng>
alias contiker="docker run --privileged \
  --mount type=bind,source=$CNG_PATH,destination=/home/user/contiki-ng \
  --sysctl net.ipv6.conf.all.disable_ipv6=0 \
  --device=/dev/ttyUSB0 \
  --device=/dev/ttyUSB1 \
  -e DISPLAY=docker.for.mac.host.internal:0 \
  -ti contiker/contiki-ng"
```

You need to enable USB devices in VirtualBox; start Virtualbox, and edit the settings for the machine running Docker to allow USB devices. You may want to download Oracle VM VirtualBox Extension Pack for USB 2.0 and USB 3.0 drivers.

## Using a different docker image
On occasion, changes to the Contiki-NG source code base require corresponding changes to the docker image, for example to include a new package dependency. This has caused issues in the past when we were distributing a single docker image based on the latest version of branch `develop`.

As of release v4.5 (#1108), we host multiple docker container images on DockerHub, as [`contiker/contiki-ng`](https://hub.docker.com/r/contiker/contiki-ng). When a user wants to use a specific Contiki-NG version then they can download the corresponding correct docker image:

* `contiker/contiki-ng:latest`: This is the default image. Under all circumstances, this image is expected to be identical to `contiker/contiki-ng:develop` and to work with the latest git version of branch `develop`.
* `contiker/contiki-ng:master`: This is an image that corresponds to the latest version of branch `master`. Typically this will also correspond to the most recent Contiki-NG release.
* `contiker/contiki-ng:develop`: This is an image that corresponds to the latest version of branch `develop`.
* `contiker/contiki-ng:<hash>`: This is an image that was created from `tools/docker` with hash `<hash>`. A new image is automatically pushed to Dockerhub for every commit that causes the hash to change.

Images tagged with a hash allow users who wish to use a specific Contiki-NG git version (after #1108) to download the corresponding container image. Let's assume you wish to use git version NNNNNN. To determine the hash and pull the corresponding container image, you can do something like this:

```shell
git checkout NNNNNN
# [...]
tools/docker/print-dockerhash.sh
# <hash>
docker pull contiker/contiki-ng:<hash>
```

## Running out-of-tree-tests
If you want to run the entire test suite inside docker, you need to either `export CI=true' before running the tests, or start 02-compile-arm-ports with `make OUT_OF_TREE_TEST=1`.

