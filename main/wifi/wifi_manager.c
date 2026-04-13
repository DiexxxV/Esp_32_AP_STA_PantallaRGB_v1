#include "wifi_mgr.h"
#include "nvs_storage.h"
#include "system_state.h"

#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "cJSON.h"

#define TAG "wifi_mgr"

/* ===== AP Config ===== */
#define AP_SSID "ESP32_Config"
#define AP_PASS "configureme"

/* ===== Event bits ===== */
#define WIFI_CONNECTED BIT0
#define WIFI_FAILED    BIT1

static EventGroupHandle_t wifi_events;
static char sta_ip[16] = "0.0.0.0";

/* =========================================================
 *                  WiFi Event Handler
 * ========================================================= */
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_DISCONNECTED) {

        ESP_LOGW(TAG, "STA disconnected");

        xEventGroupClearBits(wifi_events, WIFI_CONNECTED);
        xEventGroupSetBits(wifi_events, WIFI_FAILED);

        system_state_set(SYS_WIFI_ERROR);
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(sta_ip, sizeof(sta_ip),
                 IPSTR, IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(wifi_events, WIFI_CONNECTED);
        xEventGroupClearBits(wifi_events, WIFI_FAILED);

        system_state_set(SYS_STA_CONNECTED);

        ESP_LOGI(TAG, "STA connected, IP: %s", sta_ip);
    }
}

/* =========================================================
 *                  WiFi Init
 * ========================================================= */
void wifi_mgr_init(void)
{
    wifi_events = xEventGroupCreate();

    system_state_set(SYS_AP_MODE);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT,
                                   ESP_EVENT_ANY_ID,
                                   wifi_event_handler,
                                   NULL));

    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT,
                                   IP_EVENT_STA_GOT_IP,
                                   wifi_event_handler,
                                   NULL));

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_config_t ap_cfg = {0};
    strcpy((char *)ap_cfg.ap.ssid, AP_SSID);
    strcpy((char *)ap_cfg.ap.password, AP_PASS);
    ap_cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_cfg.ap.max_connection = 4;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    ESP_LOGI(TAG, "AP started: %s", AP_SSID);

    /* ===== AUTOCONEXIÓN DESDE NVS ===== */
    char ssid[32] = {0};
    char pass[64] = {0};

    if (nvs_load_wifi(ssid, sizeof(ssid), pass, sizeof(pass))) {
        ESP_LOGI(TAG, "Auto connecting to saved WiFi: %s", ssid);
        wifi_connect_sta(ssid, pass);
    } else {
        ESP_LOGI(TAG, "No saved WiFi, staying in AP mode");
    }
}

/* =========================================================
 *                  Scan Networks
 * ========================================================= */
void wifi_scan(char *json, size_t max)
{
    uint16_t count = 0;
    wifi_scan_config_t scan_cfg = {
        .show_hidden = true
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_cfg, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&count));

    if (count == 0) {
        snprintf(json, max, "[]");
        return;
    }

    wifi_ap_record_t *aps = calloc(count, sizeof(wifi_ap_record_t));

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&count, aps));

    cJSON *root = cJSON_CreateArray();

    for (int i = 0; i < count; i++) {
        cJSON *ap = cJSON_CreateObject();
        cJSON_AddStringToObject(ap, "ssid", (char *)aps[i].ssid);
        cJSON_AddNumberToObject(ap, "rssi", aps[i].rssi);
        cJSON_AddItemToArray(root, ap);
    }

    char *tmp = cJSON_PrintUnformatted(root);
    strncpy(json, tmp, max - 1);
    json[max - 1] = 0;

    free(tmp);
    cJSON_Delete(root);
    free(aps);
}

/* =========================================================
 *                  STA Connect Task
 * ========================================================= */
typedef struct {
    char ssid[32];
    char pass[64];
    int retries;
} wifi_sta_t;

static void wifi_sta_task(void *param)
{
    wifi_sta_t *sta = (wifi_sta_t *)param;

    wifi_config_t cfg = {0};
    strncpy((char *)cfg.sta.ssid, sta->ssid,
            sizeof(cfg.sta.ssid) - 1);
    strncpy((char *)cfg.sta.password, sta->pass,
            sizeof(cfg.sta.password) - 1);

    cfg.sta.scan_method = WIFI_FAST_SCAN;
    cfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));

    system_state_set(SYS_CONNECTING_STA);

    xEventGroupClearBits(wifi_events,
                         WIFI_CONNECTED | WIFI_FAILED);

    for (int i = 0; i < sta->retries; i++) {

        ESP_LOGI(TAG, "Connecting to %s (%d/%d)",
                 sta->ssid, i + 1, sta->retries);

        esp_wifi_connect();

        EventBits_t bits =
            xEventGroupWaitBits(wifi_events,
                                WIFI_CONNECTED | WIFI_FAILED,
                                pdTRUE,
                                pdFALSE,
                                pdMS_TO_TICKS(8000));

        if (bits & WIFI_CONNECTED) {
            nvs_save_wifi(sta->ssid, sta->pass);
            ESP_LOGI(TAG, "WiFi connected OK");
            free(sta);
            vTaskDelete(NULL);
            return;
        }

        ESP_LOGW(TAG, "Retry...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    ESP_LOGE(TAG, "WiFi connection failed");
    system_state_set(SYS_AP_MODE);

    free(sta);
    vTaskDelete(NULL);
}

/* =========================================================
 *                  Public Connect API
 * ========================================================= */
void wifi_connect_sta(const char *ssid, const char *pass)
{
    if (!ssid || strlen(ssid) == 0) return;

    if (!pass || strlen(pass) < 8 || strlen(pass) > 63) {
        ESP_LOGW(TAG, "Invalid WPA password length");
        return;
    }

    wifi_sta_t *sta = malloc(sizeof(wifi_sta_t));
    if (!sta) return;

    memset(sta, 0, sizeof(wifi_sta_t));

    strncpy(sta->ssid, ssid, sizeof(sta->ssid) - 1);
    strncpy(sta->pass, pass, sizeof(sta->pass) - 1);
    sta->retries = 10;

    xTaskCreate(wifi_sta_task,
                "wifi_sta_task",
                4096,
                sta,
                5,
                NULL);
}

/* =========================================================
 *                  Get STA IP
 * ========================================================= */
const char *wifi_get_sta_ip(void)
{
    return sta_ip;
}

/* =========================================================
 *                  Is Connected
 * ========================================================= */
bool wifi_is_connected(void)
{
    EventBits_t bits = xEventGroupGetBits(wifi_events);
    return (bits & WIFI_CONNECTED);
}
