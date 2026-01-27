cmake_minimum_required(VERSION 3.16)

# For ESP-IDF, we need to build the engine as a library within this component
# This works with idf_component_register

# Set up paths
set(ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../engine")
set(ENGINE_INCLUDE "${ENGINE_DIR}/include")

# Compile engine source files
file(GLOB_RECURSE ENGINE_SOURCES "${ENGINE_DIR}/src/*.cpp")

# Register as ESP-IDF component
idf_component_register(SRCS "src/main.cpp" ${ENGINE_SOURCES}
                       INCLUDE_DIRS "include" "${ENGINE_INCLUDE}"
                       REQUIRES esp_hw_support esp_system freertos)

# Configure engine compilation
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra)

# Link ThorVG
find_package(thorvg REQUIRED CONFIG)
target_link_libraries(${COMPONENT_LIB} PUBLIC thorvg::thorvg)
