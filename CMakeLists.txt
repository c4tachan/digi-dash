cmake_minimum_required(VERSION 3.16)
project(digi-dash)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add cmake directory to module path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Build engine by default
add_subdirectory(engine)

# Build SVG preprocessor (optional - requires ThorVG)
find_package(thorvg QUIET)
if(thorvg_FOUND)
    add_subdirectory(tools/svg_preprocessor)
    message(STATUS "ThorVG found - SVG preprocessor enabled")
else()
    message(STATUS "ThorVG not found - SVG preprocessor disabled")
    message(STATUS "To enable SVG preprocessing, install ThorVG:")
    message(STATUS "  Ubuntu/Debian: sudo apt-get install libthorvg-dev")
    message(STATUS "  macOS: brew install thorvg")
    message(STATUS "  From source: https://github.com/thorvg/thorvg")
endif()

# Build simulator on Linux
if(UNIX AND NOT APPLE)
    add_subdirectory(simulator)
endif()
