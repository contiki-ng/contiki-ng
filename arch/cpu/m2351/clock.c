#include "contiki.h"
#include "os/sys/mutex.h"
#include "NuMicro.h"

#include <stdint.h>

static uint32_t __vectors_in_ram[256] __attribute__((section("vtor_ram")));
static volatile uint32_t g_ticks = 0;
static volatile uint16_t g_tmr_ticks = 0;

static volatile mutex_t g_mutex = MUTEX_STATUS_UNLOCKED;
static volatile int mutex_counter = 0;

static void remap_vector(void)
{
	extern uint32_t __Vectors[];
	int i;

	for(i=0; i<256; i++)
	{
		__vectors_in_ram[i] = __Vectors[i];
	}
	__disable_irq();
	SCB->VTOR = (uint32_t)__vectors_in_ram;
	__DSB();
	__enable_irq();
}

void clock_init(void)
{
#if !defined(TRUSTZONE_NONSECURE)
	SYS_UnlockReg();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable HIRC clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Select HCLK clock source as HIRC and HCLK clock divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable HXT clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Wait for HXT clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Enable PLL */
    CLK->PLLCTL = CLK_PLLCTL_128MHz_HIRC;

    /* Waiting for PLL stable */
    CLK_WaitClockReady(CLK_STATUS_PLLSTB_Msk);

    /* Select HCLK clock source as PLL and HCLK source divider as 2 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(2));

//	/* Enable TIMER2 */
//	CLK_EnableModuleClock(TMR2_MODULE);
//	CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2SEL_PCLK1, 0);
#endif

	SystemCoreClockUpdate();

	remap_vector();

	SysTick_Config(CLK_GetCPUFreq() / CLOCK_SECOND);

//	TIMER_Open(TIMER2, TIMER_PERIODIC_MODE, 1000000);
//	NVIC_EnableIRQ(TMR2_IRQn);
}

clock_time_t clock_time(void)
{
	return g_ticks;
}

unsigned long clock_seconds(void)
{
	return g_ticks / CLOCK_SECOND;
}

void clock_wait(clock_time_t i)
{
	clock_time_t end = g_ticks + i + 1;
	while (g_ticks < end);
}

static void start_timer(void)
{
	mutex_try_lock(&g_mutex);
	mutex_counter++;
	if (mutex_counter == 1) {
		g_tmr_ticks = 0;
		TIMER_EnableInt(TIMER2);
		TIMER_Start(TIMER2);
	}
	mutex_unlock(&g_mutex);
}

static void end_timer(void)
{
	mutex_try_lock(&g_mutex);
	mutex_counter--;
	if (mutex_counter == 0) {
		TIMER_Stop(TIMER2);
		TIMER_DisableInt(TIMER2);
		g_tmr_ticks = 0;
	}
	mutex_unlock(&g_mutex);
}

void clock_delay_usec(uint16_t usec)
{
	start_timer();

	while (g_tmr_ticks < usec);

	end_timer();
}

void clock_delay(unsigned int i)
{
	clock_delay_usec(i);
}

void SysTick_Handler(void)
{
#ifdef TRUSTZONE_NONSECURE
	g_ticks++;
	if (etimer_pending())
		etimer_request_poll();
#endif
}

void TMR2_IRQHandler(void)
{
	g_tmr_ticks++;
	TIMER_ClearIntFlag(TIMER2);
}
