/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Tun6"
#define LOG_LEVEL LOG_LEVEL_WARN

#ifdef linux
#include <linux/if.h>
#include <linux/if_tun.h>
#endif

#include <err.h>
#include "net/netstack.h"
#include "net/packetbuf.h"

static const char *config_ipaddr = "fd00::1/64";
/* Allocate some bytes in RAM and copy the string */
static char config_tundev[64] = "tun0";


#ifndef __CYGWIN__
static int tunfd = -1;

static int set_fd(fd_set *rset, fd_set *wset);
static void handle_fd(fd_set *rset, fd_set *wset);
static const struct select_callback tun_select_callback = {
  set_fd,
  handle_fd
};
#endif /* __CYGWIN__ */

static int ssystem(const char *fmt, ...)
     __attribute__((__format__ (__printf__, 1, 2)));
static int
ssystem(const char *fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));

int
static ssystem(const char *fmt, ...)
{
  char cmd[128];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(cmd, sizeof(cmd), fmt, ap);
  va_end(ap);
  LOG_INFO("%s\n", cmd);
  fflush(stdout);
  return system(cmd);
}

/*---------------------------------------------------------------------------*/
static void
cleanup(void)
{
  ssystem("ifconfig %s down", config_tundev);
#ifndef linux
  ssystem("sysctl -w net.ipv6.conf.all.forwarding=1");
#endif
  ssystem("netstat -nr"
	  " | awk '{ if ($2 == \"%s\") print \"route delete -net \"$1; }'"
	  " | sh",
	  config_tundev);
}

/*---------------------------------------------------------------------------*/
static void
sigcleanup(int signo)
{
  fprintf(stderr, "signal %d\n", signo);
  exit(0);			/* exit(0) will call cleanup() */
}

/*---------------------------------------------------------------------------*/
static void
ifconf(const char *tundev, const char *ipaddr)
{
#ifdef linux
  ssystem("ifconfig %s inet `hostname` up", tundev);
  ssystem("ifconfig %s add %s", tundev, ipaddr);
#elif defined(__APPLE__)
  ssystem("ifconfig %s inet6 %s up", tundev, ipaddr);
  ssystem("sysctl -w net.inet.ip.forwarding=1");
#else
  ssystem("ifconfig %s inet `hostname` %s up", tundev, ipaddr);
  ssystem("sysctl -w net.inet.ip.forwarding=1");
#endif /* !linux */

  /* Print the configuration to the console. */
  ssystem("ifconfig %s\n", tundev);
}
/*---------------------------------------------------------------------------*/
#ifdef linux
static int
tun_alloc(char *dev)
{
  struct ifreq ifr;
  int fd, err;
  LOG_INFO("Opening: %s\n", dev);
  if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
    /* Error message handled by caller */
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_NO_PI - Do not provide packet information
   */
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  if(*dev != 0) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }
  if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
    /* Error message handled by caller */
    close(fd);
    return err;
  }

  LOG_INFO("Using '%s' vs '%s'\n", dev, ifr.ifr_name);
  strncpy(dev, ifr.ifr_name, strlen(dev));
  LOG_INFO("Using %s\n", dev);
  return fd;
}
#else
static int
devopen(const char *dev, int flags)
{
  char t[32];
  strcpy(t, "/dev/");
  strncat(t, dev, sizeof(t) - 5);
  return open(t, flags);
}
/*---------------------------------------------------------------------------*/
static int
tun_alloc(char *dev)
{
  LOG_INFO("Opening: %s\n", dev);
  return devopen(dev, O_RDWR);
}
#endif

#ifdef __CYGWIN__
/*wpcap process is used to connect to host interface */
static void
tun_init()
{
  setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */
}

#else


/*---------------------------------------------------------------------------*/
static void
tun_init()
{
  setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */

  LOG_INFO("Initializing tun interface\n");

  tunfd = tun_alloc(config_tundev);
  if(tunfd == -1) {
    LOG_WARN("Failed to open tun device (you may be lacking permission). Running without network.\n");
    /* err(1, "failed to allocate tun device ``%s''", config_tundev); */
    return;
  }

  LOG_INFO("Tun open:%d\n", tunfd);

  select_set_callback(tunfd, &tun_select_callback);

  fprintf(stderr, "opened %s device ``/dev/%s''\n",
          "tun", config_tundev);

  atexit(cleanup);
  signal(SIGHUP, sigcleanup);
  signal(SIGTERM, sigcleanup);
  signal(SIGINT, sigcleanup);
  ifconf(config_tundev, config_ipaddr);
}

/*---------------------------------------------------------------------------*/
static int
tun_output(uint8_t *data, int len)
{
  /* fprintf(stderr, "*** Writing to tun...%d\n", len); */
  if(tunfd != -1 && write(tunfd, data, len) != len) {
    err(1, "serial_to_tun: write");
    return -1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
tun_input(unsigned char *data, int maxlen)
{
  int size;

  if(tunfd == -1) {
    /* tun is not open */
    return 0;
  }

  if((size = read(tunfd, data, maxlen)) == -1) {
    err(1, "tun_input: read");
  }
  return size;
}

/*---------------------------------------------------------------------------*/
static uint8_t
output(const linkaddr_t *localdest)
{
  LOG_DBG("SUT: %u\n", uip_len);
  if(uip_len > 0) {
    return tun_output(&uip_buf[UIP_LLH_LEN], uip_len);
  }
  return 0;
}

/*---------------------------------------------------------------------------*/
/* tun and slip select callback                                              */
/*---------------------------------------------------------------------------*/
static int
set_fd(fd_set *rset, fd_set *wset)
{
  if(tunfd == -1) {
    return 0;
  }

  FD_SET(tunfd, rset);
  return 1;
}

/*---------------------------------------------------------------------------*/

static void
handle_fd(fd_set *rset, fd_set *wset)
{
  int size;

  if(tunfd == -1) {
    /* tun is not open */
    return;
  }

  LOG_INFO("Tun6-handle FD\n");

  if(FD_ISSET(tunfd, rset)) {
    size = tun_input(&uip_buf[UIP_LLH_LEN], sizeof(uip_buf));
    LOG_DBG("TUN data incoming read:%d\n", size);
    uip_len = size;
    tcpip_input();
  }
}
#endif /*  __CYGWIN_ */

static void input(void)
{
  /* should not happen */
  LOG_DBG("Tun6 - input\n");
}


const struct network_driver tun6_net_driver ={
  "tun6",
  tun_init,
  input,
  output
};


/*---------------------------------------------------------------------------*/
