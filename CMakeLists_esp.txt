cmake_minimum_required(VERSION 3.16)

# This file is used when building with ESP-IDF
# It includes the engine component and firmware component

include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

project(digi-dash)

# Add engine as a component
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/engine engine)

# Firmware is registered as an ESP-IDF component via firmware/CMakeLists.txt
# which is automatically included by IDF_COMPONENT_MANAGER
