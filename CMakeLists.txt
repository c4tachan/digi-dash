cmake_minimum_required(VERSION 3.16)
project(digi-dash)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add cmake directory to module path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Build engine by default
add_subdirectory(engine)

# Build SVG preprocessor
add_subdirectory(tools/svg_preprocessor)

# Build simulator on Linux
if(UNIX AND NOT APPLE)
    add_subdirectory(simulator)
endif()
