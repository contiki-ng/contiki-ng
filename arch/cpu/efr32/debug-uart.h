#ifndef DEBUG_UART_H_
#define DEBUG_UART_H_

void debug_uart_set_input_handler(int (* handler)(unsigned char c));
void debug_uart_init(void);

#endif /* DEBUG_UART_H_ */
