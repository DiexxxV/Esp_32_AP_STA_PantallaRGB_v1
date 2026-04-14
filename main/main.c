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

/* ===== CONTROL DISPLAY DESDE WEB ===== */
typedef enum {
    DISP_MODE_AUTO = 0,
    DISP_MODE_LOGO,
    DISP_MODE_COLOR,
    DISP_MODE_CLEAR
} display_mode_t;

/* 🔥 Variables globales */
volatile display_mode_t g_display_mode = DISP_MODE_AUTO;
volatile uint8_t g_r = 0, g_g = 0, g_b = 0;

/* ===== FUNCIONES PARA WEB ===== */
void display_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    g_r = r;
    g_g = g;
    g_b = b;
    g_display_mode = DISP_MODE_COLOR;
}

void display_set_logo(void)
{
    g_display_mode = DISP_MODE_LOGO;
}

void display_set_clear(void)
{
    g_display_mode = DISP_MODE_CLEAR;
}

void display_set_auto(void)
{
    g_display_mode = DISP_MODE_AUTO;
}

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

    /* 🔥 FIX: usa API correcta del display */
    display_set_logo_color(255, 255, 255);
    display_draw_logo_colored(logo);

    display_draw_text(5, 54, "BOOT", 0, 255, 255);
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* ===== WiFi ===== */
    wifi_mgr_init();

    /* ===== WEB ===== */
    web_server_start();

    /* ===== INICIAL ===== */
    display_set_auto();

    ESP_LOGI(TAG, "System started");

    /* ===== CONTROL INTELIGENTE DE REDIBUJO ===== */
    display_mode_t last_mode = -1;
    uint8_t last_r = 255, last_g = 255, last_b = 255;
    system_state_t last_state = -1;

    /* ===== LOOP PRINCIPAL ===== */
    while (1)
    {
        /* =========================
           🔥 PRIORIDAD: WEB CONTROL
        ==========================*/

        if (g_display_mode == DISP_MODE_COLOR)
        {
            if (last_mode != DISP_MODE_COLOR ||
                last_r != g_r || last_g != g_g || last_b != g_b)
            {
                display_fill(g_r, g_g, g_b);

                last_r = g_r;
                last_g = g_g;
                last_b = g_b;
                last_mode = DISP_MODE_COLOR;
            }

            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (g_display_mode == DISP_MODE_LOGO)
        {
            if (last_mode != DISP_MODE_LOGO)
            {
                display_clear();

                /* 🔥 FIX: usar función correcta */
                display_set_logo_color(255, 255, 255);
                display_draw_logo_colored(logo);

                last_mode = DISP_MODE_LOGO;
            }

            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        if (g_display_mode == DISP_MODE_CLEAR)
        {
            if (last_mode != DISP_MODE_CLEAR)
            {
                display_clear();
                last_mode = DISP_MODE_CLEAR;
            }

            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        /* =========================
           🤖 MODO AUTOMÁTICO
        ==========================*/

        system_state_t st = system_state_get();

        if (st != last_state)
        {
            last_state = st;
            last_mode = DISP_MODE_AUTO;

            display_clear();
            display_set_logo_color(255, 255, 255);
            display_draw_logo_colored(logo);

            if (st != SYS_STA_CONNECTED && st != SYS_RUNNING)
            {
                display_draw_text(2, 54, "BUSCANDO...", 255, 150, 0);
            }
            else
            {
                display_draw_text(2, 54, "ONLINE", 0, 255, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}