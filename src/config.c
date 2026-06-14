#include "config.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#define CONFIG_MAGIC   0x52434231u // "RCB1"
#define CONFIG_VERSION 1u

app_config_t g_config;

static uint32_t flash_target_offset(void) {
    extern char __flash_binary_end;
    (void)__flash_binary_end;
    return PICO_FLASH_SIZE_BYTES - CONFIG_FLASH_SECTOR_SIZE;
}

const char *term_mode_name(term_mode_t m) {
    return m == TERM_TELNET ? "TELNET" : "RAW";
}

const char *console_policy_name(console_policy_t p) {
    switch (p) {
        case CONSOLE_USB: return "USB";
        case CONSOLE_WIFI: return "WIFI";
        default: return "AUTO";
    }
}

void config_set_defaults(app_config_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->magic = CONFIG_MAGIC;
    cfg->version = CONFIG_VERSION;
    cfg->uart_baud = DEFAULT_UART_BAUD;
    cfg->tcp_port = DEFAULT_TCP_PORT;
    cfg->auto_answer = 1;
    cfg->guard_time_ms = DEFAULT_GUARD_MS;
    cfg->echo = true;
    cfg->quiet = false;
    cfg->notify = true;
    cfg->term_mode = TERM_RAW;
    cfg->console_policy = CONSOLE_AUTO;
}

void config_load(app_config_t *cfg) {
    const app_config_t *stored = (const app_config_t *)(XIP_BASE + flash_target_offset());
    if (stored->magic == CONFIG_MAGIC && stored->version == CONFIG_VERSION) {
        memcpy(cfg, stored, sizeof(*cfg));
        cfg->wifi_ssid[WIFI_SSID_MAX] = '\0';
        cfg->wifi_pass[WIFI_PASS_MAX] = '\0';
        cfg->telnet_password[TELNET_PASS_MAX] = '\0';
    } else {
        config_set_defaults(cfg);
    }
}

bool config_save(const app_config_t *cfg) {
    uint8_t sector[CONFIG_FLASH_SECTOR_SIZE];
    memset(sector, 0xFF, sizeof(sector));
    memcpy(sector, cfg, sizeof(*cfg));

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_target_offset(), CONFIG_FLASH_SECTOR_SIZE);
    flash_range_program(flash_target_offset(), sector, CONFIG_FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
    return true;
}

void config_factory_reset(void) {
    config_set_defaults(&g_config);
    config_save(&g_config);
}
