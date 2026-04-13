#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa NVS (si quieres centralizarlo aquí) */
esp_err_t nvs_storage_init(void);

/* Guarda credenciales WiFi */
esp_err_t nvs_save_wifi(const char *ssid, const char *pass);

/* 
 * Carga credenciales WiFi
 * return:
 *   true  -> hay datos válidos
 *   false -> no hay datos guardados
 */
bool nvs_load_wifi(char *ssid, size_t ssid_len,
                   char *password, size_t pass_len);

/* Borra credenciales WiFi guardadas */
esp_err_t nvs_clear_wifi(void);

#ifdef __cplusplus
}
#endif
