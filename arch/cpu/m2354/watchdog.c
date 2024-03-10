#include "contiki.h"
#include "NuMicro.h"

void watchdog_init(void)
{
#if !defined(TRUSTZONE_NONSECURE)
	/* Enable WDT module clock */
	CLK_EnableModuleClock(WDT_MODULE);
	CLK_SetModuleClock(WDT_MODULE, CLK_CLKSEL1_WDTSEL_LIRC, 0);
#endif
}

void watchdog_start(void)
{
#if !defined(TRUSTZONE_NONSECURE)
	WDT_Open(WDT_TIMEOUT_2POW14, WDT_RESET_DELAY_18CLK, TRUE, TRUE);
#endif
}

void watchdog_periodic(void)
{
#if !defined(TRUSTZONE_NONSECURE)
	WDT_Close();
	WDT_Open(WDT_TIMEOUT_2POW14, WDT_RESET_DELAY_18CLK, TRUE, TRUE);
#endif
}

void watchdog_reboot(void)
{
#if !defined(TRUSTZONE_NONSECURE)
	WDT_Close();
	WDT_Open(WDT_TIMEOUT_2POW14, WDT_RESET_DELAY_18CLK, TRUE, FALSE);
	while (1);
#endif
}

