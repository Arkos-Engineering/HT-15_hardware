# CMakeLists.txt for HT-15 firmware libraries
# This file includes all library subdirectories for the HT-15 hardware validation firmware

cmake_minimum_required(VERSION 3.13)

# Add Pico-I2S library
add_library(pico_i2s INTERFACE)

target_sources(pico_i2s INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/Pico-I2S/src/buffer_queue.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Pico-I2S/src/dma_reader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Pico-I2S/src/dma_writer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Pico-I2S/src/i2s_master_clock.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Pico-I2S/src/i2s_master_input.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Pico-I2S/src/i2s_master_output.cpp
)

# Generate PIO header for I2S
pico_generate_pio_header(pico_i2s ${CMAKE_CURRENT_LIST_DIR}/Pico-I2S/src/i2s.pio)

target_include_directories(pico_i2s INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/Pico-I2S/src
)

target_link_libraries(pico_i2s INTERFACE
    pico_stdlib
    hardware_pio
    hardware_dma
)
