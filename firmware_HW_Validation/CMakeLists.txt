# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD weact_studio_rp2350b_core CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# set(PICO_EXTRAS_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/pico-extras)
# include(pico_extras_import.cmake)

project(HT-15_Test C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# set(PICO_EPAPER_PATH "src/libraries/pico-epaper" CACHE PATH "Path to pico-epaper")
# include(src/libraries/pico-epaper/external/pico_epaper_import.cmake)

# Add library subdirectory
# Add executable. Default name is the project name, version 0.1
add_executable(HT-15_Test 
    src/HT-15_Test.cpp 
    src/keypad.cpp 
    src/audio.c
)

pico_set_program_name(HT-15_Test "HT-15_Test")
pico_set_program_version(HT-15_Test "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(HT-15_Test 0)
pico_enable_stdio_usb(HT-15_Test 1)

add_subdirectory(lib/pico_tlv320_i2s)
add_subdirectory(lib/pico_ssd1681/pico)

# Add the standard library to the build
target_link_libraries(HT-15_Test 
                        pico_stdlib
                        pico_multicore
                        hardware_adc
                        hardware_spi
                        hardware_i2c
                        hardware_clocks
                        # pico_i2s
                        pico_tlv320dac3100
                        ssd1681
                        )

# Add the standard include files to the build
target_include_directories(HT-15_Test PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib/pico_tlv320_i2s
)

pico_add_extra_outputs(HT-15_Test)

