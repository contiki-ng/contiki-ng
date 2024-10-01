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
 * \file
 *         Slip configuration
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <err.h>
#include "contiki.h"
#include "sys/platform.h"
#include "tun6-net.h"

int slip_config_verbose = 0;
int slip_config_flowcontrol = 0;
int slip_config_timestamp = 0;
const char *slip_config_siodev = NULL;
const char *slip_config_host = NULL;
const char *slip_config_port = NULL;
uint16_t slip_config_basedelay = 0;

#ifndef BAUDRATE
#define BAUDRATE B115200
#endif
speed_t slip_config_b_rate = BAUDRATE;

#define BAUDRATE_PRIO CONTIKI_VERBOSE_PRIO + 20

CONTIKI_USAGE(300, " [ipaddress]\n"
                   "example parameters: -L -v=2 -s /dev/ttyUSB1 fd00::1/64\n\n");
CONTIKI_EXTRA_HELP(300,
                   "\nVerbosity level:\n"
                   "  0   No messages\n"
                   "  1   Encapsulated SLIP debug messages (default)\n"
                   "  2   Printable strings after they are received\n"
                   "  3   Printable strings and SLIP packet notifications\n"
                   "  4   All printable characters as they are received\n"
                   "  5   All SLIP packets in hex\n");
/*---------------------------------------------------------------------------*/
static int
baudrate_callback(const char *optarg)
{
  int baudrate = atoi(optarg);
  switch(baudrate) {
  case -2:
    break;			/* Use default. */
  case 9600:
    slip_config_b_rate = B9600;
    break;
  case 19200:
    slip_config_b_rate = B19200;
    break;
  case 38400:
    slip_config_b_rate = B38400;
    break;
  case 57600:
    slip_config_b_rate = B57600;
    break;
  case 115200:
    slip_config_b_rate = B115200;
    break;
#ifdef linux
  case 921600:
    slip_config_b_rate = B921600;
    break;
#endif
  default:
    fprintf(stderr, "unknown baudrate %s", optarg);
    return 1;
  }
  return 0;
}
CONTIKI_OPTION(BAUDRATE_PRIO, { "B", required_argument, NULL, 0 },
               baudrate_callback,
#ifdef linux
               "baudrate (9600,19200,38400,57600,115200,921600)"
#else
               "baudrate (9600,19200,38400,57600,115200)"
#endif
               " (default 115200)\n");
CONTIKI_OPTION(BAUDRATE_PRIO + 1,
               { "H", no_argument, &slip_config_flowcontrol, 1 }, NULL,
               "hardware CTS/RTS flow control (default disabled)\n");
CONTIKI_OPTION(BAUDRATE_PRIO + 2,
               { "L", no_argument, &slip_config_timestamp, 1 }, NULL,
               "log output format (adds time stamps)\n");
static int
device_callback(const char *optarg)
{
  slip_config_siodev = optarg;
  return 0;
}
CONTIKI_OPTION(BAUDRATE_PRIO + 3, { "s", required_argument, NULL, 0 },
               device_callback, "serial device\n");
static int
host_callback(const char *optarg)
{
  slip_config_host = optarg;
  return 0;
}
CONTIKI_OPTION(BAUDRATE_PRIO + 4, { "a", required_argument, NULL, 0 },
               host_callback, "connect via TCP to server at <value>\n");
static int
port_callback(const char *optarg)
{
  slip_config_port = optarg;
  return 0;
}
CONTIKI_OPTION(BAUDRATE_PRIO + 5, { "p", required_argument, NULL, 0 },
               port_callback, "connect via TCP to server on port <value>\n");
static int
delay_callback(const char *optarg)
{
  slip_config_basedelay = optarg ? atoi(optarg) : 10;
  if(slip_config_basedelay < 0 ||
     (slip_config_basedelay == 0 && optarg && optarg[0] != '0')) {
    fprintf(stderr, "Delay '%s' could not be parsed as a number\n", optarg);
    return 1;
  }

  return 0;
}
CONTIKI_OPTION(BAUDRATE_PRIO + 6, { "d", optional_argument, NULL, 0 },
               delay_callback,
               "minimum delay between outgoing SLIP packets (default 10)\n"
               "\t\tActual delay is basedelay * (#6LowPAN fragments)"
               " milliseconds.\n");
/*---------------------------------------------------------------------------*/
int
slip_config_handle_arguments(int argc, char **argv)
{
  /* For backward compatiblity: assume subnet prefix if exactly one argument
     been specified. */
  if(argc == 2) {
    tun6_net_set_prefix(argv[1]);
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
verbose_callback(const char *optarg)
{
  slip_config_verbose = optarg ? atoi(optarg) : 3;
  if(slip_config_verbose < 0 || slip_config_verbose > 5 ||
     (slip_config_verbose == 0 && optarg && optarg[0] != '0')) {
    fprintf(stderr, "Verbose level '%s' not between 0 and 5\n", optarg);
    return 1;
  }
  return 0;
}
CONTIKI_OPTION(CONTIKI_VERBOSE_PRIO, { "v", optional_argument, NULL, 0 },
               verbose_callback, "verbosity level (0-5)\n");
/*---------------------------------------------------------------------------*/
/* Hidden compatibility options with legacy parameter names. */
CONTIKI_OPTION(CONTIKI_VERBOSE_PRIO + 1,
               { "v0", no_argument, &slip_config_verbose, 0 }, NULL, NULL);
CONTIKI_OPTION(CONTIKI_VERBOSE_PRIO + 2,
               { "v1", no_argument, &slip_config_verbose, 1 }, NULL, NULL);
CONTIKI_OPTION(CONTIKI_VERBOSE_PRIO + 3,
               { "v2", no_argument, &slip_config_verbose, 2 }, NULL, NULL);
CONTIKI_OPTION(CONTIKI_VERBOSE_PRIO + 4,
               { "v3", no_argument, &slip_config_verbose, 3 }, NULL, NULL);
CONTIKI_OPTION(CONTIKI_VERBOSE_PRIO + 5,
               { "v4", no_argument, &slip_config_verbose, 4 }, NULL, NULL);
CONTIKI_OPTION(CONTIKI_VERBOSE_PRIO + 6,
               { "v5", no_argument, &slip_config_verbose, 5 }, NULL, NULL);
