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
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"

#define LOG_MODULE "BR"
#define LOG_LEVEL LOG_LEVEL_NONE

#ifdef linux
#include <linux/if.h>
#include <linux/if_tun.h>
#endif

#include <err.h>
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "cmd.h"
#include "border-router.h"

extern const char *slip_config_ipaddr;
extern char slip_config_tundev[IFNAMSIZ + 1];
extern uint16_t slip_config_basedelay;

static int tunfd;

static int set_fd(fd_set *rset, fd_set *wset);

static void handle_fd(fd_set *rset, fd_set *wset);

static const struct select_callback tun_select_callback = {
        set_fd,
        handle_fd
};

int ssystem(const char *fmt, ...)
__attribute__((__format__ (__printf__, 1, 2)));

void ifconf_cleanup(const char *dev);

void ifconf(const char *tundev, const char *ipaddr);

int devopen(const char *dev, int flags);


/*---------------------------------------------------------------------------*/
void
cleanup(void) {
    ifconf_cleanup(slip_config_tundev);
}

/*---------------------------------------------------------------------------*/
void CC_NORETURN

sigcleanup(int signo) {
    fprintf(stderr, "signal %d\n", signo);
    exit(0);            /* exit(0) will call cleanup() */
}
/*---------------------------------------------------------------------------*/
#ifdef linux
int
tun_alloc(char *dev, uint16_t devsize)
{
  struct ifreq ifr;
  int fd, err;
  LOG_INFO("Opening: %s\n", dev);
  if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    /* Error message handled by caller */
    return -1;
  }

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_NO_PI - Do not provide packet information
   */
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  if(*dev != '\0') {
    memcpy(ifr.ifr_name, dev, MIN(sizeof(ifr.ifr_name), devsize));
  }
  if((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
    /* Error message handled by caller */
    close(fd);
    return err;
  }
  LOG_INFO("Using '%s' vs '%s'\n", dev, ifr.ifr_name);
  strncpy(dev, ifr.ifr_name, MIN(devsize - 1, sizeof(ifr.ifr_name)));
  dev[devsize - 1] = '\0';
  LOG_INFO("Using %s\n", dev);
  return fd;
}
#else

/*---------------------------------------------------------------------------*/
int
tun_alloc(char *dev, uint16_t devsize) {
    LOG_INFO("Opening: %s\n", dev);
    return devopen(dev, O_RDWR);
}

#endif

static uint16_t delaymsec = 0;
static uint32_t delaystartsec, delaystartmsec;

/*---------------------------------------------------------------------------*/
void
tun_init() {
    setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */

    slip_init();

    LOG_INFO("Opening tun interface:%s\n", slip_config_tundev);

    tunfd = tun_alloc(slip_config_tundev, sizeof(slip_config_tundev));
    if (tunfd == -1) {
        err(1, "tun_init: tun_alloc failed");
    }

    LOG_INFO("Tun open:%d\n", tunfd);

    select_set_callback(tunfd, &tun_select_callback);

    fprintf(stderr, "opened %s device ``/dev/%s''\n",
            "tun", slip_config_tundev);

    atexit(cleanup);
    signal(SIGHUP, sigcleanup);
    signal(SIGTERM, sigcleanup);
    signal(SIGINT, sigcleanup);
    ifconf(slip_config_tundev, slip_config_ipaddr);
}

/*---------------------------------------------------------------------------*/
static int
tun_output(uint8_t *data, int len) {
    if (write(tunfd, data, len) != len) {
        err(1, "serial_to_tun: write");
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
int
tun_input(unsigned char *data, int maxlen) {
    int size;
    if ((size = read(tunfd, data, maxlen)) == -1) {
        err(1, "tun_input: read");
    }
    return size;
}

/*---------------------------------------------------------------------------*/
static void
init(void) {
}

/*---------------------------------------------------------------------------*/
static int
output(void) {
    LOG_DBG("SUT: %u\n", uip_len);
    if (uip_len > 0) {
        return tun_output(uip_buf, uip_len);
    }
    return 0;
}

const struct uip_fallback_interface rpl_interface = {
        init, output
};

/*---------------------------------------------------------------------------*/
/* tun and slip select callback                                              */
/*---------------------------------------------------------------------------*/
static int
set_fd(fd_set *rset, fd_set *wset) {
    FD_SET(tunfd, rset);
    return 1;
}

/*---------------------------------------------------------------------------*/

static void
handle_fd(fd_set *rset, fd_set *wset) {
    /* Optional delay between outgoing packets */
    /* Base delay times number of 6lowpan fragments to be sent */
    /* delaymsec = 10; */
    if (delaymsec) {
        struct timeval tv;
        int dmsec;
        gettimeofday(&tv, NULL);
        dmsec = (tv.tv_sec - delaystartsec) * 1000 + tv.tv_usec / 1000 - delaystartmsec;
        if (dmsec < 0) {
            delaymsec = 0;
        }
        if (dmsec > delaymsec) {
            delaymsec = 0;
        }
    }

    if (delaymsec == 0) {
        int size;

        if (FD_ISSET(tunfd, rset)) {
            size = tun_input(uip_buf, sizeof(uip_buf));
            uip_len = size;
            tcpip_input();

            if (slip_config_basedelay) {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                delaymsec = slip_config_basedelay;
                delaystartsec = tv.tv_sec;
                delaystartmsec = tv.tv_usec / 1000;
            }
        }
    }
}
/*---------------------------------------------------------------------------*/
