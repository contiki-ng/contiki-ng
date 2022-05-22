/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/uart0.h"

#include <string.h>
/*---------------------------------------------------------------------------*/
#define SLIP_END     0300
#undef putchar
/*---------------------------------------------------------------------------*/
int
putchar(int c)
{
#if SLIP_ARCH_CONF_ENABLED
  static char debug_frame = 0;

  if(!debug_frame) {            /* Start of debug output */
    uart0_writeb(SLIP_END);
    uart0_writeb('\r');     /* Type debug line == '\r' */
    debug_frame = 1;
  }
#endif /* SLIP_ARCH_CONF_ENABLED */

  /* Need to also print '\n' because for example COOJA will not show
     any output before line end */
  uart0_writeb((char)c);

#if SLIP_ARCH_CONF_ENABLED
  /*
   * Line buffered output, a newline marks the end of debug output and
   * implicitly flushes debug output.
   */
  if(c == '\n') {
    uart0_writeb(SLIP_END);
    debug_frame = 0;
  }
#endif /* SLIP_ARCH_CONF_ENABLED */
  return c;
}

#if defined(__GNUC__) && (__GNUC__ >= 9)
/* The printf() in newlib in GCC 9 from Texas Instruments uses the
 * "TI C I/O" protocol which is not implemented in GDB. The user manual
 * suggests overriding write() to redirect printf() output. */
int
write(int fd, const char *buf, int len)
{
  int i = 0;
  for(; i < len && buf[i]; i++) {
    putchar(buf[i]);
  }
  return i;
}
#endif
/*---------------------------------------------------------------------------*/
