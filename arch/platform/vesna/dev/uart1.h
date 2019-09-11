#ifndef USART_H_
#define USART_H_

#include "vsnusart.h"
#include "sys/process.h"

#define BAUD2UBR(baud) baud


PROCESS_NAME(uart1_rx_process);

void uart1_init(unsigned long ubr);
void uart1_writeb(unsigned char c);
void uart1_set_input(int (*input)(unsigned char c));

#endif
