cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wps_softap_registrar)

# -----------------------------------------------------------------------------
# main/CMakeLists.txt
# -----------------------------------------------------------------------------


# Coloca este archivo dentro de la carpeta /main
# -----------------------------------------------
# idf_component_register(
# SRCS
# "main.c"
# "wifi_manager.c"
# "http_server.c"
# "io_control.c"
# INCLUDE_DIRS "."
# REQUIRES esp_wifi esp_event nvs_flash esp_http_server driver json
# )