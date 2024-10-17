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

#include "contiki.h"
#include "sys/platform.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/netstack.h"
#include "net/ipv6/uiplib.h"
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
#include <net/if.h>
#include <err.h>

#define TUN_PRIO CONTIKI_VERBOSE_PRIO + 30
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Tun6"
#define LOG_LEVEL LOG_LEVEL_WARN

#ifdef __APPLE__
/* utun0-3 are in use on Big Sur, so use utun10 as default */
#define DEFAULT_TUN "utun10"
#else /* __APPLE__ */
#define DEFAULT_TUN "tun0"
#endif /* __APPLE__ */

#define DEFAULT_PREFIX "fd00::1/64"
static const char *config_ipaddr = DEFAULT_PREFIX;
static char config_tundev[IFNAMSIZ + 1] = DEFAULT_TUN;
static void (* tun_input_callback)(void);

/* IPv6 required minimum MTU */
#define MIN_MTU_SIZE 1500
static int config_mtu = MIN_MTU_SIZE;

static int tunfd = -1;

static int set_fd(fd_set *rset, fd_set *wset);
static void handle_fd(fd_set *rset, fd_set *wset);
static const struct select_callback tun_select_callback = {
  set_fd,
  handle_fd
};

static int ssystem(const char *fmt, ...)
     __attribute__((__format__ (__printf__, 1, 2)));

static int
ssystem(const char *fmt, ...)
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
static bool
set_default_prefix(const char *prefix)
{
  char *ipaddr = strdup(prefix);
  if(!ipaddr) {
    return false;
  }

  bool success = false;
  char *s = strchr(ipaddr, '/');
  if(s) {
    uip_ip6addr_t prefix_addr;
    *s = '\0';
    if(uiplib_ipaddrconv(ipaddr, &prefix_addr)) {
      uip_ds6_set_default_prefix(&prefix_addr);
      success = true;
    }
  }
  free(ipaddr);
  return success;
}
/*---------------------------------------------------------------------------*/
const char *
tun6_net_get_prefix(void)
{
  return config_ipaddr;
}
/*---------------------------------------------------------------------------*/
void
tun6_net_set_prefix(const char *prefix)
{
  if(!set_default_prefix(prefix)) {
    LOG_WARN("Failed to set default prefix %s\n", prefix);
  }
  config_ipaddr = prefix;
}
/*---------------------------------------------------------------------------*/
const char *
tun6_net_get_tun_name(void)
{
  return config_tundev;
}
/*---------------------------------------------------------------------------*/
void
tun6_net_set_tun_name(const char *tun_name)
{
  /* Ignore "/dev/" if present in tun device name */
  if(strncmp("/dev/", tun_name, 5) == 0) {
    tun_name += 5;
  }
  strncpy(config_tundev, tun_name, sizeof(config_tundev) - 1);
  config_tundev[sizeof(config_tundev) - 1] = '\0';
}
/*---------------------------------------------------------------------------*/
int
tun6_net_get_mtu(void)
{
  return config_mtu;
}
/*---------------------------------------------------------------------------*/
void
tun6_net_set_mtu(int mtu_size)
{
  if(mtu_size < MIN_MTU_SIZE) {
    LOG_WARN("ignoring too small MTU size %d, using %d\n",
             mtu_size, config_mtu);
  } else {
    config_mtu = mtu_size;
  }
}
/*---------------------------------------------------------------------------*/
static int
tun_dev_callback(const char *optarg)
{
  tun6_net_set_tun_name(optarg);
  return 0;
}
CONTIKI_OPTION(TUN_PRIO, { "t", required_argument, NULL, 0 },
               tun_dev_callback,
               "name of tun interface (default: " DEFAULT_TUN ")\n");

/*---------------------------------------------------------------------------*/
static int
prefix_callback(const char *optarg)
{
  tun6_net_set_prefix(optarg);
  return 0;
}
CONTIKI_OPTION(TUN_PRIO + 1, { "prefix", required_argument, NULL, 0 },
               prefix_callback,
               "Subnet prefix (default: " DEFAULT_PREFIX ")\n");
/*---------------------------------------------------------------------------*/
static int
mtu_callback(const char *optarg)
{
  tun6_net_set_mtu(atoi(optarg));
  return 0;
}
CONTIKI_OPTION(TUN_PRIO + 2, { "mtu", required_argument, NULL, 0 },
               mtu_callback, "interface MTU size\n");
/*---------------------------------------------------------------------------*/
static void
cleanup(void)
{
#define TMPBUFSIZE 128
  /* Called from signal handler, avoid unsafe functions. */
  char buf[TMPBUFSIZE];
#ifdef __APPLE__
  strcpy(buf, "ifconfig ");
  /* Will not overflow, but null-terminate to avoid spurious warnings. */
  buf[TMPBUFSIZE - 1] = '\0';
  strncat(buf, config_tundev, TMPBUFSIZE - strlen(buf) - 1);
  strncat(buf, " inet6 ", TMPBUFSIZE - strlen(buf) - 1);
  strncat(buf, config_ipaddr, TMPBUFSIZE - strlen(buf) - 1);
  strncat(buf, " remove", TMPBUFSIZE - strlen(buf) - 1);
  system(buf);
#endif /* __APPLE__ */

  strcpy(buf, "ifconfig ");
  /* Will not overflow, but null-terminate to avoid spurious warnings. */
  buf[TMPBUFSIZE - 1] = '\0';
  strncat(buf, config_tundev, TMPBUFSIZE - strlen(buf) - 1);
  strncat(buf, " down", TMPBUFSIZE - strlen(buf) - 1);
  system(buf);

#ifndef __APPLE__
#ifndef linux
  system("sysctl -w net.ipv6.conf.all.forwarding=1");
#endif

  strcpy(buf, "netstat -nr"
         " | awk '{ if ($2 == \"");
  buf[TMPBUFSIZE - 1] = '\0';
  strncat(buf, config_tundev, TMPBUFSIZE - strlen(buf) - 1);
  strncat(buf, "\") print \"route delete -net \"$1; }'"
          " | sh", TMPBUFSIZE - strlen(buf) - 1);
  system(buf);
#endif /* !__APPLE__ */
}
/*---------------------------------------------------------------------------*/
static void CC_NORETURN
sigcleanup(int signo)
{
  const char *prefix = "signal ";
  const char *sig =
    signo == SIGHUP ? "HUP\n" : signo == SIGTERM ? "TERM\n" : "INT\n";
  write(fileno(stderr), prefix, strlen(prefix));
  write(fileno(stderr), sig, strlen(sig));
  cleanup();
  _exit(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------*/
static void
ifconf_setup(void)
{
#ifdef linux
  ssystem("ifconfig %s inet `hostname` mtu %d up", config_tundev, config_mtu);
  ssystem("ifconfig %s add %s", config_tundev, config_ipaddr);
#elif defined(__APPLE__)
  ssystem("ifconfig %s inet6 mtu %d up", config_tundev, config_mtu);
  ssystem("ifconfig %s inet6 %s add", config_tundev, config_ipaddr );
  ssystem("sysctl -w net.inet6.ip6.forwarding=1");
#else
  ssystem("ifconfig %s inet `hostname` %s mtu %d up", cofnig_tundev, config_ipaddr, config_mtu);
  ssystem("sysctl -w net.inet.ip.forwarding=1");
#endif /* !linux */

  /* Print the configuration to the console. */
  ssystem("ifconfig %s\n", config_tundev);
}
/*---------------------------------------------------------------------------*/
#ifdef linux
#include <linux/if.h>
#include <linux/if_tun.h>

static int
tun_alloc(void)
{
  struct ifreq ifr;
  int fd, err;

  LOG_INFO("Opening tun interface %s\n", config_tundev);

  if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    /* Error message handled by caller */
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_NO_PI - Do not provide packet information
   */
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  if(*config_tundev != '\0') {
    strncpy(ifr.ifr_name, config_tundev, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
  }

  if((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
    /* Error message handled by caller */
    close(fd);
    return err;
  }

  LOG_INFO("Using '%s' as '%s'\n", config_tundev, ifr.ifr_name);
  strncpy(config_tundev, ifr.ifr_name, sizeof(config_tundev) - 1);
  config_tundev[sizeof(config_tundev) - 1] = '\0';
  return fd;
}
#elif defined __APPLE__
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <sys/uio.h>

/*
 * Reference for utun on macOS:
 * http://newosxbook.com/src.jl?tree=listings&file=17-15-utun.c
 */
static int
tun_alloc(void)
{
  unsigned int tunif;

  if(sscanf(config_tundev, "utun%u", &tunif) != 1 || tunif >= UINT8_MAX) {
    fprintf(stderr, "tun_alloc: invalid utun interface specified: %s\n", config_tundev);
    return -1;
  }

  LOG_INFO("Opening tun interface %s\n", config_tundev);

  struct ctl_info ctl_info = { 0 };
  if(strlcpy(ctl_info.ctl_name, UTUN_CONTROL_NAME, sizeof(ctl_info.ctl_name)) >=
      sizeof(ctl_info.ctl_name)) {
    fprintf(stderr, "UTUN_CONTROL_NAME too long");
    return -1;
  }

  int fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
  if(fd == -1) {
    perror("socket(SYSPROTO_CONTROL)");
    return -1;
  }

  if(ioctl(fd, CTLIOCGINFO, &ctl_info) == -1) {
    perror("ioctl(CTLIOCGINFO)");
    close(fd);
    return -1;
  }

  struct sockaddr_ctl sc;
  sc.sc_id = ctl_info.ctl_id;
  sc.sc_len = sizeof(sc);
  sc.sc_family = AF_SYSTEM;
  sc.ss_sysaddr = AF_SYS_CONTROL;
  sc.sc_unit = tunif + 1;

  /*
   * If the connect is successful, a utun%d device will be created, where "%d"
   * is our unit number -1
   */

  if(connect(fd, (struct sockaddr *)&sc, sizeof(sc)) == -1) {
    perror("connect(AF_SYS_CONTROL)");
    close(fd);
    return -1;
  }

  return fd;
}
#else
static int
tun_alloc(void)
{
  char t[8 + sizeof(config_tundev)] = "/dev/";
  strncat(t, config_tundev, sizeof(t) - 6);
  t[sizeof(t) - 1] = '\0';
  LOG_INFO("Opening tun interface %s\n", t);
  return open(t, O_RDWR);
}
#endif
/*---------------------------------------------------------------------------*/
bool
tun6_net_init(void (* tun_input)(void))
{
  if(!tun_input) {
    return false;
  }
  tun_input_callback = tun_input;

  setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */

  tunfd = tun_alloc();
  if(tunfd == -1) {
    return false;
  }

  LOG_INFO("Tun open:%d\n", tunfd);

  select_set_callback(tunfd, &tun_select_callback);

  fprintf(stderr, "opened %s device ``/dev/%s''\n",
          "tun", config_tundev);

  atexit(cleanup);
  signal(SIGHUP, sigcleanup);
  signal(SIGTERM, sigcleanup);
  signal(SIGINT, sigcleanup);
  ifconf_setup();
  return true;
}
/*---------------------------------------------------------------------------*/
int
tun6_net_output(uint8_t *data, int len)
{
  if(tunfd == -1) {
    return 0;
  }

#ifdef __APPLE__
  /* Fake IFF_NO_PI on macOS by sending a 4 byte header containing AF_INET6 */
  u_int32_t type = htonl(AF_INET6);
  struct iovec iv[2];

  iv[0].iov_base = &type;
  iv[0].iov_len = sizeof(type);
  iv[1].iov_base = data;
  iv[1].iov_len = len;

  if(writev(tunfd, iv, 2) != (sizeof(type) + len)) {
    err(EXIT_FAILURE, "tun6_net_output: writev");
  }
#else
  if(write(tunfd, data, len) != len) {
    err(EXIT_FAILURE, "tun6_net_output: write");
  }
#endif

  return 0;
}
/*---------------------------------------------------------------------------*/
int
tun6_net_input(uint8_t *data, int maxlen)
{
  int size;

  if(tunfd == -1) {
    /* tun is not open */
    return 0;
  }

  if((size = read(tunfd, data, maxlen)) == -1) {
    err(EXIT_FAILURE, "tun6_net_input: read");
  }

#ifdef __APPLE__
#define UTUN_HEADER_LEN 4
  /* Fake IFF_NO_PI on macOS by ignoring the first 4 bytes containing AF_INET6 */
  if(size <= UTUN_HEADER_LEN) {
    err(EXIT_FAILURE, "tun6_net_input: read too small");
  }

  size -= UTUN_HEADER_LEN;
  memmove(data, data + UTUN_HEADER_LEN, size);
#undef UTUN_HEADER_LEN
#endif /* __APPLE__ */

  return size;
}

/*---------------------------------------------------------------------------*/
/* tun select callback                                                       */
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
  if(tunfd == -1) {
    /* tun is not open */
    return;
  }

  if(FD_ISSET(tunfd, rset)) {
    tun_input_callback();
  }
}

/*---------------------------------------------------------------------------*/
/* network callbacks                                                         */
/*---------------------------------------------------------------------------*/
static void
tun_input(void)
{
  int size = tun6_net_input(uip_buf, sizeof(uip_buf));
  LOG_DBG("TUN data incoming read:%d\n", size);
  uip_len = size;
  tcpip_input();
}
/*---------------------------------------------------------------------------*/
static void
network_init(void)
{
  if(!tun6_net_init(tun_input)) {
    LOG_WARN("Failed to open tun device (you may be lacking permission). Running without network.\n");
  }
}
/*---------------------------------------------------------------------------*/
static uint8_t
network_output(const linkaddr_t *localdest)
{
  if(uip_len > 0) {
    LOG_DBG("output: %u bytes to ", uip_len);
    LOG_DBG_LLADDR(localdest);
    LOG_DBG_("\n");
    return tun6_net_output(uip_buf, uip_len);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
network_input(void)
{
  /* should not happen */
  LOG_DBG("unexpected network input\n");
}
/*---------------------------------------------------------------------------*/
const struct network_driver tun6_net_driver = {
  "tun6",
  network_init,
  network_input,
  network_output
};
/*---------------------------------------------------------------------------*/
