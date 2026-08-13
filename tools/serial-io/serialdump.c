#define _GNU_SOURCE
/*---------------------------------------------------------------------------*/
#include "tools-utils.h"

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
/*---------------------------------------------------------------------------*/
#define BAUDRATE B115200

static speed_t b_rate = BAUDRATE;
/*---------------------------------------------------------------------------*/
#ifdef linux
#define MODEMDEVICE "/dev/ttyUSB0"
#else
#define MODEMDEVICE "/dev/com1"
#endif /* linux */
/*---------------------------------------------------------------------------*/
#define SLIP_END      0300
#define SLIP_ESC      0333
#define SLIP_ESC_END  0334
#define SLIP_ESC_ESC  0335

#define BUFSIZE         40
#define HCOLS           20
#define ICOLS           18

enum mode {
  MODE_START_DATE,
  MODE_DATE,
  MODE_START_TEXT,
  MODE_TEXT,
  MODE_INT,
  MODE_HEX,
  MODE_SLIP_AUTO,
  MODE_SLIP,
  MODE_SLIP_HIDE
};
/*---------------------------------------------------------------------------*/
#ifndef O_SYNC
#define O_SYNC 0
#endif

#define OPEN_FLAGS (O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC)
/*---------------------------------------------------------------------------*/
static unsigned char rxbuf[2048];
/*---------------------------------------------------------------------------*/
static int
usage(int result)
{
  /* Send the usage to stdout when explicitly requested (-h), but to stderr
     when it accompanies an error so it does not pollute piped serial output. */
  FILE *out = result == 0 ? stdout : stderr;
  fprintf(out, "Usage: serialdump [-x] [-s[on]] [-i] [-bSPEED] [-T[format]] [SERIALDEVICE]\n");
  fprintf(out, "       -x for hexadecimal output\n");
  fprintf(out, "       -i for decimal output\n");
  fprintf(out, "       -s for automatic SLIP mode\n");
  fprintf(out, "       -so for SLIP only mode (all data is SLIP packets)\n");
  fprintf(out, "       -sn to hide SLIP packages\n");
  fprintf(out, "       -T[format] to add time for each text line\n");
  fprintf(out, "         (see man page for strftime() for format description)\n");
  return result;
}
/*---------------------------------------------------------------------------*/
static void
print_hex_line(char *prefix, unsigned char *outbuf, int index)
{
  int i;

  printf("\r%s", prefix);
  for(i = 0; i < index; i++) {
    if((i % 4) == 0) {
      printf(" ");
    }
    printf("%02X", outbuf[i] & 0xFF);
  }
  printf("  ");
  for(i = index; i < HCOLS; i++) {
    if((i % 4) == 0) {
      printf(" ");
    }
    printf("  ");
  }
  for(i = 0; i < index; i++) {
    if(!isprint(outbuf[i])) {
      printf(".");
    } else {
      printf("%c", outbuf[i]);
    }
  }
}
/*---------------------------------------------------------------------------*/
static volatile sig_atomic_t should_exit = 0;
/*---------------------------------------------------------------------------*/
static void
sigint_handler(int sig)
{
  should_exit = 1;
}
/*---------------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
  struct sigaction sa;
  sa.sa_handler = sigint_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0; /* no SA_RESTART: let select() return EINTR so we can exit */
  sigaction(SIGINT, &sa, NULL);

  struct termios options;
  fd_set mask, smask;
  int fd;
  int baudrate = BUNKNOWN;
  char *device = MODEMDEVICE;
  char *timeformat = NULL;
  unsigned char buf[BUFSIZE];
  char timebuf[64];
  enum mode mode = MODE_START_TEXT;
  int nfound, flags = 0;
  unsigned char lastc = '\0';

  int index = 1;
  while(index < argc) {
    if(argv[index][0] == '-') {
      switch(argv[index][1]) {
      case 'b':
        baudrate = atoi(&argv[index][2]);
        break;
      case 'x':
        mode = MODE_HEX;
        break;
      case 'i':
        mode = MODE_INT;
        break;
      case 's':
        switch(argv[index][2]) {
        case 'n':
          mode = MODE_SLIP_HIDE;
          break;
        case 'o':
          mode = MODE_SLIP;
          break;
        default:
          mode = MODE_SLIP_AUTO;
          break;
        }
        break;
      case 'T':
        if(strlen(&argv[index][2]) == 0) {
          timeformat = "%Y-%m-%d %H:%M:%S";
        } else {
          timeformat = &argv[index][2];
        }
        mode = MODE_START_DATE;
        break;
      case 'h':
        return usage(EXIT_SUCCESS);
      default:
        fprintf(stderr, "unknown option '%c'\n", argv[index][1]);
        return usage(EXIT_FAILURE);
      }
      index++;
    } else {
      device = argv[index++];
      if(index < argc) {
        fprintf(stderr, "too many arguments\n");
        return usage(EXIT_FAILURE);
      }
    }
  }

  if(baudrate != BUNKNOWN) {
    b_rate = select_baudrate(baudrate);
    if(b_rate == 0) {
      fprintf(stderr, "unknown baudrate %d\n", baudrate);
      exit(EXIT_FAILURE);
    }
  }

  fprintf(stderr, "connecting to %s", device);

  fd = open(device, OPEN_FLAGS);

  if(fd < 0) {
    fprintf(stderr, "\n");
    perror("open");
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, " [OK]\n");

  if(fcntl(fd, F_SETFL, 0) < 0) {
    perror("could not set fcntl");
    exit(EXIT_FAILURE);
  }

  if(tcgetattr(fd, &options) < 0) {
    perror("could not get options");
    exit(EXIT_FAILURE);
  }

  cfsetispeed(&options, b_rate);
  cfsetospeed(&options, b_rate);

  /* Enable the receiver and set local mode */
  options.c_cflag |= (CLOCAL | CREAD);
  /* Mask the character size bits and turn off (odd) parity */
  options.c_cflag &= ~(CSIZE | PARENB | PARODD);
  /* Select 8 data bits */
  options.c_cflag |= CS8;

  /* Raw input */
  options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                       | INLCR | IGNCR | ICRNL | IXON);
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  /* Raw output */
  options.c_oflag &= ~OPOST;

  if(tcsetattr(fd, TCSANOW, &options) < 0) {
    perror("could not set options");
    exit(EXIT_FAILURE);
  }

  FD_ZERO(&mask);
  FD_SET(fd, &mask);
  /* Only watch stdin if it is a valid, open descriptor. A pre-closed fd 0
     would otherwise make select() fail with EBADF on every iteration. */
  if(fcntl(fileno(stdin), F_GETFD) != -1) {
    FD_SET(fileno(stdin), &mask);
  }

  index = 0;
  for(;;) {
    if(should_exit) {
      break;
    }
    smask = mask;
    nfound = select(FD_SETSIZE, &smask, (fd_set *)0, (fd_set *)0, (struct timeval *)0);
    if(nfound < 0) {
      if(errno == EINTR) {
        if(should_exit) {
          break;
        }
        fprintf(stderr, "interrupted system call\n");
        continue;
      }
      /* something is very wrong! */
      perror("select");
      exit(EXIT_FAILURE);
    }

    if(FD_ISSET(fileno(stdin), &smask)) {
      /* data from standard in */
      int n = read(fileno(stdin), buf, sizeof(buf));
      if(n < 0) {
        perror("could not read");
        exit(EXIT_FAILURE);
      } else if(n > 0) {
        int i;
        /* Write slowly, one byte at a time. The whole input is forwarded
           verbatim (including any terminating LF, which commands may need),
           and the per-byte delay gives slow serial devices time to keep up. */
        for(i = 0; i < n; i++) {
          if(write(fd, &buf[i], 1) <= 0) {
            perror("write");
            exit(EXIT_FAILURE);
          }
          fflush(NULL);
          usleep(6000);
        }
      } else {
        /* stdin reached EOF: stop watching it, but keep dumping the serial
           port so that, e.g., responses to piped-in commands are still shown. */
        FD_CLR(fileno(stdin), &mask);
      }
    }

    if(FD_ISSET(fd, &smask)) {
      int i, n = read(fd, buf, sizeof(buf));
      if(n < 0) {
        perror("could not read");
        exit(EXIT_FAILURE);
      }
      if(n == 0) {
        errno = EBADF;
        perror("serial device disconnected");
        exit(EXIT_FAILURE);
      }

      for(i = 0; i < n; i++) {
        switch(mode) {
        case MODE_START_TEXT:
        case MODE_TEXT:
          printf("%c", buf[i]);
          break;
        case MODE_START_DATE: {
          time_t t;
          t = time(&t);
          strftime(timebuf, sizeof(timebuf), timeformat, localtime(&t));
          printf("[%s] ", timebuf);
          mode = MODE_DATE;
        }
        /* fall through into MODE_DATE */
        case MODE_DATE:
          printf("%c", buf[i]);
          if(buf[i] == '\n') {
            mode = MODE_START_DATE;
          }
          break;
        case MODE_INT:
          printf("%03d ", buf[i]);
          if(++index >= ICOLS) {
            index = 0;
            printf("\n");
          }
          break;
        case MODE_HEX:
          rxbuf[index++] = buf[i];
          if(index >= HCOLS) {
            print_hex_line("", rxbuf, index);
            index = 0;
            printf("\n");
          }
          break;

        case MODE_SLIP_AUTO:
        case MODE_SLIP_HIDE:
          if(!flags && (buf[i] != SLIP_END)) {
            /* Not a SLIP packet? */
            printf("%c", buf[i]);
            break;
          }
        /* fall through to SLIP-only mode */
        case MODE_SLIP:
          switch(buf[i]) {
          case SLIP_ESC:
            lastc = SLIP_ESC;
            break;

          case SLIP_END:
            if(index > 0) {
              if(flags != 2 && mode != MODE_SLIP_HIDE) {
                /* not overflowed: show packet */
                print_hex_line("SLIP: ", rxbuf, index > HCOLS ? HCOLS : index);
                printf("\n");
              }
              lastc = '\0';
              index = 0;
              flags = 0;
            } else {
              flags = !flags;
            }
            break;

          default:
            if(lastc == SLIP_ESC) {
              lastc = '\0';

              /* Previous read byte was an escape byte, so this byte will be
                 interpreted differently from others. */
              switch(buf[i]) {
              case SLIP_ESC_END:
                buf[i] = SLIP_END;
                break;
              case SLIP_ESC_ESC:
                buf[i] = SLIP_ESC;
                break;
              }
            }

            rxbuf[index++] = buf[i];
            if(index >= (int)sizeof(rxbuf)) {
              fprintf(stderr, "**** slip overflow\n");
              index = 0;
              flags = 2;
            }
            break;
          }
          break;
        }
      }

      /* after processing, refresh the partial line for the hex output mode */
      if(index > 0 && mode == MODE_HEX) {
        print_hex_line("", rxbuf, index);
      }
      fflush(stdout);
    }
  }

  fflush(stdout);
  return EXIT_SUCCESS;
}
