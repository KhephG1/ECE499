cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME ECE499_node)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})
#BSEC
add_library(BSEC STATIC IMPORTED)

set_target_properties(BSEC PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/bsec/bin/libalgobsec.a
)

target_include_directories(BSEC INTERFACE
    ${PROJECT_SOURCE_DIR}/third_party/bsec/inc
    ${PROJECT_SOURCE_DIR}/third_party/bsec/config
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/third_party/bsec/config/bsec_iaq.c
)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)
#Add LBM Lora Module Driver Library
set(LBM_RADIO sx1262 CACHE STRING "Radio used by LBM")
add_subdirectory(third_party/lbm/lbm_lib)
# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)
add_subdirectory(third_party/smtc_hal)
file(GLOB_RECURSE SENSOR_SOURCES CONFIGURE_DEPENDS "Core/Src/sensors/*.c")
file(GLOB_RECURSE SX1262_SOURCES CONFIGURE_DEPENDS "Core/Src/sx1262/*.c")
# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${SENSOR_SOURCES}
    ${SX1262_SOURCES}
    Core/Src/uart_logs.c
    Core/Src/lora_p2p.c
    third_party/lbm/lbm_lib/smtc_modem_hal/smtc_modem_hal.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Core/Inc/sensors/
    Core/Inc/sx1262/
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)
# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
     BSEC #bosh bsec binary
     m #math.h
    # Add user defined libraries
    LoRaBasicsModemApi
    lora_basics_modem_combined
    smtc_hal
    sx126x_driver
    smtc_ral::smtc_ral_sx126x
    smtc_ralf::smtc_ralf_sx126x
)
