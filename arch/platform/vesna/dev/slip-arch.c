#include "contiki.h"
#include "dev/slip.h"
#include "dev/uart1.h"
#include "vesna-conf.h"


void
slip_arch_writeb(unsigned char c)
{
    uart1_writeb(c);
}


void
slip_arch_init(void)
{
    uart1_set_input(slip_input_byte);
}

