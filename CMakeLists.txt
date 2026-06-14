cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_retro_console_bridge C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico_retro_console_bridge
    src/main.c
    src/config.c
    src/at.c
    src/console.c
    src/network.c
    src/uart.c
    src/buffer.c
    src/led.c
)

target_include_directories(pico_retro_console_bridge PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_definitions(pico_retro_console_bridge PRIVATE
    WIFI_COUNTRY=CYW43_COUNTRY_JAPAN
)

target_link_libraries(pico_retro_console_bridge
    pico_stdlib
    hardware_uart
    hardware_flash
    hardware_watchdog
    pico_cyw43_arch_lwip_threadsafe_background
)

pico_enable_stdio_usb(pico_retro_console_bridge 1)
pico_enable_stdio_uart(pico_retro_console_bridge 0)

pico_add_extra_outputs(pico_retro_console_bridge)
