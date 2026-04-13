#include "led_status.h"
#include "system_state.h"

#include <stdbool.h>   // ✅ NECESARIO

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "led_status"

static int led_gpio = -1;
static system_state_t current_state = SYS_INIT;
static SemaphoreHandle_t led_mutex = NULL;

static void led_task(void *arg)
{
    bool level = false;

    while (1) {
        int delay_ms = 1000;
        system_state_t state;

        if (xSemaphoreTake(led_mutex, portMAX_DELAY)) {
            state = current_state;
            xSemaphoreGive(led_mutex);
        } else {
            state = SYS_ERROR;
        }

        switch (state) {
            case SYS_AP_MODE:
                delay_ms = 1000; // parpadeo lento
                break;

            case SYS_CONNECTING_STA:
                delay_ms = 300; // medio rápido
                break;

            case SYS_STA_CONNECTED:
            case SYS_RUNNING:
                gpio_set_level(led_gpio, 1); // fijo
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;

            case SYS_WIFI_ERROR:
            case SYS_ERROR:
                delay_ms = 150; // rápido
                break;

            default:
                delay_ms = 800;
                break;
        }

        level = !level;
        gpio_set_level(led_gpio, level);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void led_status_set_system_state(system_state_t state)
{
    if (led_mutex == NULL) return;

    if (xSemaphoreTake(led_mutex, portMAX_DELAY)) {
        current_state = state;
        xSemaphoreGive(led_mutex);
    }
}

void led_status_init(int gpio)
{
    led_gpio = gpio;

    led_mutex = xSemaphoreCreateMutex();

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << led_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(led_gpio, 0);

    xTaskCreate(led_task, "led_task", 2048, NULL, 2, NULL);

    ESP_LOGI(TAG, "LED status task started on GPIO %d", led_gpio);
}
