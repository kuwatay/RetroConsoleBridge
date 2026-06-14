#include "led.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_WIFI 10
#define LED_LINK 11
#define LED_TX   12
#define LED_RX   13

#define PULSE_MS 150

static absolute_time_t tx_until;
static absolute_time_t rx_until;

void led_init(void) {
    const uint pins[] = { LED_WIFI, LED_LINK, LED_TX, LED_RX };

    for (int i = 0; i < 4; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_OUT);
        gpio_put(pins[i], 0);
    }

    tx_until = get_absolute_time();
    rx_until = get_absolute_time();
}

void led_set_wifi(bool on) {
    gpio_put(LED_WIFI, on ? 1 : 0);
}

void led_set_link(bool on) {
    gpio_put(LED_LINK, on ? 1 : 0);
}

void led_pulse_tx(void) {
    gpio_put(LED_TX, 1);
    tx_until = make_timeout_time_ms(PULSE_MS);
}

void led_pulse_rx(void) {
    gpio_put(LED_RX, 1);
    rx_until = make_timeout_time_ms(PULSE_MS);
}

void led_task(void) {
    if (absolute_time_diff_us(get_absolute_time(), tx_until) <= 0) {
        gpio_put(LED_TX, 0);
    }

    if (absolute_time_diff_us(get_absolute_time(), rx_until) <= 0) {
        gpio_put(LED_RX, 0);
    }
}
