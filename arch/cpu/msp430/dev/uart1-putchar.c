#include "dev/uart1.h"

/* In case of IPv4: putchar() is defined by the SLIP driver */
int
putchar(int c)
{
  uart1_writeb((char)c);
  return c;
}
