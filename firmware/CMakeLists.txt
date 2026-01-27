cmake_minimum_required(VERSION 3.16)

# ESP-IDF CMake build system
include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

project(digi-dash-firmware)

# Add the engine as a static library component
idf_component_register(SRCS "src/main.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_hw_support esp_system)

# Link the shared engine
target_link_libraries(${COMPONENT_LIB} PUBLIC digidash-engine)
