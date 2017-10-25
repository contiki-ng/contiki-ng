/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
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
 *         Simple posix main loop with support functions.
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "lwm2m-engine.h"
#include "lwm2m-rd-client.h"
#include "coap-timer.h"
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <err.h>
#include <stdio.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef SELECT_CONF_MAX
#define SELECT_MAX SELECT_CONF_MAX
#else
#define SELECT_MAX 8
#endif

static const struct select_callback *select_callback[SELECT_MAX];
static int select_max = 0;
static int is_stdin_open = 1;
static void (* stdin_callback)(const char *line);
static char stdin_buffer[2048];
static uint16_t stdin_count;
/*---------------------------------------------------------------------------*/
int
select_set_callback(int fd, const struct select_callback *callback)
{
  int i;
  if(fd >= 0 && fd < SELECT_MAX) {
    /* Check that the callback functions are set */
    if(callback != NULL &&
       (callback->set_fd == NULL || callback->handle_fd == NULL)) {
      callback = NULL;
    }

    select_callback[fd] = callback;

    /* Update fd max */
    if(callback != NULL) {
      if(fd > select_max) {
        select_max = fd;
      }
    } else {
      select_max = 0;
      for(i = SELECT_MAX - 1; i > 0; i--) {
        if(select_callback[i] != NULL) {
          select_max = i;
          break;
        }
      }
    }
    return 1;
  }
  fprintf(stderr, "*** failed to set callback for fd %d\n", fd);
  return 0;
}
/*---------------------------------------------------------------------------*/
void
select_set_stdin_callback(void (* line_read)(const char *line))
{
  stdin_callback = line_read;
}
/*---------------------------------------------------------------------------*/
static int
stdin_set_fd(fd_set *rset, fd_set *wset)
{
  if(is_stdin_open) {
    FD_SET(STDIN_FILENO, rset);
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
stdin_handle_fd(fd_set *rset, fd_set *wset)
{
  ssize_t ret;
  char c;
  if(is_stdin_open && FD_ISSET(STDIN_FILENO, rset)) {
    ret = read(STDIN_FILENO, &c, 1);
    if(ret > 0) {
      if(c == '\r') {
        /* Ignore CR */
      } else if(c == '\n' || stdin_count >= sizeof(stdin_buffer) - 1) {
        stdin_buffer[stdin_count] = 0;
        if(stdin_count > 0 && stdin_callback != NULL) {
          stdin_callback(stdin_buffer);
        } else {
          fprintf(stderr, "STDIN: %s\n", stdin_buffer);
        }
        stdin_count = 0;
      } else {
        stdin_buffer[stdin_count++] = (char)c;
      }
    } else if(ret == 0) {
      /* Standard in closed */
      is_stdin_open = 0;
      fprintf(stderr, "*** stdin closed\n");
      stdin_count = 0;
      if(stdin_callback) {
        stdin_buffer[0] = 0;
        stdin_callback(stdin_buffer);
      }
    } else if(errno != EAGAIN) {
      err(1, "stdin: read");
    }
  }
}
/*---------------------------------------------------------------------------*/
const static struct select_callback stdin_fd = {
  stdin_set_fd, stdin_handle_fd
};
/*---------------------------------------------------------------------------*/
int
main(int argc, char *argv[])
{
  uint64_t next_time;
  fd_set fdr, fdw;
  int maxfd, retval, i;
  struct timeval tv;

  /* Make standard output unbuffered. */
  setvbuf(stdout, (char *)NULL, _IONBF, 0);

  select_set_callback(STDIN_FILENO, &stdin_fd);

  /* Start the application */
  start_application(argc, argv);

  while(1) {
    tv.tv_sec = 0;
    tv.tv_usec = 250;

    next_time = coap_timer_time_to_next_expiration();
    if(next_time > 0) {
      tv.tv_sec = next_time / 1000;
      tv.tv_usec = (next_time % 1000) * 1000;
      if(tv.tv_usec == 0 && tv.tv_sec == 0) {
        /*
         * CoAP timer time resolution is milliseconds. Avoid millisecond
         * busy loops.
         */
        tv.tv_usec = 250;
      }
    }

    FD_ZERO(&fdr);
    FD_ZERO(&fdw);
    maxfd = 0;
    for(i = 0; i <= select_max; i++) {
      if(select_callback[i] != NULL && select_callback[i]->set_fd(&fdr, &fdw)) {
        maxfd = i;
      }
    }

    retval = select(maxfd + 1, &fdr, &fdw, NULL, &tv);
    if(retval < 0) {
      if(errno != EINTR) {
        perror("select");
      }
    } else if(retval > 0) {
      /* timeout => retval == 0 */
      for(i = 0; i <= maxfd; i++) {
        if(select_callback[i] != NULL) {
          select_callback[i]->handle_fd(&fdr, &fdw);
        }
      }
    }

    /* Process network timers */
    for(retval = 0; retval < 5 && coap_timer_run(); retval++);
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
