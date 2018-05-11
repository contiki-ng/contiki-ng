/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/uart1.h"

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
    uart1_writeb(SLIP_END);
    uart1_writeb('\r');     /* Type debug line == '\r' */
    debug_frame = 1;
  }
#endif /* SLIP_ARCH_CONF_ENABLED */

  /* Need to also print '\n' because for example COOJA will not show
     any output before line end */
  uart1_writeb((char)c);

#if SLIP_ARCH_CONF_ENABLED
  /*
   * Line buffered output, a newline marks the end of debug output and
   * implicitly flushes debug output.
   */
  if(c == '\n') {
    uart1_writeb(SLIP_END);
    debug_frame = 0;
  }
#endif /* SLIP_ARCH_CONF_ENABLED */
  return c;
}
/*---------------------------------------------------------------------------*/
