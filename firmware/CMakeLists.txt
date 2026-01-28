cmake_minimum_required(VERSION 3.16)

# Use custom partition table with SPIFFS BEFORE project include
set(PARTITION_TABLE_CSV_PATH "${CMAKE_CURRENT_SOURCE_DIR}/partitions.csv")

# Source the IDF tools
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(digi-dash)
