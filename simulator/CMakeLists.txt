cmake_minimum_required(VERSION 3.16)
project(digi-dash-simulator VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fix for x86_64 assembly compilation issues with LVGL
enable_language(ASM)
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -x assembler-with-cpp -o <OBJECT> <SOURCE>")

# Find SDL2
find_package(SDL2 REQUIRED)

# Fetch nlohmann/json for JSON configuration
include(FetchContent)
FetchContent_Declare(json 
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

# LVGL configuration
set(LV_CONF_PATH 
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h
    CACHE STRING "" FORCE)

# Disable ARM-specific optimizations for x86_64 simulator
set(LV_USE_DRAW_SW_ASM OFF CACHE BOOL "" FORCE)
set(LV_USE_DRAW_SW_ASM_HELIUM OFF CACHE BOOL "" FORCE)
set(LV_USE_DRAW_SW_ASM_NEON OFF CACHE BOOL "" FORCE)

# Skip ASM assembly source files (they're ARM-specific)
set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -E -x c -o <OBJECT> <SOURCE>" CACHE STRING "" FORCE)

# Add LVGL library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/lvgl)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/lvgl
    ${SDL2_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/gauge_window.cpp
    src/window_manager.cpp
    src/rpm_gauge.cpp
    src/speed_display.cpp
    src/temp_gauge.cpp
    src/mock_data.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Copy dashboard.json to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/dashboard.json
    ${CMAKE_CURRENT_BINARY_DIR}/dashboard.json
    COMMENT "Copying dashboard.json to build directory"
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    lvgl
    lvgl::thorvg
    ${SDL2_LIBRARIES}
    nlohmann_json::nlohmann_json
    pthread
    m
)

# Compile options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Enable debugging symbols
set(CMAKE_BUILD_TYPE Debug)
