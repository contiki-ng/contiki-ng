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
#include "net/ipv6/uiplib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
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
#include "net/linkaddr.h"
/*---------------------------------------------------------------------------*/
static char config_tundev[64] = "tun0";
static int log_level = LOG_LEVEL_INFO;
static const char *config_ipaddr = "fd00::1/64";
/*---------------------------------------------------------------------------*/
/*
 * Default link layer address to fall back to if the user does not specify
 * one from the command line.
 */
#ifdef TUN6_NET_CONF_LL_ADDR
static const uint8_t ll_addr[] = TUN6_NET_CONF_LL_ADDR;
#else
static const uint8_t ll_addr[] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
};
#endif
/*---------------------------------------------------------------------------*/
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
set_lladdr(void)
{
  linkaddr_t addr;

  memset(&addr, 0, sizeof(linkaddr_t));
  memcpy(addr.u8, &ll_addr[sizeof(ll_addr) - LINKADDR_SIZE], sizeof(addr.u8));
  linkaddr_set_node_addr(&addr);
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
set_log_level(int level)
{
  if(level < LOG_LEVEL_NONE || level > LOG_LEVEL_DBG) {
    return;
  }

  log_set_level("all", log_level);
}
/*---------------------------------------------------------------------------*/
void
tun6_net_parse_args(int argc, char **argv)
{
  const char *prog;
  int c;
  linkaddr_t addr;
  uip_ip6addr_t prefix;
  bool use_default_ll_addr = true;

  prog = argv[0];

  while((c = getopt(argc, argv, "hl:t:v::")) != -1) {
    switch(c) {
    case 't':
      if(strncmp("/dev/", optarg, 5) == 0) {
        strncpy(config_tundev, optarg + 5, sizeof(config_tundev));
      } else {
        strncpy(config_tundev, optarg, sizeof(config_tundev));
      }
      break;

    case 'l':
      /* Link layer address */
      if(linkaddr_from_string(&addr, optarg) == LINKADDR_SIZE) {
        linkaddr_set_node_addr(&addr);
        use_default_ll_addr = false;
      }

      break;
    case 'v':
      log_level = 3;
      if(optarg) {
        log_level = strtol(optarg, NULL, 10);
      }

      set_log_level(log_level);
      break;

    case 'h':
    default:
      fprintf(stderr, "usage:  %s [options] [ipaddress]\n", prog);
      fprintf(stderr, "example: %s -t tun1 -l 01:23:F:7:89:AB:CD:EF -v2 "
                      "fd00::1/64\n", prog);
      fprintf(stderr, "If ipaddress is omitted, the default is %s\n", config_ipaddr);
      fprintf(stderr, "Options are:\n");
      fprintf(stderr, " -t tundev      Name of interface (default tun0)\n");
      fprintf(stderr, " -l address     Link layer address to assign to the node.\n");
      fprintf(stderr, "                Must be of a standard MAC address\n");
      fprintf(stderr, "                format with groups of two hex digits\n");
      fprintf(stderr, "                separated by colons (':'). The min\n");
      fprintf(stderr, "                required length depends on the value\n");
      fprintf(stderr, "                of LINKADDR_SIZE.\n");
#ifdef __APPLE__
      fprintf(stderr, " -v level       Set logging level to level.\n");
#else
      fprintf(stderr, " -v[level]      Set logging level to level.\n");
#endif
      fprintf(stderr, "                Levels correspond to the values of LOG_LEVEL_ macros.\n");
      fprintf(stderr, "    -v0         Set debug level to LOG_LEVEL_NONE\n");
      fprintf(stderr, "    -v1         Set debug level to LOG_LEVEL_ERROR\n");
      fprintf(stderr, "    -v2         Set debug level to LOG_LEVEL_WARN\n");
      fprintf(stderr, "    -v3         Set debug level to LOG_LEVEL_INFO (default)\n");
      fprintf(stderr, "    -v4         Set debug level to LOG_LEVEL_DEBUG\n");
#ifndef __APPLE__
      fprintf(stderr, "    -v          Equivalent to -v4\n");
#endif
      exit(1);
      break;
    }
  }
  argc -= optind - 1;
  argv += optind - 1;

  if(argc == 2) {
    config_ipaddr = argv[1];
  }

  /*
   * The IPv6 address specified in the command line will be used as the
   * address of the tunnel interface. We need to make sure to use the same
   * IPv6 prefix for our address auto-configuration, otherwise the process
   * will not be reachable over the tunnel interface.
   */
  if(uiplib_ip6addrconv(config_ipaddr, &prefix)) {
    /*
     * The address was parsed successfully. Ideally we would determine the
     * prefix length from the address (/n) and zero-out the last 128-n bits.
     * However, we assume /64 in many places, so for now simply zero-out the
     * last 8 bytes.
     */
    memset(&prefix.u16[4], 0, 8);

#if NETSTACK_CONF_WITH_IPV6
    LOG_INFO("Setting default prefix to ");
    LOG_INFO_6ADDR(&prefix);
    LOG_INFO_("\n");
#endif

    uip_ds6_set_default_prefix(&prefix);
  }


  if(*config_tundev == '\0') {
    /* Use default. */
    strcpy(config_tundev, "tun0");
  }

  if(use_default_ll_addr) {
    LOG_WARN("Invalid link layer address. Using default\n");
    set_lladdr();
  }
}
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

  LOG_INFO("Assigning address %s to ``%s''\n", config_ipaddr, config_tundev);

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
    return tun_output(uip_buf, uip_len);
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

  LOG_DBG("Tun6-handle FD\n");

  if(FD_ISSET(tunfd, rset)) {
    size = tun_input(uip_buf, sizeof(uip_buf));
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
