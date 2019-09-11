#include "contiki-conf.h"
#include "dev/uart1.h"
#include "sys/process.h"

#include "vsn.h"
#include "vsnusart.h"
#include "vsnledind.h"

#include <stdio.h>

// Pointer to function, which handles bytes.
// There can be only one function handling incoming data.
// Most often this will be tun/slip and serial handlers.
static int (*input_handler)(unsigned char c) = NULL;


PROCESS(uart1_rx_process, "UART1 Rx process");


void uart1_init(unsigned long ubr) {
	// Initialize UART1 with vesna-drivers
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = ubr;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    vsnUSART_init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	// Disable buffering
	//setvbuf(stdout, NULL, _IONBF, 0);

	// Clear input handler
	input_handler = NULL;
}

void uart1_writeb(unsigned char c) {
	// Send individual charachers over the UART1
    vsnUSART_write(USART1, (const char *)&c, 1);
}

void uart1_set_input(int (*input) (unsigned char c)) {
    input_handler = input;
}


PROCESS_THREAD(uart1_rx_process, ev, data)
{
	unsigned char buf;

	PROCESS_BEGIN();

	// This loop will check UART1 buffer for incoming data (characters), then
	// forward it to input_handler function.
	while (1) {
		// A while loop would hang the device. Contiki's macro PROCESS_PAUSE()
		// transfers control to other processes and will return to this one
		// after some time. This is a "preemptive" strategy of proto-threads.
		PROCESS_PAUSE();

		if (input_handler != NULL) {
			while (vsnUSART_read(USART1, (char *)&buf, 1)) {
				input_handler(buf);
			}
		}
	}

	PROCESS_END();
}
