#include "uart.h"
#include "config.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

void rc_uart_init(uint32_t baud) {
    uart_init(RC_UART_ID, baud);
    gpio_set_function(RC_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RC_UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(RC_UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(RC_UART_ID, true);
}

void rc_uart_set_baud(uint32_t baud) {
    uart_set_baudrate(RC_UART_ID, baud);
}

bool rc_uart_read(uint8_t *ch) {
    if (!uart_is_readable(RC_UART_ID)) return false;
    *ch = (uint8_t)uart_getc(RC_UART_ID);
    return true;
}

bool rc_uart_write(uint8_t ch) {
    if (!uart_is_writable(RC_UART_ID)) return false;
    uart_putc_raw(RC_UART_ID, ch);
    return true;
}

void rc_uart_task(void) {
    // Reserved for future IRQ/ring-buffer implementation.
}
