# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and c/c++ standards
project(midi_to_cv C CXX ASM)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME}
   main.c
   midi_uart.c 
   mcp4725.c
)

# Create mab/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})
	
target_link_libraries(${PROJECT_NAME}
   pico_stdlib
   hardware_i2c
   hardware_gpio
   hardware_uart
)

# Where the standard input/output will be routed
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
