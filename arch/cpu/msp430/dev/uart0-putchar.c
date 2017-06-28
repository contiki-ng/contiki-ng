#include "dev/uart0.h"

/* In case of IPv4: putchar() is defined by the SLIP driver */
int
putchar(int c)
{
  uart0_writeb((char)c);
  return c;
}
