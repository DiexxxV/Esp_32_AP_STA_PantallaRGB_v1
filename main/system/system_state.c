#include "system_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "led_status.h"

#include "esp_log.h"

#define TAG "system_state"

/* =======================
 * Variables internas
 * ======================= */
static system_state_t current_state = SYS_INIT;
static SemaphoreHandle_t state_mutex = NULL;

/* =======================
 * Init
 * ======================= */
void system_state_init(void)
{
    if (state_mutex == NULL) {
        state_mutex = xSemaphoreCreateMutex();
    }

    current_state = SYS_INIT;
    led_status_set_system_state(current_state);

    ESP_LOGI(TAG, "System state initialized: %s",
             system_state_to_str(current_state));
}

/* =======================
 * Set state
 * ======================= */
void system_state_set(system_state_t state)
{
    if (state_mutex == NULL) return;

    if (xSemaphoreTake(state_mutex, portMAX_DELAY)) {

        if (current_state != state) {
            ESP_LOGI(TAG, "State change: %s -> %s",
                     system_state_to_str(current_state),
                     system_state_to_str(state));
        }

        current_state = state;

        /* Actualiza LED */
        led_status_set_system_state(state);

        xSemaphoreGive(state_mutex);
    }
}

/* =======================
 * Get state
 * ======================= */
system_state_t system_state_get(void)
{
    system_state_t state = SYS_ERROR;

    if (state_mutex == NULL) return SYS_ERROR;

    if (xSemaphoreTake(state_mutex, portMAX_DELAY)) {
        state = current_state;
        xSemaphoreGive(state_mutex);
    }

    return state;
}

/* =======================
 * State to string
 * ======================= */
const char *system_state_to_str(system_state_t state)
{
    switch (state) {
        case SYS_INIT:           return "INIT";
        case SYS_AP_MODE:        return "AP_MODE";
        case SYS_CONNECTING_STA: return "CONNECTING_STA";
        case SYS_STA_CONNECTED:  return "STA_CONNECTED";
        case SYS_RUNNING:        return "RUNNING";
        case SYS_WIFI_ERROR:     return "WIFI_ERROR";
        case SYS_ERROR:          return "ERROR";
        default:                 return "UNKNOWN";
    }
}

/* =======================
 * Helpers
 * ======================= */
bool system_state_is_running(void)
{
    system_state_t s = system_state_get();

    return (s == SYS_STA_CONNECTED ||
            s == SYS_RUNNING);
}
