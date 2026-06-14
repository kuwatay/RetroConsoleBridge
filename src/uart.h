#pragma once
#include <stdint.h>
#include <stdbool.h>

void rc_uart_init(uint32_t baud);
void rc_uart_set_baud(uint32_t baud);
bool rc_uart_read(uint8_t *ch);
bool rc_uart_write(uint8_t ch);
void rc_uart_task(void);
