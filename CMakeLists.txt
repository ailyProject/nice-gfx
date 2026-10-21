# Native ESP-IDF is the default. Arduino-as-a-component callers explicitly set
# NICE_GFX_ARDUINO_COMPONENT to arduino or espressif__arduino-esp32.
set(nice_gfx_root "${CMAKE_CURRENT_LIST_DIR}")
set(nice_gfx_requires esp_lcd esp_driver_spi esp_driver_gpio esp_timer
    esp_hw_support esp_system esp_mm esp_psram heap freertos)
if(IDF_TARGET MATCHES "^(esp32p4|esp32s31)$")
    list(APPEND nice_gfx_requires esp_driver_ppa)
endif()
if(NICE_GFX_ARDUINO_COMPONENT)
    list(APPEND nice_gfx_requires ${NICE_GFX_ARDUINO_COMPONENT})
endif()

# IDF also evaluates component requirements before build targets exist.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    file(GLOB_RECURSE nice_gfx_sources CONFIGURE_DEPENDS "${nice_gfx_root}/src/*.cpp")
    if(NOT NICE_GFX_ARDUINO_COMPONENT)
        list(FILTER nice_gfx_sources EXCLUDE REGEX "/Platform/(Arduino|RP2040|STM32)/")
    endif()
    # Optional build-only selection. The default retains every bundled font.
    # Names are e.g. Fixed6x13;GfxSansCN16; setFont() remains unchanged.
    if(NICE_GFX_FONT_SOURCES)
        list(FILTER nice_gfx_sources EXCLUDE REGEX "/Fonts/Mono/GFX_Font_[^/]+\\.cpp$")
        foreach(nice_gfx_font IN LISTS NICE_GFX_FONT_SOURCES)
            if(NOT nice_gfx_font MATCHES "^[A-Za-z0-9_]+$")
                message(FATAL_ERROR "Invalid NICE_GFX_FONT_SOURCES entry: ${nice_gfx_font}")
            endif()
            set(nice_gfx_font_source "${nice_gfx_root}/src/Fonts/Mono/GFX_Font_${nice_gfx_font}.cpp")
            if(NOT EXISTS "${nice_gfx_font_source}")
                message(FATAL_ERROR "Unknown bundled font: ${nice_gfx_font}")
            endif()
            list(APPEND nice_gfx_sources "${nice_gfx_font_source}")
        endforeach()
        list(REMOVE_DUPLICATES nice_gfx_sources)
    endif()
endif()
idf_component_register(SRCS ${nice_gfx_sources}
    INCLUDE_DIRS "${nice_gfx_root}/src" REQUIRES ${nice_gfx_requires})
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
    target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-error=misleading-indentation)
    target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=missing-field-initializers)
endif()
