#include "NuMicro.h"
#include "contiki.h"
#include "timer.h"
#include "clk.h"

static volatile rtimer_clock_t g_ticks = 0;
static volatile rtimer_clock_t g_time = 0;

void rtimer_arch_init(void)
{
#if !defined(TRUSTZONE_NONSECURE)
	CLK_EnableModuleClock(TMR3_MODULE);
	CLK_SetModuleClock(TMR3_MODULE, CLK_CLKSEL1_TMR3SEL_PCLK1, 0);
#endif

	TIMER_Open(TIMER3, TIMER_PERIODIC_MODE, RTIMER_ARCH_SECOND);

	TIMER_EnableInt(TIMER3);
	NVIC_EnableIRQ(TMR3_IRQn);

	TIMER_Start(TIMER3);
}

void rtimer_arch_schedule(rtimer_clock_t t)
{
	g_time = t;
}

rtimer_clock_t rtimer_arch_now(void)
{
	return g_ticks;
}

void TMR3_IRQHandler(void)
{
	if (g_time && g_time <= g_ticks) {
		g_time = 0;
		rtimer_run_next();
	}
		
	g_ticks++;
	TIMER_ClearIntFlag(TIMER3);
}

