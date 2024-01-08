#include "contiki.h"
#include "NuMicro.h"

void gpio_relay_on(void)
{
    PF7 = 0;    
    PF7 = 1;    
    clock_wait(200);
    PF7 = 0;    
}

void gpio_relay_off(void)
{
    PF6 = 0;    
    PF6 = 1;    
    clock_wait(200);
    PF6 = 0;    
}
