/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/**
 * \file
 *   ip64 driver backed by a Linux TAP device. A TAP device carries Ethernet
 *   frames, which is exactly what ip64 emits and expects, so frames pass in
 *   both directions untouched. The host end of the device is given an IPv4
 *   address, which makes the node reachable from ordinary host software.
 *
 *   Creating the device and configuring the interface both need
 *   CAP_NET_ADMIN, as they do for the tun device that the native platform
 *   opens for IPv6. Linux only.
 */

#include "contiki.h"
#include "ip64-tap-driver.h"
#include "ip64/ip64-eth-interface.h"
#include "ip64/ip64-eth.h"

#include <ctype.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "sys/log.h"
#define LOG_MODULE "ip64-tap"
#define LOG_LEVEL LOG_LEVEL_INFO

/* The device is overridable from the environment so that the test script can
   pick one that does not collide with whatever else the machine is running.
   The addresses are not: the node answers to an address of its own that is
   compiled in, and the test script is written around both. */
#define TAP_DEV_DEFAULT "tap0"
#define TAP_HOST_ADDR   "192.0.2.1"
#define TAP_NETMASK     "255.255.255.0"

#define TAP_BUFSIZE 1600

static int tapfd = -1;
static uint8_t rxbuf[TAP_BUFSIZE];

/*---------------------------------------------------------------------------*/
static const char *
env_or(const char *name, const char *fallback)
{
  const char *value = getenv(name);

  return (value != NULL && *value != '\0') ? value : fallback;
}
/*---------------------------------------------------------------------------*/
/* The name ends up in a command handed to a shell, and one that the kernel
   truncates would configure a different device than the one created. */
static bool
dev_name_is_usable(const char *dev)
{
  size_t i;

  if(*dev == '\0' || strlen(dev) >= IFNAMSIZ) {
    return false;
  }

  for(i = 0; dev[i] != '\0'; i++) {
    if(!isalnum((unsigned char)dev[i]) &&
       dev[i] != '-' && dev[i] != '_' && dev[i] != '.') {
      return false;
    }
  }

  return true;
}
/*---------------------------------------------------------------------------*/
static int
set_fd(fd_set *rset, fd_set *wset)
{
  FD_SET(tapfd, rset);

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
handle_fd(fd_set *rset, fd_set *wset)
{
  ssize_t len;

  if(!FD_ISSET(tapfd, rset)) {
    return;
  }

  len = read(tapfd, rxbuf, sizeof(rxbuf));
  if(len <= 0) {
    LOG_ERR("Read from the TAP device failed\n");
    return;
  }

  /* ip64_eth_interface_input() reads the header before it looks at the
     length it is given. */
  if(len < (ssize_t)sizeof(struct ip64_eth_hdr)) {
    LOG_WARN("Discarding a frame of %d bytes\n", (int)len);
    return;
  }

  /* The frame arrives exactly as an Ethernet chip would deliver it. */
  ip64_eth_interface_input(rxbuf, (uint16_t)len);
}
/*---------------------------------------------------------------------------*/
static const struct select_callback tap_select_callback = {
  set_fd, handle_fd
};
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  struct ifreq ifr;
  const char *dev = env_or("IP64_TAP_DEV", TAP_DEV_DEFAULT);
  char cmd[128];

  if(!dev_name_is_usable(dev)) {
    LOG_ERR("IP64_TAP_DEV is not a usable device name\n");
    exit(EXIT_FAILURE);
  }

  tapfd = open("/dev/net/tun", O_RDWR);
  if(tapfd < 0) {
    LOG_ERR("Cannot open /dev/net/tun; CAP_NET_ADMIN is required\n");
    exit(EXIT_FAILURE);
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  strncpy(ifr.ifr_name, dev, IFNAMSIZ - 1);
  if(ioctl(tapfd, TUNSETIFF, &ifr) < 0) {
    LOG_ERR("Cannot create the TAP device %s\n", dev);
    exit(EXIT_FAILURE);
  }

  /* ifconfig rather than ip(8), as the native tun driver does, because the
     test image is not guaranteed to carry iproute2. The name comes back from
     the kernel, which is the one the device really has. */
  if(snprintf(cmd, sizeof(cmd), "ifconfig %s %s netmask %s up",
              ifr.ifr_name, TAP_HOST_ADDR, TAP_NETMASK) >= (int)sizeof(cmd)) {
    LOG_ERR("The command to configure %s does not fit\n", ifr.ifr_name);
    exit(EXIT_FAILURE);
  }
  if(system(cmd) != 0) {
    LOG_ERR("Cannot configure %s with address %s\n", ifr.ifr_name,
            TAP_HOST_ADDR);
    exit(EXIT_FAILURE);
  }

  select_set_callback(tapfd, &tap_select_callback);

  LOG_INFO("TAP device %s is up, host address %s\n", ifr.ifr_name,
           TAP_HOST_ADDR);
}
/*---------------------------------------------------------------------------*/
static int
output(uint8_t *packet, uint16_t packet_len)
{
  if(write(tapfd, packet, packet_len) != packet_len) {
    LOG_ERR("Write of %u bytes to the TAP device failed\n", packet_len);
    return 0;
  }

  return packet_len;
}
/*---------------------------------------------------------------------------*/
const struct ip64_driver ip64_tap_driver = { init, output };
/*---------------------------------------------------------------------------*/
