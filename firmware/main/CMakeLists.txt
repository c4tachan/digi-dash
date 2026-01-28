idf_component_register(SRCS "main.cpp"
                           "application/application.cpp"
                           "platform/display/display_driver.cpp"
                           "subsystems/rendering/render_engine.cpp"
                           "subsystems/storage/storage_manager.cpp"
                       INCLUDE_DIRS "." 
                                    "application"
                                    "platform"
                                    "platform/display"
                                    "subsystems/rendering"
                                    "subsystems/storage"
                       REQUIRES freertos esp_hw_support esp_system vfs spiffs esp_lcd esp_timer)

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

# Platform configuration - set QEMU target flag
if(DEFINED CONFIG_DIGI_DASH_TARGET_QEMU)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_DIGI_DASH_TARGET_QEMU)
endif()

# Link ThorVG if available
find_package(thorvg CONFIG)
if(thorvg_FOUND)
    target_link_libraries(${COMPONENT_LIB} PUBLIC thorvg::thorvg)
endif()
