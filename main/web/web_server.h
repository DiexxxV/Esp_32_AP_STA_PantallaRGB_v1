#pragma once

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Arranca el servidor HTTP y registra handlers */
void web_server_start(void);

#ifdef __cplusplus
}
#endif
