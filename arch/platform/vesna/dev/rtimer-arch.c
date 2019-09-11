/*
    Real-time middleware for this architecture/platform
    Contiki requires to define functions:
      - rtimer_arch_now()
      - void rtimer_arch_init(void)
      - void rtimer_arch_schedule(rtimer_clock_t t);
      - RTIMER_ARCH_SECOND

    TODO: Timers & RTimers are usualy CPU specific.
        At some point move this to cpu/.
*/
#include <stdio.h>
#include <stdlib.h>
#include "contiki.h"
#include "sys/energest.h"
#include "sys/rtimer.h"

#include "rtimer-arch.h"

// STM libraries
#include "misc.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"

#include "sys/critical.h"

#define RTIMER_TIMx TIM5
#define RTIMER_IRQn TIM5_IRQn
#define RTIMER_APB1 RCC_APB1Periph_TIM5

/*
    Contiki(-ng) uses several timers. We implemented rtimer (r is for real-time)
    using TIM5 general purpose timer.

    TIM5 properties:
        - 16-bit up/down counter,
        - interrupt can be triggered on overflow or specific value,

    Our goal is to get >= 32kHz triggers (see tsch-slot-operation.c). We set goal to have 64kHz triggers.

    Internal clock of STM32 is drifting a lot - too much for precise TSCH operations. This presents a 
    problem, because our devices are mising the slots. So we have 2 options:

    1) When we are using SNR board, we can use AT86RF2xx oscilator clock as main clock for STM32, which has
       very low drift (configured in platform.c).

    2) When we are using ISMTV board, AT86RF2xx CLK pin is not connected anywhere. But we can use oscilator
       of CC1101 chip, which is connected to TIM5 Channel 3 (PA2 on STM32). So only TIM5 will have external 
       clock source, which is not drifting.
*/

void rtimer_arch_init(void) {

    // TIM5 clock enable
    RCC_APB1PeriphClockCmd(RTIMER_APB1, ENABLE);

#if (AT86RF2XX_BOARD_ISMTV_V1_0 || AT86RF2XX_BOARD_ISMTV_V1_1)
    TIM_TimeBaseInitTypeDef externalTimerInitStructure = {
        .TIM_Prescaler = 206 - 1,       // 0 = run @ 13.5MHz, 206 = run @ 65533.98 Hz
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = 65533, // upper bound value of counter (65536) (same as RTIMER_ARCH_SECOND)
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0, // Not available on TIM5 anyway
    };

    GPIO_InitTypeDef gpioInitStructure = {
        .GPIO_Pin = GPIO_Pin_2,
        .GPIO_Mode = GPIO_Mode_IN_FLOATING,
        .GPIO_Speed = GPIO_Speed_10MHz,
    };
    // Initialize timer
    TIM_TimeBaseInit(RTIMER_TIMx, &externalTimerInitStructure);

        // GPIO clock enable
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // Do we need this?

    // Initialize GPIO
    GPIO_Init(GPIOA, &gpioInitStructure);

    // Connect channel 3 and 2 to channel 1 via XOR gate
    TIM_SelectHallSensor(RTIMER_TIMx, ENABLE);
    
    // Setup timer trigger as external clock source
    TIM_TIxExternalClockConfig(RTIMER_TIMx, TIM_TIxExternalCLK1Source_TI1, TIM_ICPolarity_Falling, 0x0);
#else
    TIM_TimeBaseInitTypeDef timerInitStructure = {
        .TIM_Prescaler = 977 - 1,       // 0 = run @ 64MHz, 977 = run @ 65506 Hz
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = 65503,            // upper bound value of counter (65506) (same as RTIMER_ARCH_SECOND)
        .TIM_ClockDivision = TIM_CKD_DIV1,
        .TIM_RepetitionCounter = 0,     // Not available on TIM5 anyway
    };
    // Initialize timer
    TIM_TimeBaseInit(RTIMER_TIMx, &timerInitStructure);
#endif

    // Set initial value (value could be random at power on)
    TIM_SetCounter(RTIMER_TIMx, 0);

    // Disable ALL interrupts from TIM5
    TIM_ITConfig(RTIMER_TIMx, 0xFF, DISABLE);

    // Clear interrupt bit, if it was triggered for some reason
    TIM_ClearITPendingBit(RTIMER_TIMx, TIM_IT_CC1);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = RTIMER_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Enable timer
    TIM_Cmd(RTIMER_TIMx, ENABLE);
}

rtimer_clock_t
rtimer_arch_now(void)
{   
    return (rtimer_clock_t)TIM_GetCounter(RTIMER_TIMx);
}

void
rtimer_arch_schedule(rtimer_clock_t t)
{
    int_master_status_t status;

    status = critical_enter();

    TIM_ITConfig(RTIMER_TIMx, TIM_IT_CC1, DISABLE);
    TIM_ClearITPendingBit(RTIMER_TIMx, TIM_IT_CC1);

    TIM_OCInitTypeDef tim_oc1_init;
    tim_oc1_init.TIM_OCMode = TIM_OCMode_Timing;
    tim_oc1_init.TIM_OCPolarity = TIM_OCPolarity_High;
    tim_oc1_init.TIM_Pulse = (uint16_t)t; // To be compared against
    tim_oc1_init.TIM_OutputState = TIM_OutputState_Disable;

    TIM_OC1Init(RTIMER_TIMx, &tim_oc1_init);
    TIM_ITConfig(RTIMER_TIMx, TIM_IT_CC1, ENABLE);

    critical_exit(status);
}

void
contiki_rtimer_isr(void)
{
    // Comparator 1 trigger
    if (TIM_GetITStatus(RTIMER_TIMx, TIM_IT_CC1) != RESET) {
        TIM_ITConfig(RTIMER_TIMx, TIM_IT_CC1, DISABLE);
        TIM_ClearITPendingBit(RTIMER_TIMx, TIM_IT_CC1);

        rtimer_run_next();
    }
}


