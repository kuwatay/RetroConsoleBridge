#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    OWNER_NONE = 0,
    OWNER_USB,
    OWNER_WIFI,
} console_owner_t;

typedef enum {
    SESSION_ONLINE = 0,
    SESSION_COMMAND,
    SESSION_AUTH,
} session_mode_t;

typedef struct {
    console_owner_t owner;
    session_mode_t usb_mode;
    session_mode_t wifi_mode;
    uint64_t last_activity_us;
} console_state_t;

extern console_state_t g_console;

void console_init(void);
void console_task(void);
void console_on_wifi_connected(void);
void console_on_wifi_disconnected(void);
void console_write_usb(const char *s);
void console_write_wifi(const char *s);
const char *console_owner_name(console_owner_t o);
