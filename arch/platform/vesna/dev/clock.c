/**
 * \file clock.c
 * \brief Hardware-dependent part of the clock module.
 *
 * Provides functionality for managing timers in Contiki.
 *
 * \author: Zoltan Padrah
 * \date: 01.12.2010
 *
 * copyright (c) SensorLab, Jozef Stefan Institute
 */

#include <sys/clock.h>
#include <sys/cc.h>
#include <sys/etimer.h>

/**
 * tick count since the system start
 */
static volatile clock_time_t current_tick_count = 0;

/**
 * Number of seconds since system start
 */
static volatile unsigned long current_seconds = 0;

/**
 * Number of ticks remaining from a delay
 */
static volatile unsigned long delay_remaining = 0;

/**
 * Initialize clock module
 */
void
clock_init(void)
{
}

/**
 *
 * @return current time in clock_time_t units
 */
clock_time_t
clock_time()
{
	return current_tick_count;
}

/**
 * Delay for a given amount of ticks
 * @param delay the number of tick to wait before the funtion returns
 */
void
clock_delay(unsigned int delay)
{
	delay_remaining = delay;
	while(delay_remaining > 0){
		// debug_str("entering standby\n");
		// PWR_EnterSTANDBYMode();
		// debug_str("exited standby\n");
	}
}


/**
 *
 * @return the current time, in seconds
 */
unsigned long
clock_seconds(void)
{
	return current_seconds;
}

/**
 * Function called by the SysTick handler, once in a millisecond.
 */
void
clock_interrupt_handler()
{
	// advance in time
	current_tick_count++;
	current_seconds = current_tick_count / CLOCK_CONF_SECOND;
	// update delay
	if(delay_remaining > 0) delay_remaining--;
	// poll timers
	if(etimer_pending() && (etimer_next_expiration_time() >= current_tick_count)) {
        etimer_request_poll();
	}
}

