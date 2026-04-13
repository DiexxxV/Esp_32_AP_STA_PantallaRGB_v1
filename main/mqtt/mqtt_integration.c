#include "mqtt_integration.h"
#include "system_state.h"
#include "wifi_mgr.h"
#include "io_ctrl.h"
#include "led_status.h"
#include "nvs_storage.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define TAG "MQTT"

/* ===== TOPICS ===== */
#define MQTT_TOPIC_STATUS "diego/esp32/status"
#define MQTT_TOPIC_CMD    "diego/esp32/cmd"

/* ===== MQTT CLIENT ===== */
static esp_mqtt_client_handle_t client = NULL;

/* ====================== EVENT HANDLER ====================== */
static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event_id) {

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            mqtt_subscribe(MQTT_TOPIC_CMD, 1);
            mqtt_publish(MQTT_TOPIC_STATUS, "ONLINE", 1, false);
            break;

        case MQTT_EVENT_DATA: {
            ESP_LOGI(TAG, "Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Data: %.*s", event->data_len, event->data);

            char topic[64] = {0};
            memcpy(topic, event->topic, event->topic_len);

            if (strcmp(topic, MQTT_TOPIC_CMD) == 0) {
                cJSON *root = cJSON_Parse(event->data);
                if (root) {
                    cJSON *relay = cJSON_GetObjectItem(root, "relay");
                    cJSON *state = cJSON_GetObjectItem(root, "state");

                    if (relay && state) {
                        io_set_relay(relay->valueint, state->valueint);
                        ESP_LOGI(TAG, "Relay %d -> %d",
                                 relay->valueint, state->valueint);
                    }
                    cJSON_Delete(root);
                }
            }
        } break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            break;

        default:
            break;
    }
}

/* ====================== INIT MQTT ====================== */
esp_err_t mqtt_init(void)
{
    if (client) return ESP_OK;

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = "mqtts://a81542c015374cecbbddc761de1a2e42.s1.eu.hivemq.cloud:8883",

        .credentials.username = "hivemq.webclient.1771361368426",
        .credentials.authentication.password = "DwlR!>zZ.G93iLnp10J*",

        // 👇 ESTO ES OBLIGATORIO PARA HIVE MQ CLOUD
        .broker.verification.certificate = esp_crt_bundle_attach,
    };

    client = esp_mqtt_client_init(&cfg);

    esp_mqtt_client_register_event(client,
                                   ESP_EVENT_ANY_ID,
                                   mqtt_event_handler,
                                   NULL);

    esp_mqtt_client_start(client);

    ESP_LOGI(TAG, "MQTT client started (HiveMQ Cloud)");
    return ESP_OK;
}

/* ====================== PUBLICAR ====================== */
esp_err_t mqtt_publish(const char *topic, const char *msg, int qos, bool retain)
{
    if (!client) return ESP_FAIL;
    esp_mqtt_client_publish(client, topic, msg, 0, qos, retain);
    return ESP_OK;
}

/* ====================== STATUS ====================== */
void mqtt_publish_status(void)
{
    if (!client) return;

    system_state_t st = system_state_get();

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "system", system_state_to_str(st));
    cJSON_AddBoolToObject(root, "wifi_connected", wifi_is_connected());
    cJSON_AddStringToObject(root, "ip", wifi_get_sta_ip());

    cJSON *inputs = cJSON_CreateObject();
    cJSON_AddNumberToObject(inputs, "in1", io_get_input(1));
    cJSON_AddNumberToObject(inputs, "in2", io_get_input(2));
    cJSON_AddItemToObject(root, "inputs", inputs);

    char *msg = cJSON_PrintUnformatted(root);
    mqtt_publish(MQTT_TOPIC_STATUS, msg, 1, false);

    cJSON_free(msg);
    cJSON_Delete(root);
}

/* ====================== SUBSCRIBE ====================== */
esp_err_t mqtt_subscribe(const char *topic, int qos)
{
    if (!client) return ESP_FAIL;
    esp_mqtt_client_subscribe(client, topic, qos);
    ESP_LOGI(TAG, "Subscribed: %s", topic);
    return ESP_OK;
}

/* ====================== TASK ====================== */
static void mqtt_task(void *param)
{
    while (1) {
        if (wifi_is_connected() && client) {
            mqtt_publish_status();
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void mqtt_start_task(void)
{
    xTaskCreate(mqtt_task, "mqtt_task", 4096, NULL, 5, NULL);
}
