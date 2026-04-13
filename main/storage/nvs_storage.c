#include "nvs_storage.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

#define TAG "nvs"

#define NVS_NS   "wifi_cfg"
#define KEY_SSID "ssid"
#define KEY_PASS "pass"

esp_err_t nvs_save_wifi(const char *ssid, const char *pass)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_str(h, KEY_SSID, ssid);
    if (err != ESP_OK) {
        nvs_close(h);
        return err;
    }

    err = nvs_set_str(h, KEY_PASS, pass);
    if (err != ESP_OK) {
        nvs_close(h);
        return err;
    }

    err = nvs_commit(h);
    nvs_close(h);

    ESP_LOGI(TAG, "WiFi credentials saved");
    return err;
}

/* 
 * Devuelve true si hay datos válidos
 * false si no existen
 */
bool nvs_load_wifi(char *ssid, size_t ssid_len,
                   char *pass, size_t pass_len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No NVS namespace");
        return false;
    }

    memset(ssid, 0, ssid_len);
    memset(pass, 0, pass_len);

    err = nvs_get_str(h, KEY_SSID, ssid, &ssid_len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No SSID saved");
        nvs_close(h);
        return false;
    }

    err = nvs_get_str(h, KEY_PASS, pass, &pass_len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No PASS saved");
        nvs_close(h);
        return false;
    }

    nvs_close(h);

    ESP_LOGI(TAG, "Loaded WiFi: %s", ssid);
    return true;
}

/* Borra credenciales WiFi */
esp_err_t nvs_clear_wifi(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No namespace to clear");
        return err;
    }

    err = nvs_erase_all(h);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }

    nvs_close(h);

    ESP_LOGW(TAG, "WiFi credentials erased");
    return err;
}
