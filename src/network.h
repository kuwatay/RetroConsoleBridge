#pragma once
#include <stdbool.h>
#include <stdint.h>

bool network_init(void);
void network_task(void);
bool network_connect(void);
void network_disconnect(void);
bool network_is_connected(void);
const char *network_ip_string(void);
const char *network_gateway_string(void);
const char *network_netmask_string(void);
const char *network_mac_string(void);
int network_rssi_dbm(void);
bool network_client_connected(void);
const char *network_client_ip_string(void);
uint32_t network_bytes_in(void);
uint32_t network_bytes_out(void);
void network_send_byte(uint8_t b);
void network_send_str(const char *s);
void network_close(void);
