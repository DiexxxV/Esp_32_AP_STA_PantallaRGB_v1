#pragma once

#include <stdint.h>
#include <stdbool.h>

/* =======================
 * System States
 * ======================= */
typedef enum {
    SYS_INIT = 0,          // Boot / startup
    SYS_AP_MODE,           // AP activo (modo configuración)
    SYS_CONNECTING_STA,    // Intentando conectar a WiFi
    SYS_STA_CONNECTED,     // Conectado a WiFi + IP
    SYS_RUNNING,           // Sistema operativo normal
    SYS_WIFI_ERROR,        // Error WiFi (timeout, auth, etc)
    SYS_ERROR              // Error crítico general
} system_state_t;

/* =======================
 * API
 * ======================= */

/**
 * @brief Inicializa el sistema de estado (DEBE llamarse una vez)
 */
void system_state_init(void);

/**
 * @brief Cambia el estado actual del sistema
 */
void system_state_set(system_state_t state);

/**
 * @brief Obtiene el estado actual del sistema
 */
system_state_t system_state_get(void);

/**
 * @brief Obtiene el estado en formato string (para logs / web)
 */
const char *system_state_to_str(system_state_t state);

/**
 * @brief Indica si el sistema está operativo
 */
bool system_state_is_running(void);
