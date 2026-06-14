#include "network.h"
#include "config.h"
#include "console.h"
#include "at.h"
#include "led.h"
#include "uart.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <string.h>
#include <stdio.h>

static struct tcp_pcb *server_pcb;
static struct tcp_pcb *client_pcb;
static bool wifi_connected;
static char ipbuf[24] = "0.0.0.0";
static char gwbuf[24] = "0.0.0.0";
static char maskbuf[24] = "0.0.0.0";
static char macbuf[18] = "00:00:00:00:00:00";
static char client_ipbuf[24] = "0.0.0.0";
static uint32_t bytes_in;
static uint32_t bytes_out;
static char auth_buf[TELNET_PASS_MAX + 1];
static int auth_len;

static err_t client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

static void telnet_filter_and_forward(const uint8_t *data, uint16_t len) {
    // Initial simple TELNET mode: strip IAC command triplets. RAW mode forwards all bytes.
    for (uint16_t i = 0; i < len; i++) {
        uint8_t c = data[i];
        if (g_config.term_mode == TERM_TELNET && c == 0xFF) {
            if (i + 2 < len) i += 2;
            continue;
        }
        if (g_console.wifi_mode == SESSION_ONLINE) {
            if (at_escape_feed(OWNER_WIFI, (char)c)) continue;
            rc_uart_write(c);
	    led_pulse_rx();
        } else if (g_console.wifi_mode == SESSION_COMMAND) {
            at_feed(OWNER_WIFI, (char)c);
        } else if (g_console.wifi_mode == SESSION_AUTH) {
            if (c == '\r' || c == '\n') {
                auth_buf[auth_len] = 0;
                if (strcmp(auth_buf, g_config.telnet_password) == 0) {
                    network_send_str("\r\nCONNECT ");
                    char b[32]; snprintf(b, sizeof(b), "%lu\r\n", (unsigned long)g_config.uart_baud);
                    network_send_str(b);
                    console_on_wifi_connected();
                } else {
                    network_send_str("\r\nERROR\r\n");
                    network_close();
                }
                auth_len = 0;
            } else if (auth_len < TELNET_PASS_MAX) {
                auth_buf[auth_len++] = (char)c;
            }
        }
    }
}

static err_t client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg;
    if (!p) {
        client_pcb = NULL;
	led_set_link(false);
        console_on_wifi_disconnected();
        return ERR_OK;
    }
    tcp_recved(tpcb, p->tot_len);
    bytes_in += p->tot_len;
    for (struct pbuf *q = p; q; q = q->next) {
        telnet_filter_and_forward((const uint8_t *)q->payload, q->len);
    }
    pbuf_free(p);
    return ERR_OK;
}

static err_t client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) { (void)arg; (void)tpcb; (void)len; return ERR_OK; }

static void client_err(void *arg, err_t err) {
    (void)arg; (void)err;
    client_pcb = NULL;
    led_set_link(false);
    console_on_wifi_disconnected();
}

static err_t server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    (void)arg; (void)err;
    if (client_pcb) {
        tcp_write(newpcb, "BUSY\r\n", 6, TCP_WRITE_FLAG_COPY);
        tcp_close(newpcb);
        return ERR_OK;
    }
    client_pcb = newpcb;
    snprintf(client_ipbuf, sizeof(client_ipbuf), "%s", ipaddr_ntoa(&newpcb->remote_ip));
    tcp_recv(client_pcb, client_recv);
    tcp_sent(client_pcb, client_sent);
    tcp_err(client_pcb, client_err);
    auth_len = 0;

    if (g_config.telnet_password[0]) {
        g_console.wifi_mode = SESSION_AUTH;
        network_send_str("Pico Retro Console Bridge\r\nPassword: ");
    } else {
        network_send_str("CONNECT ");
        char b[32]; snprintf(b, sizeof(b), "%lu\r\n", (unsigned long)g_config.uart_baud);
        network_send_str(b);
	led_set_link(true);
	console_on_wifi_connected();
    }
    return ERR_OK;
}

static void start_server(void) {
    if (server_pcb) return;
    server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!server_pcb) return;
    if (tcp_bind(server_pcb, IP_ANY_TYPE, g_config.tcp_port) != ERR_OK) {
        tcp_close(server_pcb); server_pcb = NULL; return;
    }
    server_pcb = tcp_listen(server_pcb);
    tcp_accept(server_pcb, server_accept);
}

bool network_init(void) {
  if (cyw43_arch_init_with_country(WIFI_COUNTRY)) {
    return false;
  }
  cyw43_arch_enable_sta_mode();
  return true;  
}

bool network_connect(void) {
    if (!g_config.wifi_ssid[0]) return false;
    int r = cyw43_arch_wifi_connect_timeout_ms(g_config.wifi_ssid, g_config.wifi_pass, CYW43_AUTH_WPA2_AES_PSK, 30000);
    wifi_connected = (r == 0);
    led_set_wifi(wifi_connected);
    if (wifi_connected) {
        const ip4_addr_t *addr = netif_ip4_addr(netif_default);
        snprintf(ipbuf, sizeof(ipbuf), "%s", ip4addr_ntoa(addr));
        snprintf(gwbuf, sizeof(gwbuf), "%s", ip4addr_ntoa(netif_ip4_gw(netif_default)));
        snprintf(maskbuf, sizeof(maskbuf), "%s", ip4addr_ntoa(netif_ip4_netmask(netif_default)));
        uint8_t mac[6];
        cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac);
        snprintf(macbuf, sizeof(macbuf), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        start_server();
    }
    return wifi_connected;
}

void network_disconnect(void) {
    network_close();
    cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
    wifi_connected = false;
    led_set_wifi(false);
    strcpy(ipbuf, "0.0.0.0");
    strcpy(gwbuf, "0.0.0.0");
    strcpy(maskbuf, "0.0.0.0");
}

bool network_is_connected(void) { return wifi_connected; }
const char *network_ip_string(void) { return ipbuf; }
const char *network_gateway_string(void) { return gwbuf; }
const char *network_netmask_string(void) { return maskbuf; }
const char *network_mac_string(void) { return macbuf; }
int network_rssi_dbm(void) {
    int32_t rssi = 0;
    if (!wifi_connected) return 0;
    if (cyw43_wifi_get_rssi(&cyw43_state, &rssi) != 0) return 0;
    return (int)rssi;
}
bool network_client_connected(void) { return client_pcb != NULL; }
const char *network_client_ip_string(void) { return client_ipbuf; }
uint32_t network_bytes_in(void) { return bytes_in; }
uint32_t network_bytes_out(void) { return bytes_out; }

void network_send_byte(uint8_t b) {
    if (!client_pcb) return;
    cyw43_arch_lwip_begin();
    tcp_write(client_pcb, &b, 1, TCP_WRITE_FLAG_COPY);
    bytes_out += 1;
    tcp_output(client_pcb);
    led_pulse_tx();
    cyw43_arch_lwip_end();
}

void network_send_str(const char *s) {
    if (!client_pcb) return;
    cyw43_arch_lwip_begin();
    u16_t len = (u16_t)strlen(s);
    tcp_write(client_pcb, s, len, TCP_WRITE_FLAG_COPY);
    bytes_out += len;
    tcp_output(client_pcb);
    cyw43_arch_lwip_end();
}

void network_close(void) {
    if (!client_pcb) return;
    cyw43_arch_lwip_begin();
    tcp_close(client_pcb);
    client_pcb = NULL;
    cyw43_arch_lwip_end();
    console_on_wifi_disconnected();
}

void network_task(void) {
    // Using pico_cyw43_arch_lwip_threadsafe_background, background polling is handled by SDK.
}
