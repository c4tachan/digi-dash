cmake_minimum_required(VERSION 3.16)
project(digidash-engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Engine library
add_library(digidash-engine
    src/vector_renderer.cpp
    src/binary_gauge_loader.cpp
    src/animation_engine.cpp
    src/gauge_scene.cpp
    src/font_manager.cpp
    src/pid_binding_system.cpp
)

target_include_directories(digidash-engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Platform abstractions
target_include_directories(digidash-engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/digidash/platform
)

# For ESP32, we'll link against ESP-IDF provided libraries
# For Linux simulator, we'll link against SDL2 and LVGL
if(IDF_TARGET)
    # Building for ESP32
else()
    # Building for Linux/Simulator
endif()
