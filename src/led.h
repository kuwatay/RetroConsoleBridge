#pragma once
#include <stdbool.h>

void led_init(void);
void led_task(void);

void led_set_wifi(bool on);
void led_set_link(bool on);

void led_pulse_tx(void);
void led_pulse_rx(void);
