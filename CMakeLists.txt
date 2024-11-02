# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if(EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
# We also need PICO EXTRAS
# pico_extras is inside the pico_sdk_path
set(PICO_EXTRAS_PATH $ENV{PICO_SDK_PATH}/pico_extras)
include(${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(ClockEvent-C C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Run python scripts
message("Running create_font python script")
execute_process(COMMAND
    py src/python/create_font.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

# Add executable. Default name is the project name, version 0.1

add_executable(${PROJECT_NAME}
    src/ClockEvent-C.cpp
    src/DS3231/DS3231.cpp
    src/graphics/ST7735/ST7735.cpp
    src/graphics/graphics.cpp
    src/Rotary/Rotary.cpp
    src/Rotary/Button/Button.cpp
    src/Settings/Settings.cpp
    src/Settings/TimeMaster/TimeMaster.cpp
    src/settings_config/settings_config.cpp
)

pico_set_program_name(${PROJECT_NAME} "ClockEvent-C")
pico_set_program_version(${PROJECT_NAME} "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

# Add the standard library to the build
target_link_libraries(${PROJECT_NAME}
    pico_stdlib)

# Add the standard include files to the build
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    #   ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
    # ${CMAKE_CURRENT_LIST_DIR}/src/access_point # for our common lwipopts or any other standard includes, if required

)

# Add any user requested libraries
target_link_libraries(${PROJECT_NAME}
    hardware_spi
    hardware_i2c
    hardware_dma
    hardware_pio
    hardware_interp
    hardware_timer
    hardware_watchdog
    hardware_clocks
    hardware_sleep # from pico_extras
)

pico_add_extra_outputs(${PROJECT_NAME})

