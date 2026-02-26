cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_ssd1681 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Option to select SPI mode
option(USE_3WIRE_SPI "Use 3-wire SPI mode (9-bit frames)" OFF)

# Main library
add_library(ssd1681 STATIC
    pico_ssd1681.c
)

target_include_directories(ssd1681 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(ssd1681
    pico_stdlib
    hardware_spi
    hardware_gpio
)

if(USE_3WIRE_SPI)
    target_compile_definitions(ssd1681 PUBLIC USE_3WIRE_SPI)
endif()

# Example executable
add_executable(example
    example.c
)

target_link_libraries(example
    ssd1681
    pico_stdlib
)

if(USE_3WIRE_SPI)
    target_compile_definitions(example PUBLIC USE_3WIRE_SPI)
endif()

# Enable USB output
pico_enable_stdio_usb(example 1)
pico_enable_stdio_uart(example 0)

pico_add_extra_outputs(example)
