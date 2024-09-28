#include "contiki.h"
#include "NuMicro.h"
#include "platform_secure.h"

int dbg_putchar(int c)
{
	if (c == '\n') {
		while(UART_CONSOLE->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
		UART_CONSOLE->DAT = '\r';
	}

	while(UART_CONSOLE->FIFOSTS & UART_FIFOSTS_TXFULL_Msk);
	UART_CONSOLE->DAT = c;

	return c;
}

unsigned int dbg_send_bytes(const unsigned char *ptr, unsigned int len)
{
	int i = len;

	while (i--)
		dbg_putchar(*ptr++);

	return len;
}

