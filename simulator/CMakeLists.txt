cmake_minimum_required(VERSION 3.16)
project(digi-dash-simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
find_package(SDL2 REQUIRED)

# Simulator executable
add_executable(digi-dash-simulator
    src/main.cpp
    src/sdl_display.cpp
    src/sdl_input.cpp
    src/fake_pid_provider.cpp
)

# Link against engine and SDL2
target_link_libraries(digi-dash-simulator
    digidash-engine
    SDL2::SDL2
)

target_include_directories(digi-dash-simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Copy assets to build directory
add_custom_command(TARGET digi-dash-simulator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_CURRENT_BINARY_DIR}/assets
)
