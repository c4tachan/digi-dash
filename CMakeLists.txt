# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# SPIFFS partition for SVG files
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components)

project(digi-dash-esp32)

spiffs_create_partition_image(storage ${CMAKE_SOURCE_DIR}/assets FLASH_IN_PROJECT)
