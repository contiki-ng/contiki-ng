#include "contiki.h"
#include "lib/dbg-io/dbg.h"

#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
int
puts(const char *str)
{
  dbg_send_bytes((unsigned char *)str, strlen(str));
  return dbg_putchar('\n');
}
/*---------------------------------------------------------------------------*/

