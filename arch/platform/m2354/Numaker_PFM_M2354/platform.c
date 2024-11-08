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
 * \addtogroup m2354-platforms
 * @{
 *
 * \file
 *      Platform implementation for m2354
 * \author
 *      Kyung-mo Kim <kkim@securityplatform.co.kr>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "net/ipv6/uip.h"
#include "dev/slip.h"
#include "dev/serial-line.h"
#include "sensors.h"
#include "NuMicro.h"

#ifdef TRUSTZONE_NONSECURE 
#include "secure_context.h"
#endif

#if NETSTACK_CONF_WITH_IPV6
#include "net/ipv6/uip-ds6.h"
#endif /* NETSTACK_CONF_WITH_IPV6 */

/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "M2354"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/

#ifdef PLATFORM_CONF_MAC_ADDR
static uint8_t mac_addr[] = PLATFORM_CONF_MAC_ADDR;
#else /* PLATFORM_CONF_MAC_ADDR */
static uint8_t mac_addr[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
#endif /* PLATFORM_CONF_MAC_ADDR */

SENSORS_SENSOR(dummy_sensor, "dummy", NULL, NULL, NULL);
SENSORS(&dummy_sensor);

static void
set_lladdr(void)
{
  linkaddr_t addr;

  memset(&addr, 0, sizeof(linkaddr_t));
#if NETSTACK_CONF_WITH_IPV6
  memcpy(addr.u8, mac_addr, sizeof(addr.u8));
#else
  int i;
  for(i = 0; i < sizeof(linkaddr_t); ++i) {
    addr.u8[i] = mac_addr[7 - i];
  }
#endif
  linkaddr_set_node_addr(&addr);
}

/*---------------------------------------------------------------------------*/
#if 0
#if NETSTACK_CONF_WITH_IPV6
static void
set_route(void)
{
  uip_ipaddr_t ipaddr;

  memset(&ipaddr, 0, sizeof(ipaddr));
  uip_create_linklocal_prefix(&ipaddr);
  ipaddr.u8[15] = 1;
  uip_ds6_defrt_add(&ipaddr, 0);

  LOG_INFO("Added route ");
  LOG_INFO_6ADDR(&ipaddr);
  LOG_INFO_("\n");
}
#endif
#endif

void
platform_init_stage_one(void)
{
}
/*---------------------------------------------------------------------------*/

void UART0_IRQHandler(void)
{
	uint8_t c = 0xff;
	uint32_t int_sts = UART0->INTSTS;

	if (int_sts & UART_INTSTS_RDAINT_Msk) {
		/* Get all the input characters */
		while (UART_IS_RX_READY(UART0)) {
			/* Get the character from UART Buffer */
			c = (uint8_t)UART_READ(UART0);
			printf("%c", c);
			if (c == '\n')
				printf("\r");
			else if (c == '\r')
				printf("\n");
			fflush(stdout);
			serial_line_input_byte(c);
		}

	}

	/* Handle transmission error */
	if (UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk |
				UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
		UART0->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk |
				UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk);
}

__attribute__((weak)) int meter_if_serial_input_byte(unsigned char c)
{
    (void)c;
    return 0;
}

void UART1_IRQHandler(void)
{
	uint8_t c = 0xff;
	uint32_t int_sts = UART1->INTSTS;

	if (int_sts & UART_INTSTS_RDAINT_Msk) {
		/* Get all the input characters */
		while (UART_IS_RX_READY(UART1)) {
			/* Get the character from UART Buffer */
			c = (uint8_t)UART_READ(UART1);
#if 0
			printf("UART1 INT: %02x\r\n", c);
#endif
			//slip_input_byte(c);
                        meter_if_serial_input_byte(c);
		}
	}

	/* Handle transmission error */
	if (UART1->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk |
				UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
		UART1->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk |
				UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk);
}

void UART4_IRQHandler(void)
{
	uint8_t c = 0xff;
	uint32_t int_sts = UART4->INTSTS;

	if (int_sts & UART_INTSTS_RDAINT_Msk) {
		/* Get all the input characters */
		while (UART_IS_RX_READY(UART4)) {
			/* Get the character from UART Buffer */
			c = (uint8_t)UART_READ(UART4);
#if 0
			printf("%02x\n", c);
#endif
			slip_input_byte(c);
		}
	}

	/* Handle transmission error */
	if (UART4->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk |
				UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
		UART4->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk |
				UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk);
}

static void uart0_init(void)
{
#ifndef TRUSTZONE_NONSECURE 
	/* Enable UART0 module clock */
	CLK_EnableModuleClock(UART0_MODULE);

	/* Select UART0 module clock source as HIRC and UART0 module clock divider as 1 */
	CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL2_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

	/* Set multi-function pins for UART0 RXD and TXD */
	SYS->GPA_MFPL = (SYS->GPA_MFPL & (~(UART0_RXD_PA6_Msk | UART0_TXD_PA7_Msk)))
		| UART0_RXD_PA6 | UART0_TXD_PA7;
#endif
	/* Configure UART0: 115200, 8-bit word, no parity bit, 1 stop bit. */
	UART_Open(UART0, 115200);

	NVIC_EnableIRQ(UART0_IRQn);
	UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk);
}

static void uart1_init(void)
{
	/* Configure UART1: 115200, 8-bit word, no parity bit, 1 stop bit. */
	UART_Open(UART1, 115200);

	NVIC_EnableIRQ(UART1_IRQn);
	UART_EnableInt(UART1, UART_INTEN_RDAIEN_Msk);
}

static void uart4_init(void)
{
#ifndef TRUSTZONE_NONSECURE 
	/* Enable UART4 module clock */
	CLK_EnableModuleClock(UART4_MODULE);

	/* Select UART4 module clock source as HIRC and UART4 module clock divider as 1 */
	CLK_SetModuleClock(UART4_MODULE, CLK_CLKSEL3_UART4SEL_HIRC, CLK_CLKDIV4_UART4(1));

	/* Set multi-function pins for UART0 RXD and TXD */
	SYS->GPA_MFPH = (SYS->GPA_MFPH & (~(UART4_RXD_PA13_Msk | UART4_TXD_PA12_Msk)))
		| UART4_RXD_PA13 | UART4_TXD_PA12;
#endif
	/* Configure UART4: 9600, 8-bit word, no parity bit, 1 stop bit. */
	UART_Open(UART4, 9600);

	NVIC_EnableIRQ(UART4_IRQn);
	UART_EnableInt(UART4, UART_INTEN_RDAIEN_Msk);
}

void platform_init_stage_two(void)
{
	uart0_init();
	uart1_init();
	uart4_init();
	serial_line_init();
    set_lladdr();
}
/*---------------------------------------------------------------------------*/
void SVC_Handler(void)
{
#ifdef TRUSTZONE_NONSECURE 
  SecureContextHandle_t handle;

  SecureContext_Init();
  handle = SecureContext_AllocateContext(4 * 1024);
  SecureContext_LoadContext(handle);
#endif
}

static void setup_secure_stack(void)
{
	__asm volatile ("svc 0");
}

void
platform_init_stage_three(void)
{
	setup_secure_stack();
//    set_route();
	process_start(&slip_process, NULL);

}
/*---------------------------------------------------------------------------*/
void
platform_idle()
{
#if 0
  lpm_drop();
#endif
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
