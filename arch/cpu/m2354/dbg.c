#include "contiki.h"
#include "NuMicro.h"

int dbg_putchar(int c)
{
	if (c == '\n') {
		while(UART0->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
		UART0->DAT = '\r';
	}

	while(UART0->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
	UART0->DAT = c;

	return c;
}

unsigned int dbg_send_bytes(const unsigned char *ptr, unsigned int len)
{
	int i = len;

	while (i--)
		dbg_putchar(*ptr++);

	return len;
}

