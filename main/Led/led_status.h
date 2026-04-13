#ifndef LED_STATUS_H
#define LED_STATUS_H

#include "system_state.h"

/* Inicializa el LED de estado en el GPIO indicado */
void led_status_init(int gpio);

/* Actualiza el estado del sistema (para el patrón del LED) */
void led_status_set_system_state(system_state_t state);

#endif // LED_STATUS_H
