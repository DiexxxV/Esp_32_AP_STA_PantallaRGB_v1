#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include "app_state.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIFI_MGR_H
#define WIFI_MGR_H

/* ===== Init WiFi (AP + STA) ===== */
void wifi_mgr_init(void);

/* ===== Scan WiFi ===== */
void wifi_scan(char *json, size_t max);

/* ===== Connect to selected network ===== */
void wifi_connect_sta(const char *ssid, const char *pass);

/* ===== Get STA IP ===== */
const char *wifi_get_sta_ip(void);

/* ===== NEW: WiFi connected? ===== */
bool wifi_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MGR_H */
