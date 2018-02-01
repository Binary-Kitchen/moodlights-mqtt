#ifndef UART_H
#define UART_H

#include <stdbool.h>

extern void uart_init(void);

extern unsigned char *uart_pwm_buffer;
extern bool uart_data_rdy;

#endif /* UART_H */
