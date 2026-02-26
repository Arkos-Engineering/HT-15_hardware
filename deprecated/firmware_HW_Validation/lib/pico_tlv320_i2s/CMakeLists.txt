cmake_minimum_required(VERSION 3.13)

# Include Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_tlv320dac3100 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Create library
add_library(pico_tlv320dac3100
    pico_tlv320dac3100.c
)

target_include_directories(pico_tlv320dac3100 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(pico_tlv320dac3100
    pico_stdlib
    hardware_i2c
)

# Optional: Build example
if(BUILD_EXAMPLES)
    add_executable(tlv320_example
        example.c
    )

    target_link_libraries(tlv320_example
        pico_stdlib
        hardware_i2c
        pico_tlv320dac3100
    )

    # Enable USB output, disable UART output
    pico_enable_stdio_usb(tlv320_example 1)
    pico_enable_stdio_uart(tlv320_example 0)

    # Create map/bin/hex/uf2 files
    pico_add_extra_outputs(tlv320_example)
endif()
