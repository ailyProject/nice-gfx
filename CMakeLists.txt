# Native IDF is the default. Existing adapters can explicitly request Arduino.
if(NOT NICE_GFX_ARDUINO_COMPONENT)
    set(nice_gfx_root "${CMAKE_CURRENT_LIST_DIR}")
    if(NOT CMAKE_BUILD_EARLY_EXPANSION)
        file(GLOB_RECURSE nice_gfx_sources CONFIGURE_DEPENDS "${nice_gfx_root}/src/*.cpp")
        list(FILTER nice_gfx_sources EXCLUDE REGEX "/Platform/(Arduino|RP2040|STM32)/")
    endif()
    idf_component_register(
        SRCS ${nice_gfx_sources}
        INCLUDE_DIRS "${nice_gfx_root}/src"
        REQUIRES esp_lcd esp_driver_spi esp_driver_gpio esp_driver_ppa
                 esp_timer esp_hw_support esp_system esp_mm esp_psram heap freertos)
    target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
    # The shared renderer uses compact statements in public inline functions.
    # Keep this style diagnostic visible with IDF's default warning policy.
    target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-error=misleading-indentation)
    # Older generated font aggregates omit optional trailing zero fields.
    target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=missing-field-initializers)
    return()
endif()

# Detect the standard managed/local names, including during IDF's early
# requirements expansion. A custom adapter may set the name before this include.
if(NOT NICE_GFX_ARDUINO_COMPONENT)
    idf_build_get_property(nice_gfx_project_dir PROJECT_DIR)
    if(EXISTS "${nice_gfx_project_dir}/managed_components/espressif__arduino-esp32/CMakeLists.txt")
        set(NICE_GFX_ARDUINO_COMPONENT espressif__arduino-esp32)
    elseif(EXISTS "${nice_gfx_project_dir}/components/arduino-esp32/CMakeLists.txt")
        set(NICE_GFX_ARDUINO_COMPONENT arduino-esp32)
    else()
        set(NICE_GFX_ARDUINO_COMPONENT arduino)
    endif()
endif()

# Absolute paths also allow external component adapters to reuse this source list.
set(nice_gfx_root "${CMAKE_CURRENT_LIST_DIR}")
# IDF expands dependencies in script mode before configuring build targets.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    file(GLOB_RECURSE nice_gfx_sources CONFIGURE_DEPENDS "${nice_gfx_root}/src/*.cpp")
endif()

# Arduino, IDF and exports use the same independent source units.
idf_component_register(
    SRCS ${nice_gfx_sources}
    INCLUDE_DIRS "${nice_gfx_root}/src"
    REQUIRES ${NICE_GFX_ARDUINO_COMPONENT} esp_lcd esp_driver_spi
             esp_driver_gpio esp_hw_support heap freertos)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
