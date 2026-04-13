#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_STATE_AP = 0,
    APP_STATE_SCANNING,
    APP_STATE_STA_CONNECTING,
    APP_STATE_STA_CONNECTED,
    APP_STATE_STA_FAILED
} app_state_t;

/* Estado actual */
void app_state_set(app_state_t state);
app_state_t app_state_get(void);

/* IP STA (para UI) */
void app_state_set_ip(const char *ip);
const char *app_state_get_ip(void);

#ifdef __cplusplus
}
#endif
