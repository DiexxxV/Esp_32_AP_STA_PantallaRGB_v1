#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa el cliente MQTT y conecta al broker */
esp_err_t mqtt_init(void);


/* Publica el estado del sistema y los inputs */
void mqtt_publish_status(void);

/* Publica un mensaje genérico en un topic */
esp_err_t mqtt_publish(const char *topic, const char *msg, int qos, bool retain);

/* Suscribe un topic para recibir comandos */
esp_err_t mqtt_subscribe(const char *topic, int qos);

/* Lanza la tarea de suscripción/recepción de MQTT */
void mqtt_start_task(void);

#ifdef __cplusplus
}
#endif
