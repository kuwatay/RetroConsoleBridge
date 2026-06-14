#pragma once

#include <stdint.h>
#include <stdbool.h>

#define APP_NAME        "Pico Retro Console Bridge"
#define APP_VERSION     "0.1.0"

#define WIFI_SSID_MAX   32
#define WIFI_PASS_MAX   64
#define TELNET_PASS_MAX 32

#define DEFAULT_UART_BAUD 115200
#define DEFAULT_TCP_PORT  23
#define DEFAULT_GUARD_MS  1000

// RC2014 TTL UART pins. Change if needed.
#define RC_UART_ID       uart0
#define RC_UART_TX_PIN   0
#define RC_UART_RX_PIN   1

// Flash config: last 4KB sector of flash.
#define CONFIG_FLASH_SECTOR_SIZE 4096

typedef enum {
    TERM_RAW = 0,
    TERM_TELNET = 1,
} term_mode_t;

typedef enum {
    CONSOLE_AUTO = 0,
    CONSOLE_USB = 1,
    CONSOLE_WIFI = 2,
} console_policy_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    char wifi_ssid[WIFI_SSID_MAX + 1];
    char wifi_pass[WIFI_PASS_MAX + 1];
    char telnet_password[TELNET_PASS_MAX + 1]; // v0: plaintext for simplicity; hash later.
    uint32_t uart_baud;
    uint16_t tcp_port;
    uint8_t auto_answer;       // S0
    uint16_t guard_time_ms;    // S12
    bool echo;                 // E
    bool quiet;                // Q
    bool notify;
    term_mode_t term_mode;     // ATTERM
    console_policy_t console_policy; // ATCONSOLE
} app_config_t;

extern app_config_t g_config;

void config_set_defaults(app_config_t *cfg);
void config_load(app_config_t *cfg);
bool config_save(const app_config_t *cfg);
void config_factory_reset(void);

const char *term_mode_name(term_mode_t m);
const char *console_policy_name(console_policy_t p);
