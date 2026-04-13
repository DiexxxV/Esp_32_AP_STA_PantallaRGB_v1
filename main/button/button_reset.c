#include "button_reset.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "nvs_storage.h"
#include "system_state.h"

#define TAG "button_reset"

#define RESET_TIME_US (5 * 1000000) // 5 segundos

static int btn_gpio = -1;
static int64_t press_time = 0;

static void IRAM_ATTR button_isr(void *arg)
{
    int level = gpio_get_level(btn_gpio);

    if (level == 0) { // presionado
        press_time = esp_timer_get_time();
    } else { // soltado
        int64_t delta = esp_timer_get_time() - press_time;

        if (delta >= RESET_TIME_US) {
            ESP_LOGW(TAG, "RESET WiFi triggered");

            nvs_clear_wifi();
            system_state_set(SYS_AP_MODE);

            esp_restart();
        }
    }
}

void button_reset_init(int gpio)
{
    btn_gpio = gpio;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << btn_gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(btn_gpio, button_isr, NULL);

    ESP_LOGI(TAG, "Reset button on GPIO %d", gpio);
}
