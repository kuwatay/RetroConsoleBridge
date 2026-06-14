#include "console.h"
#include "config.h"
#include "at.h"
#include "uart.h"
#include "network.h"
#include "pico/stdlib.h"
#include <stdio.h>

console_state_t g_console;

const char *console_owner_name(console_owner_t o) {
    switch (o) {
        case OWNER_USB: return "USB";
        case OWNER_WIFI: return "WIFI";
        default: return "NONE";
    }
}

void console_write_usb(const char *s) { printf("%s", s); }
void console_write_wifi(const char *s) { network_send_str(s); }

void console_init(void) {
    g_console.owner = OWNER_USB;
    g_console.usb_mode = SESSION_ONLINE;
    g_console.wifi_mode = SESSION_COMMAND;
    g_console.last_activity_us = time_us_64();
}

void console_on_wifi_connected(void) {
    if (g_config.console_policy == CONSOLE_USB) {
        g_console.wifi_mode = SESSION_COMMAND;
        network_send_str("BUSY\r\n");
        return;
    }
    g_console.owner = OWNER_WIFI;
    g_console.wifi_mode = SESSION_ONLINE;
    g_console.usb_mode = SESSION_COMMAND;
    if (g_config.notify) console_write_usb("\r\n[TELNET CONNECTED]\r\n[CONSOLE WIFI]\r\n");
}

void console_on_wifi_disconnected(void) {
    g_console.wifi_mode = SESSION_COMMAND;
    if (g_config.console_policy == CONSOLE_WIFI) {
        g_console.owner = OWNER_NONE;
    } else {
        g_console.owner = OWNER_USB;
        g_console.usb_mode = SESSION_ONLINE;
    }
    if (g_config.notify) console_write_usb("\r\n[TELNET DISCONNECTED]\r\n[CONSOLE USB]\r\n");
}

static void handle_usb_input(int ch) {
    if (g_console.owner == OWNER_USB && g_console.usb_mode == SESSION_ONLINE) {
        if (at_escape_feed(OWNER_USB, (char)ch)) return;
        rc_uart_write((uint8_t)ch);
    } else {
        at_feed(OWNER_USB, (char)ch);
    }
}

void console_task(void) {
    int ch;
    while ((ch = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
        handle_usb_input(ch);
    }

    uint8_t b;
    while (rc_uart_read(&b)) {
        if (g_console.owner == OWNER_USB) putchar_raw(b);
        else if (g_console.owner == OWNER_WIFI) network_send_byte(b);
    }
}
