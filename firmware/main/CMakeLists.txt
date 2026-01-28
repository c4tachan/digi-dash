idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES freertos esp_hw_support esp_system vfs spiffs)

# Get the firmware CMakeLists location
set(FIRMWARE_ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../engine")

# Add engine source files  
file(GLOB_RECURSE ENGINE_SOURCES "${FIRMWARE_ENGINE_DIR}/src/*.cpp")

# Compile engine files as part of this component
target_sources(${COMPONENT_LIB} PRIVATE ${ENGINE_SOURCES})

# Add engine include path
target_include_directories(${COMPONENT_LIB} PUBLIC "${FIRMWARE_ENGINE_DIR}/include")

# Configure C++ standard
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)

# Link ThorVG if available
find_package(thorvg CONFIG)
if(thorvg_FOUND)
    target_link_libraries(${COMPONENT_LIB} PUBLIC thorvg::thorvg)
endif()
