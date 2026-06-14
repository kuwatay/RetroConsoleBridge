#include "at.h"
#include "config.h"
#include "console.h"
#include "uart.h"
#include "network.h"
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct {
    char line[160];
    int len;
    int plus_count;
    uint64_t last_char_us;
} at_state_t;

static at_state_t usb_at, wifi_at;

static at_state_t *state_for(console_owner_t src) {
    return src == OWNER_WIFI ? &wifi_at : &usb_at;
}

void at_reply(console_owner_t dst, const char *s) {
    if (dst == OWNER_WIFI) console_write_wifi(s);
    else console_write_usb(s);
}

static void ok(console_owner_t d) { if (!g_config.quiet) at_reply(d, "OK\r\n"); }
static void error(console_owner_t d) { if (!g_config.quiet) at_reply(d, "ERROR\r\n"); }

static bool starts_with(const char *s, const char *p) {
    return strncmp(s, p, strlen(p)) == 0;
}

static void uppercase(char *s) {
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}


static const char *wifi_status_string(void) {
    return network_is_connected() ? "CONNECTED TO WIFI" : "NOT CONNECTED";
}

static const char *call_status_string(void) {
    return network_client_connected() ? "CONNECTED" : "NOT CONNECTED";
}

static void show_ati_basic(console_owner_t d) {
    at_reply(d, "\r\n" APP_NAME "\r\nVersion " APP_VERSION "\r\n\r\n");
    ok(d);
}

static void show_ati4(console_owner_t d) {
    char buf[768];
    snprintf(buf, sizeof(buf),
        "\r\n%s\r\n"
        "Version....: %s\r\n"
        "Build......: %s %s\r\n"
        "Baud.......: %lu\r\n"
        "WiFi status: %s\r\n"
        "SSID.......: %s\r\n"
        "RSSI.......: %d dBm\r\n"
        "MAC address: %s\r\n"
        "IP address.: %s\r\n"
        "Gateway....: %s\r\n"
        "Subnet mask: %s\r\n"
        "Server port: %u\r\n"
        "Term mode..: %s\r\n"
        "Console....: %s\r\n"
        "Owner......: %s\r\n"
        "Password...: %s\r\n"
        "Bytes in...: %lu\r\n"
        "Bytes out..: %lu\r\n"
        "Call status: %s\r\n",
        APP_NAME,
        APP_VERSION,
        __DATE__, __TIME__,
        (unsigned long)g_config.uart_baud,
        wifi_status_string(),
        g_config.wifi_ssid[0] ? g_config.wifi_ssid : "(not set)",
        network_rssi_dbm(),
        network_mac_string(),
        network_ip_string(),
        network_gateway_string(),
        network_netmask_string(),
        g_config.tcp_port,
        term_mode_name(g_config.term_mode),
        console_policy_name(g_config.console_policy),
        console_owner_name(g_console.owner),
        g_config.telnet_password[0] ? "SET" : "NOT SET",
        (unsigned long)network_bytes_in(),
        (unsigned long)network_bytes_out(),
        call_status_string());
    at_reply(d, buf);
    ok(d);
}

static void show_atv(console_owner_t d) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "\r\n%s\r\n"
        "Firmware : %s\r\n\r\n"
        "WiFi\r\n"
        "  SSID       : %s\r\n"
        "  Status     : %s\r\n"
        "  IP Address : %s\r\n\r\n"
        "Network\r\n"
        "  Port       : %u\r\n\r\n"
        "Terminal\r\n"
        "  Mode       : %s\r\n\r\n"
        "Console\r\n"
        "  Policy     : %s\r\n"
        "  Owner      : %s\r\n\r\n"
        "UART\r\n"
        "  Baud       : %lu\r\n"
        "  Format     : 8N1\r\n\r\n"
        "Security\r\n"
        "  Telnet Password : %s\r\n\r\n"
        "Modem\r\n"
        "  AutoAnswer : %u\r\n"
        "  GuardTime  : %u ms\r\n"
        "  Notify     : %s\r\n\r\n",
        APP_NAME, APP_VERSION,
        g_config.wifi_ssid[0] ? g_config.wifi_ssid : "(not set)",
        network_is_connected() ? "Connected" : "Disconnected",
        network_ip_string(),
        g_config.tcp_port,
        term_mode_name(g_config.term_mode),
        console_policy_name(g_config.console_policy),
        console_owner_name(g_console.owner),
        (unsigned long)g_config.uart_baud,
        g_config.telnet_password[0] ? "Set" : "Not set",
        g_config.auto_answer,
        g_config.guard_time_ms,
        g_config.notify ? "ON" : "OFF");
    at_reply(d, buf); ok(d);
}

static void show_atv1(console_owner_t d) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "\r\nACTIVE PROFILE:\r\n\r\n"
        "E%d Q%d V1 X4\r\n\r\n"
        "S0=%u\r\nS12=%u\r\n\r\n"
        "SSID=%s\r\nIP=%s\r\n\r\n"
        "PORT=%u\r\nTERM=%s\r\n\r\n"
        "CONSOLE=%s\r\nOWNER=%s\r\n\r\n"
        "BAUD=%lu\r\nFORMAT=8N1\r\nPASSWORD=%s\r\n\r\n",
        g_config.echo ? 1 : 0, g_config.quiet ? 1 : 0,
        g_config.auto_answer, g_config.guard_time_ms,
        g_config.wifi_ssid[0] ? g_config.wifi_ssid : "",
        network_ip_string(),
        g_config.tcp_port, term_mode_name(g_config.term_mode),
        console_policy_name(g_config.console_policy), console_owner_name(g_console.owner),
        (unsigned long)g_config.uart_baud,
        g_config.telnet_password[0] ? "SET" : "NONE");
    at_reply(d, buf); ok(d);
}


static void show_help(console_owner_t d) {
    static const char *help[] = {
        "Help..........: AT?",
        "Info..........: ATI",
        "Detailed info.: ATI4",
        "Show settings.: AT&V",
        "Show profile..: AT&V1",
        "Save settings.: AT&W",
        "Fact. defaults: AT&F",
        "Reset modem...: ATZ",
        "Online mode...: ATO",
        "Hang up.......: ATH",
        "WiFi SSID.....: AT$SSID=ssid",
        "WiFi password.: AT$PASS=password",
        "WiFi connect..: ATC1",
        "WiFi disconnect: ATC0",
        "IP address....: ATIP",
        "Serial speed..: AT$SB=n",
        "Server port...: AT$SP=n",
        "Terminal mode.: ATTERM=RAW|TELNET",
        "Console mode..: ATCONSOLE=AUTO|USB|WIFI",
        "Telnet passwd.: ATPASSWD=password",
        "Auto answer...: ATS0=n",
        "Guard time....: ATS12=n",
        "Notifications.: ATNOTIFY=0|1",
        "",
        "Query most commands followed by '?'",
        "e.g. AT$SSID?, ATTERM?, ATCONSOLE?",
    };

    at_reply(d, "\r\nAT Command Summary:\r\n\r\n");

    const int n = (int)(sizeof(help) / sizeof(help[0]));
    for (int i = 0; i < n; ) {
        if (help[i][0] == '\0') {
            at_reply(d, "\r\n");
            i++;
            continue;
        }

        char line[96];
        if ((i + 1) < n && help[i + 1][0] != '\0') {
            snprintf(line, sizeof(line), "%-34s %s\r\n", help[i], help[i + 1]);
            i += 2;
        } else {
            snprintf(line, sizeof(line), "%s\r\n", help[i]);
            i++;
        }
        at_reply(d, line);
    }

    at_reply(d, "\r\n");
    ok(d);
}

static void handle_command(console_owner_t d, char *line) {
    while (*line == ' ') line++;
    if (starts_with(line, "AT")) line += 2;
    else { error(d); return; }

    char uline[160];
    strncpy(uline, line, sizeof(uline)-1); uline[sizeof(uline)-1] = 0;
    uppercase(uline);

    if (*uline == 0) { ok(d); return; }
    if (!strcmp(uline, "I") || !strcmp(uline, "I0")) { show_ati_basic(d); return; }
    if (!strcmp(uline, "I4")) { show_ati4(d); return; }
    if (!strcmp(uline, "?")) { show_help(d); return; }
    if (!strcmp(uline, "&V") || !strcmp(uline, "&V0")) { show_atv(d); return; }
    if (!strcmp(uline, "&V1")) { show_atv1(d); return; }
    if (!strcmp(uline, "&W")) { config_save(&g_config); ok(d); return; }
    if (!strcmp(uline, "&F")) { config_set_defaults(&g_config); ok(d); return; }
    if (!strcmp(uline, "Z")) { ok(d); sleep_ms(100); watchdog_reboot(0,0,0); return; }
    if (!strcmp(uline, "O")) {
        if (d == OWNER_WIFI && g_console.owner == OWNER_WIFI) g_console.wifi_mode = SESSION_ONLINE;
        if (d == OWNER_USB && g_console.owner == OWNER_USB) g_console.usb_mode = SESSION_ONLINE;
        ok(d); return;
    }
    if (!strcmp(uline, "H")) { network_close(); ok(d); return; }
    if (!strcmp(uline, "C1")) { network_connect(); ok(d); return; }
    if (!strcmp(uline, "C0")) { network_disconnect(); ok(d); return; }
    if (!strcmp(uline, "IP?")) { at_reply(d, network_ip_string()); at_reply(d, "\r\n"); ok(d); return; }

    if (!strcmp(uline, "$SSID?")) { at_reply(d, g_config.wifi_ssid); at_reply(d, "\r\n"); ok(d); return; }
    if (starts_with(uline, "$SSID=")) { strncpy(g_config.wifi_ssid, line + 6, WIFI_SSID_MAX); g_config.wifi_ssid[WIFI_SSID_MAX]=0; ok(d); return; }
    if (!strcmp(uline, "$PASS?")) { at_reply(d, g_config.wifi_pass[0] ? "Password : Set\r\n" : "Password : Not set\r\n"); ok(d); return; }
    if (starts_with(uline, "$PASS=")) { strncpy(g_config.wifi_pass, line + 6, WIFI_PASS_MAX); g_config.wifi_pass[WIFI_PASS_MAX]=0; ok(d); return; }
    if (!strcmp(uline, "$SB?")) { char b[32]; snprintf(b,sizeof(b),"%lu\r\n",(unsigned long)g_config.uart_baud); at_reply(d,b); ok(d); return; }
    if (starts_with(uline, "$SB=")) { g_config.uart_baud = (uint32_t)strtoul(line + 4, NULL, 10); rc_uart_set_baud(g_config.uart_baud); ok(d); return; }
    if (!strcmp(uline, "$SP?")) { char b[32]; snprintf(b,sizeof(b),"%u\r\n",g_config.tcp_port); at_reply(d,b); ok(d); return; }
    if (starts_with(uline, "$SP=")) { g_config.tcp_port = (uint16_t)strtoul(line + 4, NULL, 10); ok(d); return; }

    if (!strcmp(uline, "TERM?")) { at_reply(d, term_mode_name(g_config.term_mode)); at_reply(d,"\r\n"); ok(d); return; }
    if (!strcmp(uline, "TERM=RAW")) { g_config.term_mode = TERM_RAW; ok(d); return; }
    if (!strcmp(uline, "TERM=TELNET")) { g_config.term_mode = TERM_TELNET; ok(d); return; }

    if (!strcmp(uline, "CONSOLE?")) { at_reply(d, console_policy_name(g_config.console_policy)); at_reply(d,"\r\n"); ok(d); return; }
    if (!strcmp(uline, "CONSOLE=AUTO")) { g_config.console_policy = CONSOLE_AUTO; ok(d); return; }
    if (!strcmp(uline, "CONSOLE=USB")) { g_config.console_policy = CONSOLE_USB; ok(d); return; }
    if (!strcmp(uline, "CONSOLE=WIFI")) { g_config.console_policy = CONSOLE_WIFI; ok(d); return; }

    if (!strcmp(uline, "PASSWD?")) { at_reply(d, g_config.telnet_password[0] ? "Password : Set\r\n" : "Password : Not set\r\n"); ok(d); return; }
    if (starts_with(uline, "PASSWD=")) { strncpy(g_config.telnet_password, line + 7, TELNET_PASS_MAX); g_config.telnet_password[TELNET_PASS_MAX]=0; ok(d); return; }

    if (!strcmp(uline, "S0?")) { char b[32]; snprintf(b,sizeof(b),"%u\r\n",g_config.auto_answer); at_reply(d,b); ok(d); return; }
    if (starts_with(uline, "S0=")) { g_config.auto_answer = (uint8_t)strtoul(line + 3, NULL, 10); ok(d); return; }
    if (!strcmp(uline, "S12?")) { char b[32]; snprintf(b,sizeof(b),"%u\r\n",g_config.guard_time_ms); at_reply(d,b); ok(d); return; }
    if (starts_with(uline, "S12=")) { g_config.guard_time_ms = (uint16_t)strtoul(line + 4, NULL, 10); ok(d); return; }
    if (!strcmp(uline, "NOTIFY?")) { at_reply(d, g_config.notify ? "1\r\n" : "0\r\n"); ok(d); return; }
    if (!strcmp(uline, "NOTIFY=0")) { g_config.notify = false; ok(d); return; }
    if (!strcmp(uline, "NOTIFY=1")) { g_config.notify = true; ok(d); return; }

    error(d);
}

void at_enter_command_mode(console_owner_t src) {
    if (src == OWNER_WIFI) g_console.wifi_mode = SESSION_COMMAND;
    else g_console.usb_mode = SESSION_COMMAND;
    at_reply(src, "\r\nOK\r\n");
}

bool at_escape_feed(console_owner_t src, char ch) {
    at_state_t *s = state_for(src);
    uint64_t now = time_us_64();
    uint64_t guard = (uint64_t)g_config.guard_time_ms * 1000u;

    if (ch == '+' && (g_config.guard_time_ms == 0 || now - s->last_char_us >= guard || s->plus_count > 0)) {
        s->plus_count++;
        s->last_char_us = now;
        if (s->plus_count == 3) {
            // Simplicity: enter immediately. Future: wait post-guard before confirming.
            s->plus_count = 0;
            at_enter_command_mode(src);
            return true;
        }
        return true;
    }
    if (s->plus_count) {
        for (int i = 0; i < s->plus_count; i++) rc_uart_write('+');
        s->plus_count = 0;
    }
    s->last_char_us = now;
    return false;
}

void at_feed(console_owner_t src, char ch) {
    at_state_t *s = state_for(src);
    if (ch == '\r' || ch == '\n') {
        if (s->len > 0) {
            s->line[s->len] = 0;
            handle_command(src, s->line);
            s->len = 0;
        }
        return;
    }
    if (g_config.echo) at_reply(src, (char[]){ch,0});
    if (s->len < (int)sizeof(s->line)-1) s->line[s->len++] = ch;
}

void at_init(void) {
    memset(&usb_at, 0, sizeof(usb_at));
    memset(&wifi_at, 0, sizeof(wifi_at));
}
