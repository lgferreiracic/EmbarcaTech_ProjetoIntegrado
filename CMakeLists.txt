cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)

# Define the project name and version
project(ProjetoIntegrado C CXX ASM)
pico_sdk_init()

add_executable(ProjetoIntegrado 
        ProjetoIntegrado.c #Main file
        inc/ssd1306.c #Library for OLED display
        )

target_link_libraries(ProjetoIntegrado 
            pico_stdlib
            hardware_i2c
            hardware_adc
            hardware_pwm
            hardware_clocks
            hardware_pio
        )
pico_enable_stdio_uart(ProjetoIntegrado 0)
pico_enable_stdio_usb(ProjetoIntegrado 1)
pico_generate_pio_header(ProjetoIntegrado ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

pico_add_extra_outputs(ProjetoIntegrado)
