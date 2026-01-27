cmake_minimum_required(VERSION 3.16)
project(svg_preprocessor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add cmake directory to module path for FindThorVG.cmake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake)

# Find ThorVG - provide helpful error message if not found
find_package(thorvg REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(digidash_preproc
    src/svg_loader.cpp
    src/svg_normalizer.cpp
    src/path_flattener.cpp
    src/gauge_serializer.cpp
)

target_link_libraries(digidash_preproc PUBLIC thorvg::thorvg)

add_executable(svg_preprocessor
    src/main.cpp
)

target_link_libraries(svg_preprocessor PRIVATE digidash_preproc)