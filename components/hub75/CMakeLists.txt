# HUB75 RGB LED matrix library utilizing ESP32 DMA Engine
# https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA
# MIT License

cmake_minimum_required(VERSION 3.5)

idf_build_get_property(target IDF_TARGET)

# 🔥 FORZAR MODO ESP-IDF PURO (SIN ARDUINO NI GFX)
set(CONFIG_ESP32_HUB75_USE_GFX 0)
set(ARDUINO_ARCH_ESP32 0)

# Dependencias reales en ESP-IDF
set(build_dependencies esp_lcd driver)

# Extra source para ESP32-S3
if(${target} STREQUAL "esp32s3")
    list(APPEND extra_srcs src/platforms/${target}/gdma_lcd_parallel16.cpp)

    if (NOT esp_lcd IN_LIST build_dependencies)
        list(APPEND build_dependencies esp_lcd)
    endif()
endif()

# Registro del componente
idf_component_register(
    SRCS
        "src/platforms/esp32/esp32_i2s_parallel_dma.cpp"
        "src/ESP32-HUB75-MatrixPanel-I2S-DMA.cpp"
        "src/ESP32-HUB75-MatrixPanel-leddrivers.cpp"
        ${extra_srcs}
    INCLUDE_DIRS
        "./src"
)

# Link dinámico de dependencias
idf_build_get_property(components BUILD_COMPONENTS)

foreach(component_name IN LISTS build_dependencies)
    if (${component_name} IN_LIST components)
        idf_component_get_property(lib_name ${component_name} COMPONENT_LIB)
        target_link_libraries(${COMPONENT_LIB} PUBLIC ${lib_name})
    endif()
endforeach()

# 🔥 CLAVE: DESACTIVAR GFX COMPLETAMENTE
target_compile_options(${COMPONENT_TARGET} PUBLIC -DNO_GFX)

# Opciones adicionales (opcionales)
# target_compile_options(${COMPONENT_TARGET} PUBLIC -DNO_FAST_FUNCTIONS)

project(ESP32-HUB75-MatrixPanel-I2S-DMA)