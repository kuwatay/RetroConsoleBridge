#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"
#include "config.h"
#include "uart.h"
#include "console.h"
#include "at.h"
#include "network.h"
#include "led.h"
#include <stdio.h>

int main(void) {
    stdio_init_all();
    sleep_ms(1200); // Give USB CDC time to enumerate.

    config_load(&g_config);
    rc_uart_init(g_config.uart_baud);
    at_init();
    led_init();
    console_init();
    network_init();

    printf("\r\n%s %s\r\n", APP_NAME, APP_VERSION);
    printf("USB-CDC online. Type +++ then AT&V.\r\n");

    if (g_config.wifi_ssid[0]) {
        printf("Connecting WiFi SSID: %s\r\n", g_config.wifi_ssid);
        if (network_connect()) printf("WiFi connected: %s\r\n", network_ip_string());
        else printf("WiFi connect failed. Use USB AT$SSID/AT$PASS.\r\n");
    }

    while (true) {
        console_task();
        network_task();
	led_task();
        tight_loop_contents();
    }
}
