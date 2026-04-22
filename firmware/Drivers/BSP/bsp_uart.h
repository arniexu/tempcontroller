#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>

void bsp_uart_init(void);
bool bsp_uart_read_line(char *buf, unsigned int buf_size);
void bsp_uart_write(const char *text);

#endif
