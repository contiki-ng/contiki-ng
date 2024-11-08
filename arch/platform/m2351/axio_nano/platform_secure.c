/*
 * Copyright (C) 2024 Kyung-mo Kim <kkim@securityplatform.co.kr>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup m2351-platforms
 * @{
 *
 * \file
 *      Platform implementation for m2351
 * \author
 *      Kyung-mo Kim <kkim@securityplatform.co.kr>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "NuMicro.h"
#include "platform_secure.h"
#ifdef TRUSTZONE_SECURE

/*
 * CONSOLE: UART2, (RX, TX) = (PB0, PB1)
 * LED    : PA2,   ON = 1
 */
void uart2_clock_init(void)
{
    /* Enable UART2 module clock */
    CLK_EnableModuleClock(UART2_MODULE);

    /* Select UART2 module clock source as HIRC and UART2 module clock divider as 1 */
    CLK_SetModuleClock(UART2_MODULE, CLK_CLKSEL3_UART2SEL_HIRC, CLK_CLKDIV4_UART2(1));

    /* Set multi-function pins for UART2 RXD and TXD */
    SYS->GPB_MFPL = (SYS->GPB_MFPL & (~(UART2_RXD_PB0_Msk | UART2_TXD_PB1_Msk)))
            | UART2_RXD_PB0 | UART2_TXD_PB1;
}

void led_amber_init(void)
{
    GPIO_SetMode(PA, BIT2, GPIO_MODE_OUTPUT);
}

void led_amber_on(void)
{
    PA2 = 1;
}

void peri_init(void)
{
    uart2_clock_init();
    UART_Open(UART_CONSOLE, 115200);

    led_amber_init();
    led_amber_on();
}

void platform_init_stage_secure(void)
{
    clock_init();
    peri_init();
}
#endif

/**
 * @}
 */
