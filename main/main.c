#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "wifi_mgr.h"
#include "web_server.h"
#include "io_ctrl.h"
#include "nvs_storage.h"
#include "led_status.h"
#include "system_state.h"
#include "button_reset.h"
#include "mqtt_integration.h"

/* DISPLAY */
#include "display.h"

static const char *TAG = "MAIN";

/* Imagen global */
extern const uint16_t logo[4096];

void app_main(void)
{
    esp_err_t err;

    /* ===== NVS ===== */
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* ===== Core ESP ===== */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* ===== SYSTEM STATE ===== */
    system_state_init();
    system_state_set(SYS_INIT);

    /* ===== LED ===== */
    led_status_init(4);

    /* ===== BOTÓN ===== */
    button_reset_init(35);

    /* ===== IO ===== */
    io_init();

    /* ===== DISPLAY ===== */
    display_init();

    /* ===== SPLASH ===== */
    display_clear();
    display_draw_image(logo);
    display_draw_text(5, 54, "BOOT", 0, 255, 255);
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* ===== WiFi ===== */
    wifi_mgr_init();

    /* ===== WEB ===== */
    web_server_start();

    /* ===== STATUS INICIAL ===== */
    display_clear();
    display_draw_image(logo);
    display_draw_text(2, 54, "WIFI...", 255, 255, 0);

    ESP_LOGI(TAG, "System started");

    /* ===== LOOP PRINCIPAL ===== */
    while (1)
    {
        system_state_t st = system_state_get();

        // ===== WIFI NO CONECTADO =====
        if (st != SYS_STA_CONNECTED && st != SYS_RUNNING)
        {
            display_clear();
            display_draw_image(logo);
            display_draw_text(2, 54, "BUSCANDO...", 255, 150, 0);

            vTaskDelay(pdMS_TO_TICKS(1500));
            continue;
        }

        // ===== WIFI OK =====
        display_clear();
        display_draw_image(logo);
        display_draw_text(2, 54, "ONLINE", 0, 255, 0);
        vTaskDelay(pdMS_TO_TICKS(1500));

        // ===== ANIMACIÓN =====
        display_clear();
        display_fill(0, 0, 40); // azul
        vTaskDelay(pdMS_TO_TICKS(400));

        display_clear();
        display_fill(40, 0, 0); // rojo
        vTaskDelay(pdMS_TO_TICKS(400));

        // ===== RUN =====
        display_clear();
        display_draw_image(logo);
        display_draw_text(2, 54, "RUN", 0, 255, 255);
        vTaskDelay(pdMS_TO_TICKS(1200));
    }
}