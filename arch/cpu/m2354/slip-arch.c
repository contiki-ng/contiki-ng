#include "contiki.h"
#include "dev/slip.h"

#include "NuMicro.h"

void
slip_arch_writeb(uint8_t c)
{
    UART_Write(UART4, &c, 1);
}

