idf_component_register(
    SRCS
        "main.c"

        "wifi/wifi_manager.c"
        "web/web_server.c"
        "io/io.c"
        "storage/nvs_storage.c"
        "system/system_state.c"
        "led/led_status.c"
        "button/button_reset.c" 
        "mqtt/mqtt_integration.c"   # 🔹 NUEVO

    INCLUDE_DIRS
        "."
        "wifi"
        "web"
        "io"
        "storage"
        "system"
        "led"
        "button"
        "mqtt"                 # 🔹 NUEVO

    REQUIRES
        esp_wifi
        esp_event
        esp_netif
        esp_timer
        nvs_flash
        esp_http_server
        driver
        json
        freertos
        log
        mqtt                  # 🔹 Librería oficial de ESP-IDF
        display
        
)
